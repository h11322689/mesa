   /**
   * \file enable.c
   * Enable/disable/query GL capabilities.
   */
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
         #include "util/glheader.h"
   #include "arrayobj.h"
   #include "blend.h"
   #include "clip.h"
   #include "context.h"
   #include "debug_output.h"
   #include "draw_validate.h"
   #include "enable.h"
   #include "errors.h"
   #include "light.h"
   #include "mtypes.h"
   #include "enums.h"
   #include "state.h"
   #include "texstate.h"
   #include "varray.h"
   #include "api_exec_decl.h"
      #include "state_tracker/st_cb_bitmap.h"
   #include "state_tracker/st_context.h"
      void
   _mesa_update_derived_primitive_restart_state(struct gl_context *ctx)
   {
      if (ctx->Array.PrimitiveRestart ||
      ctx->Array.PrimitiveRestartFixedIndex) {
   unsigned restart_index[3] = {
      _mesa_primitive_restart_index(ctx, 1),
   _mesa_primitive_restart_index(ctx, 2),
               ctx->Array._RestartIndex[0] = restart_index[0];
   ctx->Array._RestartIndex[1] = restart_index[1];
            /* Enable primitive restart only when the restart index can have an
   * effect. This is required for correctness in AMD GFX8 support.
   * Other hardware may also benefit from taking a faster, non-restart path
   * when possible.
   */
   ctx->Array._PrimitiveRestart[0] = true && restart_index[0] <= UINT8_MAX;
   ctx->Array._PrimitiveRestart[1] = true && restart_index[1] <= UINT16_MAX;
      } else {
      ctx->Array._PrimitiveRestart[0] = false;
   ctx->Array._PrimitiveRestart[1] = false;
         }
         /**
   * Helper to enable/disable VAO client-side state.
   */
   static void
   vao_state(struct gl_context *ctx, struct gl_vertex_array_object* vao,
         {
      if (state)
         else
      }
         /**
   * Helper to enable/disable client-side state.
   */
   static void
   client_state(struct gl_context *ctx, struct gl_vertex_array_object* vao,
         {
      switch (cap) {
      case GL_VERTEX_ARRAY:
      vao_state(ctx, vao, VERT_ATTRIB_POS, state);
      case GL_NORMAL_ARRAY:
      vao_state(ctx, vao, VERT_ATTRIB_NORMAL, state);
      case GL_COLOR_ARRAY:
      vao_state(ctx, vao, VERT_ATTRIB_COLOR0, state);
      case GL_INDEX_ARRAY:
      vao_state(ctx, vao, VERT_ATTRIB_COLOR_INDEX, state);
      case GL_TEXTURE_COORD_ARRAY:
      vao_state(ctx, vao, VERT_ATTRIB_TEX(ctx->Array.ActiveTexture), state);
      case GL_EDGE_FLAG_ARRAY:
      vao_state(ctx, vao, VERT_ATTRIB_EDGEFLAG, state);
      case GL_FOG_COORDINATE_ARRAY_EXT:
      vao_state(ctx, vao, VERT_ATTRIB_FOG, state);
      case GL_SECONDARY_COLOR_ARRAY_EXT:
                  case GL_POINT_SIZE_ARRAY_OES:
      if (ctx->VertexProgram.PointSizeEnabled != state) {
      FLUSH_VERTICES(ctx, ctx->st->lower_point_size ? _NEW_PROGRAM : 0,
         ctx->NewDriverState |= ST_NEW_RASTERIZER;
      }
               /* GL_NV_primitive_restart */
   case GL_PRIMITIVE_RESTART_NV:
      if (!_mesa_has_NV_primitive_restart(ctx))
                        ctx->Array.PrimitiveRestart = state;
               default:
      }
         invalid_enum_error:
      _mesa_error(ctx, GL_INVALID_ENUM, "gl%sClientState(%s)",
      }
         /* Helper for GL_EXT_direct_state_access following functions:
   *   - EnableClientStateIndexedEXT
   *   - EnableClientStateiEXT
   *   - DisableClientStateIndexedEXT
   *   - DisableClientStateiEXT
   */
   static void
   client_state_i(struct gl_context *ctx, struct gl_vertex_array_object* vao,
         {
               if (cap != GL_TEXTURE_COORD_ARRAY) {
      _mesa_error(ctx, GL_INVALID_ENUM, "gl%sClientStateiEXT(cap=%s)",
      state ? "Enable" : "Disable",
                  if (index >= ctx->Const.MaxTextureCoordUnits) {
      _mesa_error(ctx, GL_INVALID_VALUE, "gl%sClientStateiEXT(index=%d)",
      state ? "Enable" : "Disable",
                  saved_active = ctx->Array.ActiveTexture;
   _mesa_ClientActiveTexture(GL_TEXTURE0 + index);
   client_state(ctx, vao, cap, state);
      }
         /**
   * Enable GL capability.
   * \param cap  state to enable/disable.
   *
   * Get's the current context, assures that we're outside glBegin()/glEnd() and
   * calls client_state().
   */
   void GLAPIENTRY
   _mesa_EnableClientState( GLenum cap )
   {
      GET_CURRENT_CONTEXT(ctx);
      }
         void GLAPIENTRY
   _mesa_EnableVertexArrayEXT( GLuint vaobj, GLenum cap )
   {
      GET_CURRENT_CONTEXT(ctx);
   struct gl_vertex_array_object* vao = _mesa_lookup_vao_err(ctx, vaobj,
               if (!vao)
            /* The EXT_direct_state_access spec says:
   *    "Additionally EnableVertexArrayEXT and DisableVertexArrayEXT accept
   *    the tokens TEXTURE0 through TEXTUREn where n is less than the
   *    implementation-dependent limit of MAX_TEXTURE_COORDS.  For these
   *    GL_TEXTUREi tokens, EnableVertexArrayEXT and DisableVertexArrayEXT
   *    act identically to EnableVertexArrayEXT(vaobj, TEXTURE_COORD_ARRAY)
   *    or DisableVertexArrayEXT(vaobj, TEXTURE_COORD_ARRAY) respectively
   *    as if the active client texture is set to texture coordinate set i
   *    based on the token TEXTUREi indicated by array."
   */
   if (GL_TEXTURE0 <= cap && cap < GL_TEXTURE0 + ctx->Const.MaxTextureCoordUnits) {
      GLuint saved_active = ctx->Array.ActiveTexture;
   _mesa_ClientActiveTexture(cap);
   client_state(ctx, vao, GL_TEXTURE_COORD_ARRAY, GL_TRUE);
      } else {
            }
         void GLAPIENTRY
   _mesa_EnableClientStateiEXT( GLenum cap, GLuint index )
   {
      GET_CURRENT_CONTEXT(ctx);
      }
         /**
   * Disable GL capability.
   * \param cap  state to enable/disable.
   *
   * Get's the current context, assures that we're outside glBegin()/glEnd() and
   * calls client_state().
   */
   void GLAPIENTRY
   _mesa_DisableClientState( GLenum cap )
   {
      GET_CURRENT_CONTEXT(ctx);
      }
      void GLAPIENTRY
   _mesa_DisableVertexArrayEXT( GLuint vaobj, GLenum cap )
   {
      GET_CURRENT_CONTEXT(ctx);
   struct gl_vertex_array_object* vao = _mesa_lookup_vao_err(ctx, vaobj,
               if (!vao)
            /* The EXT_direct_state_access spec says:
   *    "Additionally EnableVertexArrayEXT and DisableVertexArrayEXT accept
   *    the tokens TEXTURE0 through TEXTUREn where n is less than the
   *    implementation-dependent limit of MAX_TEXTURE_COORDS.  For these
   *    GL_TEXTUREi tokens, EnableVertexArrayEXT and DisableVertexArrayEXT
   *    act identically to EnableVertexArrayEXT(vaobj, TEXTURE_COORD_ARRAY)
   *    or DisableVertexArrayEXT(vaobj, TEXTURE_COORD_ARRAY) respectively
   *    as if the active client texture is set to texture coordinate set i
   *    based on the token TEXTUREi indicated by array."
   */
   if (GL_TEXTURE0 <= cap && cap < GL_TEXTURE0 + ctx->Const.MaxTextureCoordUnits) {
      GLuint saved_active = ctx->Array.ActiveTexture;
   _mesa_ClientActiveTexture(cap);
   client_state(ctx, vao, GL_TEXTURE_COORD_ARRAY, GL_FALSE);
      } else {
            }
      void GLAPIENTRY
   _mesa_DisableClientStateiEXT( GLenum cap, GLuint index )
   {
      GET_CURRENT_CONTEXT(ctx);
      }
      /**
   * Return pointer to current texture unit for setting/getting coordinate
   * state.
   * Note that we'll set GL_INVALID_OPERATION and return NULL if the active
   * texture unit is higher than the number of supported coordinate units.
   */
   static struct gl_fixedfunc_texture_unit *
   get_texcoord_unit(struct gl_context *ctx)
   {
      if (ctx->Texture.CurrentUnit >= ctx->Const.MaxTextureCoordUnits) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glEnable/Disable(texcoord unit)");
      }
   else {
            }
         /**
   * Helper function to enable or disable a texture target.
   * \param bit  one of the TEXTURE_x_BIT values
   * \return GL_TRUE if state is changing or GL_FALSE if no change
   */
   static GLboolean
   enable_texture(struct gl_context *ctx, GLboolean state, GLbitfield texBit)
   {
      struct gl_fixedfunc_texture_unit *texUnit =
         if (!texUnit)
            const GLbitfield newenabled = state
            if (texUnit->Enabled == newenabled)
            FLUSH_VERTICES(ctx, _NEW_TEXTURE_STATE, GL_TEXTURE_BIT | GL_ENABLE_BIT);
   texUnit->Enabled = newenabled;
      }
         /**
   * Helper function to enable or disable GL_MULTISAMPLE, skipping the check for
   * whether the API supports it (GLES doesn't).
   */
   void
   _mesa_set_multisample(struct gl_context *ctx, GLboolean state)
   {
      if (ctx->Multisample.Enabled == state)
            /* GL compatibility needs Multisample.Enable to determine program state
   * constants.
   */
   if (_mesa_is_desktop_gl_compat(ctx) || _mesa_is_gles1(ctx)) {
         } else {
                  ctx->NewDriverState |= ctx->DriverFlags.NewMultisampleEnable;
      }
      /**
   * Helper function to enable or disable GL_FRAMEBUFFER_SRGB, skipping the
   * check for whether the API supports it (GLES doesn't).
   */
   void
   _mesa_set_framebuffer_srgb(struct gl_context *ctx, GLboolean state)
   {
      if (ctx->Color.sRGBEnabled == state)
            /* TODO: Switch i965 to the new flag and remove the conditional */
   FLUSH_VERTICES(ctx, 0,
         ctx->NewDriverState |= ST_NEW_FB_STATE;
      }
      /**
   * Helper function to enable or disable state.
   *
   * \param ctx GL context.
   * \param cap  the state to enable/disable
   * \param state whether to enable or disable the specified capability.
   *
   * Updates the current context and flushes the vertices as needed. For
   * capabilities associated with extensions it verifies that those extensions
   * are effectivly present before updating. Notifies the driver via
   * dd_function_table::Enable.
   */
   void
   _mesa_set_enable(struct gl_context *ctx, GLenum cap, GLboolean state)
   {
      if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "%s %s (newstate is %x)\n",
                     switch (cap) {
      case GL_ALPHA_TEST:
      if (ctx->API != API_OPENGL_COMPAT && ctx->API != API_OPENGLES)
         if (ctx->Color.AlphaEnabled == state)
         /* AlphaEnabled is used by the fixed-func fragment program */
   FLUSH_VERTICES(ctx, _NEW_COLOR | _NEW_FF_FRAG_PROGRAM,
         ctx->NewDriverState |= ctx->DriverFlags.NewAlphaTest;
   ctx->Color.AlphaEnabled = state;
      case GL_AUTO_NORMAL:
      if (ctx->API != API_OPENGL_COMPAT)
         if (ctx->Eval.AutoNormal == state)
         FLUSH_VERTICES(ctx, 0, GL_EVAL_BIT | GL_ENABLE_BIT);
   vbo_exec_update_eval_maps(ctx);
   ctx->Eval.AutoNormal = state;
      case GL_BLEND:
      {
      GLbitfield newEnabled =
         if (newEnabled != ctx->Color.BlendEnabled) {
      _mesa_flush_vertices_for_blend_adv(ctx, newEnabled,
         ctx->PopAttribState |= GL_ENABLE_BIT;
   ctx->Color.BlendEnabled = newEnabled;
   _mesa_update_allow_draw_out_of_order(ctx);
         }
      case GL_CLIP_DISTANCE0: /* aka GL_CLIP_PLANE0 */
   case GL_CLIP_DISTANCE1:
   case GL_CLIP_DISTANCE2:
   case GL_CLIP_DISTANCE3:
   case GL_CLIP_DISTANCE4:
   case GL_CLIP_DISTANCE5:
   case GL_CLIP_DISTANCE6:
   case GL_CLIP_DISTANCE7:
                                       if ((ctx->Transform.ClipPlanesEnabled & (1 << p))
                  /* The compatibility profile needs _NEW_TRANSFORM to transform
   * clip planes according to the projection matrix.
   */
   if (_mesa_is_desktop_gl_compat(ctx) || _mesa_is_gles1(ctx)) {
      FLUSH_VERTICES(ctx, _NEW_TRANSFORM,
      } else {
                                          /* The projection matrix transforms the clip plane. */
   /* TODO: glEnable might not be the best place to do it. */
   if (_mesa_is_desktop_gl_compat(ctx) || _mesa_is_gles1(ctx)) {
      _mesa_update_clip_plane(ctx, p);
         }
   else {
            }
      case GL_COLOR_MATERIAL:
      if (ctx->API != API_OPENGL_COMPAT && ctx->API != API_OPENGLES)
         if (ctx->Light.ColorMaterialEnabled == state)
         FLUSH_VERTICES(ctx, _NEW_LIGHT_CONSTANTS | _NEW_FF_VERT_PROGRAM,
         FLUSH_CURRENT(ctx, 0);
   ctx->Light.ColorMaterialEnabled = state;
   if (state) {
      _mesa_update_color_material( ctx,
      }
      case GL_CULL_FACE:
      if (ctx->Polygon.CullFlag == state)
         FLUSH_VERTICES(ctx, 0,
         ctx->NewDriverState |= ST_NEW_RASTERIZER;
   ctx->Polygon.CullFlag = state;
      case GL_DEPTH_TEST:
      if (ctx->Depth.Test == state)
         FLUSH_VERTICES(ctx, 0,
         ctx->NewDriverState |= ST_NEW_DSA;
   ctx->Depth.Test = state;
   _mesa_update_allow_draw_out_of_order(ctx);
      case GL_DEBUG_OUTPUT:
   case GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB:
      _mesa_set_debug_state_int(ctx, cap, state);
   _mesa_update_debug_callback(ctx);
      case GL_DITHER:
      if (ctx->Color.DitherFlag == state)
         FLUSH_VERTICES(ctx, 0,
         ctx->NewDriverState |= ST_NEW_BLEND;
   ctx->Color.DitherFlag = state;
      case GL_FOG:
      if (ctx->API != API_OPENGL_COMPAT && ctx->API != API_OPENGLES)
         if (ctx->Fog.Enabled == state)
         FLUSH_VERTICES(ctx, _NEW_FOG | _NEW_FF_FRAG_PROGRAM,
         ctx->Fog.Enabled = state;
   ctx->Fog._PackedEnabledMode = state ? ctx->Fog._PackedMode : FOG_NONE;
      case GL_LIGHT0:
   case GL_LIGHT1:
   case GL_LIGHT2:
   case GL_LIGHT3:
   case GL_LIGHT4:
   case GL_LIGHT5:
   case GL_LIGHT6:
   case GL_LIGHT7:
      if (ctx->API != API_OPENGL_COMPAT && ctx->API != API_OPENGLES)
         if (ctx->Light.Light[cap-GL_LIGHT0].Enabled == state)
         FLUSH_VERTICES(ctx, _NEW_LIGHT_CONSTANTS | _NEW_FF_VERT_PROGRAM,
         ctx->Light.Light[cap-GL_LIGHT0].Enabled = state;
   if (state) {
         }
   else {
         }
      case GL_LIGHTING:
      if (ctx->API != API_OPENGL_COMPAT && ctx->API != API_OPENGLES)
         if (ctx->Light.Enabled == state)
         FLUSH_VERTICES(ctx, _NEW_LIGHT_CONSTANTS | _NEW_FF_VERT_PROGRAM |
               ctx->Light.Enabled = state;
      case GL_LINE_SMOOTH:
      if (!_mesa_is_desktop_gl(ctx) && ctx->API != API_OPENGLES)
         if (ctx->Line.SmoothFlag == state)
         FLUSH_VERTICES(ctx, 0,
         ctx->NewDriverState |= ST_NEW_RASTERIZER;
   ctx->Line.SmoothFlag = state;
      case GL_LINE_STIPPLE:
      if (ctx->API != API_OPENGL_COMPAT)
         if (ctx->Line.StippleFlag == state)
         FLUSH_VERTICES(ctx, 0,
         ctx->NewDriverState |= ST_NEW_RASTERIZER;
   ctx->Line.StippleFlag = state;
      case GL_INDEX_LOGIC_OP:
      if (ctx->API != API_OPENGL_COMPAT)
         if (ctx->Color.IndexLogicOpEnabled == state)
         FLUSH_VERTICES(ctx, 0,
         ctx->NewDriverState |= ST_NEW_BLEND;
   ctx->Color.IndexLogicOpEnabled = state;
      case GL_CONSERVATIVE_RASTERIZATION_INTEL:
      if (!_mesa_has_INTEL_conservative_rasterization(ctx))
         if (ctx->IntelConservativeRasterization == state)
         FLUSH_VERTICES(ctx, 0, 0);
   ctx->NewDriverState |= ST_NEW_RASTERIZER;
   ctx->IntelConservativeRasterization = state;
   _mesa_update_valid_to_render_state(ctx);
      case GL_CONSERVATIVE_RASTERIZATION_NV:
      if (!_mesa_has_NV_conservative_raster(ctx))
         if (ctx->ConservativeRasterization == state)
         FLUSH_VERTICES(ctx, 0, GL_ENABLE_BIT);
   ctx->NewDriverState |= ST_NEW_RASTERIZER;
   ctx->ConservativeRasterization = state;
      case GL_COLOR_LOGIC_OP:
      if (!_mesa_is_desktop_gl(ctx) && ctx->API != API_OPENGLES)
         if (ctx->Color.ColorLogicOpEnabled == state)
         FLUSH_VERTICES(ctx, 0,
         ctx->NewDriverState |= ST_NEW_BLEND;
   ctx->Color.ColorLogicOpEnabled = state;
   _mesa_update_allow_draw_out_of_order(ctx);
      case GL_MAP1_COLOR_4:
      if (ctx->API != API_OPENGL_COMPAT)
         if (ctx->Eval.Map1Color4 == state)
         FLUSH_VERTICES(ctx, 0, GL_EVAL_BIT | GL_ENABLE_BIT);
   vbo_exec_update_eval_maps(ctx);
   ctx->Eval.Map1Color4 = state;
      case GL_MAP1_INDEX:
      if (ctx->API != API_OPENGL_COMPAT)
         if (ctx->Eval.Map1Index == state)
         FLUSH_VERTICES(ctx, 0, GL_EVAL_BIT | GL_ENABLE_BIT);
   vbo_exec_update_eval_maps(ctx);
   ctx->Eval.Map1Index = state;
      case GL_MAP1_NORMAL:
      if (ctx->API != API_OPENGL_COMPAT)
         if (ctx->Eval.Map1Normal == state)
         FLUSH_VERTICES(ctx, 0, GL_EVAL_BIT | GL_ENABLE_BIT);
   vbo_exec_update_eval_maps(ctx);
   ctx->Eval.Map1Normal = state;
      case GL_MAP1_TEXTURE_COORD_1:
      if (ctx->API != API_OPENGL_COMPAT)
         if (ctx->Eval.Map1TextureCoord1 == state)
         FLUSH_VERTICES(ctx, 0, GL_EVAL_BIT | GL_ENABLE_BIT);
   vbo_exec_update_eval_maps(ctx);
   ctx->Eval.Map1TextureCoord1 = state;
      case GL_MAP1_TEXTURE_COORD_2:
      if (ctx->API != API_OPENGL_COMPAT)
         if (ctx->Eval.Map1TextureCoord2 == state)
         FLUSH_VERTICES(ctx, 0, GL_EVAL_BIT | GL_ENABLE_BIT);
   vbo_exec_update_eval_maps(ctx);
   ctx->Eval.Map1TextureCoord2 = state;
      case GL_MAP1_TEXTURE_COORD_3:
      if (ctx->API != API_OPENGL_COMPAT)
         if (ctx->Eval.Map1TextureCoord3 == state)
         FLUSH_VERTICES(ctx, 0, GL_EVAL_BIT | GL_ENABLE_BIT);
   vbo_exec_update_eval_maps(ctx);
   ctx->Eval.Map1TextureCoord3 = state;
      case GL_MAP1_TEXTURE_COORD_4:
      if (ctx->API != API_OPENGL_COMPAT)
         if (ctx->Eval.Map1TextureCoord4 == state)
         FLUSH_VERTICES(ctx, 0, GL_EVAL_BIT | GL_ENABLE_BIT);
   vbo_exec_update_eval_maps(ctx);
   ctx->Eval.Map1TextureCoord4 = state;
      case GL_MAP1_VERTEX_3:
      if (ctx->API != API_OPENGL_COMPAT)
         if (ctx->Eval.Map1Vertex3 == state)
         FLUSH_VERTICES(ctx, 0, GL_EVAL_BIT | GL_ENABLE_BIT);
   vbo_exec_update_eval_maps(ctx);
   ctx->Eval.Map1Vertex3 = state;
      case GL_MAP1_VERTEX_4:
      if (ctx->API != API_OPENGL_COMPAT)
         if (ctx->Eval.Map1Vertex4 == state)
         FLUSH_VERTICES(ctx, 0, GL_EVAL_BIT | GL_ENABLE_BIT);
   vbo_exec_update_eval_maps(ctx);
   ctx->Eval.Map1Vertex4 = state;
      case GL_MAP2_COLOR_4:
      if (ctx->API != API_OPENGL_COMPAT)
         if (ctx->Eval.Map2Color4 == state)
         FLUSH_VERTICES(ctx, 0, GL_EVAL_BIT | GL_ENABLE_BIT);
   vbo_exec_update_eval_maps(ctx);
   ctx->Eval.Map2Color4 = state;
      case GL_MAP2_INDEX:
      if (ctx->API != API_OPENGL_COMPAT)
         if (ctx->Eval.Map2Index == state)
         FLUSH_VERTICES(ctx, 0, GL_EVAL_BIT | GL_ENABLE_BIT);
   vbo_exec_update_eval_maps(ctx);
   ctx->Eval.Map2Index = state;
      case GL_MAP2_NORMAL:
      if (ctx->API != API_OPENGL_COMPAT)
         if (ctx->Eval.Map2Normal == state)
         FLUSH_VERTICES(ctx, 0, GL_EVAL_BIT | GL_ENABLE_BIT);
   vbo_exec_update_eval_maps(ctx);
   ctx->Eval.Map2Normal = state;
      case GL_MAP2_TEXTURE_COORD_1:
      if (ctx->API != API_OPENGL_COMPAT)
         if (ctx->Eval.Map2TextureCoord1 == state)
         FLUSH_VERTICES(ctx, 0, GL_EVAL_BIT | GL_ENABLE_BIT);
   vbo_exec_update_eval_maps(ctx);
   ctx->Eval.Map2TextureCoord1 = state;
      case GL_MAP2_TEXTURE_COORD_2:
      if (ctx->API != API_OPENGL_COMPAT)
         if (ctx->Eval.Map2TextureCoord2 == state)
         FLUSH_VERTICES(ctx, 0, GL_EVAL_BIT | GL_ENABLE_BIT);
   vbo_exec_update_eval_maps(ctx);
   ctx->Eval.Map2TextureCoord2 = state;
      case GL_MAP2_TEXTURE_COORD_3:
      if (ctx->API != API_OPENGL_COMPAT)
         if (ctx->Eval.Map2TextureCoord3 == state)
         FLUSH_VERTICES(ctx, 0, GL_EVAL_BIT | GL_ENABLE_BIT);
   vbo_exec_update_eval_maps(ctx);
   ctx->Eval.Map2TextureCoord3 = state;
      case GL_MAP2_TEXTURE_COORD_4:
      if (ctx->API != API_OPENGL_COMPAT)
         if (ctx->Eval.Map2TextureCoord4 == state)
         FLUSH_VERTICES(ctx, 0, GL_EVAL_BIT | GL_ENABLE_BIT);
   vbo_exec_update_eval_maps(ctx);
   ctx->Eval.Map2TextureCoord4 = state;
      case GL_MAP2_VERTEX_3:
      if (ctx->API != API_OPENGL_COMPAT)
         if (ctx->Eval.Map2Vertex3 == state)
         FLUSH_VERTICES(ctx, 0, GL_EVAL_BIT | GL_ENABLE_BIT);
   vbo_exec_update_eval_maps(ctx);
   ctx->Eval.Map2Vertex3 = state;
      case GL_MAP2_VERTEX_4:
      if (ctx->API != API_OPENGL_COMPAT)
         if (ctx->Eval.Map2Vertex4 == state)
         FLUSH_VERTICES(ctx, 0, GL_EVAL_BIT | GL_ENABLE_BIT);
   vbo_exec_update_eval_maps(ctx);
   ctx->Eval.Map2Vertex4 = state;
      case GL_NORMALIZE:
      if (ctx->API != API_OPENGL_COMPAT && ctx->API != API_OPENGLES)
         if (ctx->Transform.Normalize == state)
         FLUSH_VERTICES(ctx, _NEW_TRANSFORM | _NEW_FF_VERT_PROGRAM,
         ctx->Transform.Normalize = state;
      case GL_POINT_SMOOTH:
      if (ctx->API != API_OPENGL_COMPAT && ctx->API != API_OPENGLES)
         if (ctx->Point.SmoothFlag == state)
         FLUSH_VERTICES(ctx, _NEW_POINT, GL_POINT_BIT | GL_ENABLE_BIT);
   ctx->Point.SmoothFlag = state;
      case GL_POLYGON_SMOOTH:
      if (!_mesa_is_desktop_gl(ctx))
         if (ctx->Polygon.SmoothFlag == state)
         FLUSH_VERTICES(ctx, 0,
         ctx->NewDriverState |= ST_NEW_RASTERIZER;
   ctx->Polygon.SmoothFlag = state;
      case GL_POLYGON_STIPPLE:
      if (ctx->API != API_OPENGL_COMPAT)
         if (ctx->Polygon.StippleFlag == state)
         FLUSH_VERTICES(ctx, 0,
         ctx->NewDriverState |= ST_NEW_RASTERIZER;
   ctx->Polygon.StippleFlag = state;
      case GL_POLYGON_OFFSET_POINT:
      if (!_mesa_is_desktop_gl(ctx))
         if (ctx->Polygon.OffsetPoint == state)
         FLUSH_VERTICES(ctx, 0,
         ctx->NewDriverState |= ST_NEW_RASTERIZER;
   ctx->Polygon.OffsetPoint = state;
      case GL_POLYGON_OFFSET_LINE:
      if (!_mesa_is_desktop_gl(ctx))
         if (ctx->Polygon.OffsetLine == state)
         FLUSH_VERTICES(ctx, 0,
         ctx->NewDriverState |= ST_NEW_RASTERIZER;
   ctx->Polygon.OffsetLine = state;
      case GL_POLYGON_OFFSET_FILL:
      if (ctx->Polygon.OffsetFill == state)
         FLUSH_VERTICES(ctx, 0,
         ctx->NewDriverState |= ST_NEW_RASTERIZER;
   ctx->Polygon.OffsetFill = state;
      case GL_RESCALE_NORMAL_EXT:
      if (ctx->API != API_OPENGL_COMPAT && ctx->API != API_OPENGLES)
         if (ctx->Transform.RescaleNormals == state)
         FLUSH_VERTICES(ctx, _NEW_TRANSFORM | _NEW_FF_VERT_PROGRAM,
         ctx->Transform.RescaleNormals = state;
      case GL_SCISSOR_TEST:
      {
      /* Must expand glEnable to all scissors */
   GLbitfield newEnabled =
         if (newEnabled != ctx->Scissor.EnableFlags) {
      FLUSH_VERTICES(ctx, 0,
         ctx->NewDriverState |= ST_NEW_SCISSOR | ST_NEW_RASTERIZER;
         }
      case GL_STENCIL_TEST:
      if (ctx->Stencil.Enabled == state)
         FLUSH_VERTICES(ctx, 0,
         ctx->NewDriverState |= ST_NEW_DSA;
   ctx->Stencil.Enabled = state;
   _mesa_update_allow_draw_out_of_order(ctx);
      case GL_TEXTURE_1D:
      if (ctx->API != API_OPENGL_COMPAT)
         if (!enable_texture(ctx, state, TEXTURE_1D_BIT)) {
         }
      case GL_TEXTURE_2D:
      if (ctx->API != API_OPENGL_COMPAT && ctx->API != API_OPENGLES)
         if (!enable_texture(ctx, state, TEXTURE_2D_BIT)) {
         }
      case GL_TEXTURE_3D:
      if (ctx->API != API_OPENGL_COMPAT && ctx->API != API_OPENGLES)
         if (!enable_texture(ctx, state, TEXTURE_3D_BIT)) {
         }
      case GL_TEXTURE_GEN_S:
   case GL_TEXTURE_GEN_T:
   case GL_TEXTURE_GEN_R:
   case GL_TEXTURE_GEN_Q:
                                       if (texUnit) {
      GLbitfield coordBit = S_BIT << (cap - GL_TEXTURE_GEN_S);
   GLbitfield newenabled = texUnit->TexGenEnabled & ~coordBit;
   if (state)
         if (texUnit->TexGenEnabled == newenabled)
         FLUSH_VERTICES(ctx, _NEW_TEXTURE_STATE | _NEW_FF_VERT_PROGRAM |
                                 case GL_TEXTURE_GEN_STR_OES:
      /* disable S, T, and R at the same time */
                                    if (texUnit) {
      GLuint newenabled =
         if (state)
         if (texUnit->TexGenEnabled == newenabled)
         FLUSH_VERTICES(ctx, _NEW_TEXTURE_STATE | _NEW_FF_VERT_PROGRAM |
                           /* client-side state */
   case GL_VERTEX_ARRAY:
   case GL_NORMAL_ARRAY:
   case GL_COLOR_ARRAY:
   case GL_TEXTURE_COORD_ARRAY:
      if (ctx->API != API_OPENGL_COMPAT && ctx->API != API_OPENGLES)
         client_state( ctx, ctx->Array.VAO, cap, state );
      case GL_INDEX_ARRAY:
   case GL_EDGE_FLAG_ARRAY:
   case GL_FOG_COORDINATE_ARRAY_EXT:
   case GL_SECONDARY_COLOR_ARRAY_EXT:
      if (ctx->API != API_OPENGL_COMPAT)
         client_state( ctx, ctx->Array.VAO, cap, state );
      case GL_POINT_SIZE_ARRAY_OES:
      if (ctx->API != API_OPENGLES)
                     /* GL_ARB_texture_cube_map */
   case GL_TEXTURE_CUBE_MAP:
      if (ctx->API != API_OPENGL_COMPAT && ctx->API != API_OPENGLES)
         if (!enable_texture(ctx, state, TEXTURE_CUBE_BIT)) {
                     /* GL_EXT_secondary_color */
   case GL_COLOR_SUM_EXT:
      if (ctx->API != API_OPENGL_COMPAT)
         if (ctx->Fog.ColorSumEnabled == state)
         FLUSH_VERTICES(ctx, _NEW_FOG | _NEW_FF_FRAG_PROGRAM,
                     /* GL_ARB_multisample */
   case GL_MULTISAMPLE_ARB:
      if (!_mesa_is_desktop_gl(ctx) && ctx->API != API_OPENGLES)
         _mesa_set_multisample(ctx, state);
      case GL_SAMPLE_ALPHA_TO_COVERAGE_ARB:
      if (ctx->Multisample.SampleAlphaToCoverage == state)
         FLUSH_VERTICES(ctx, 0,
         ctx->NewDriverState |= ST_NEW_BLEND;
   ctx->Multisample.SampleAlphaToCoverage = state;
      case GL_SAMPLE_ALPHA_TO_ONE_ARB:
      if (!_mesa_is_desktop_gl(ctx) && ctx->API != API_OPENGLES)
         if (ctx->Multisample.SampleAlphaToOne == state)
         FLUSH_VERTICES(ctx, 0,
         ctx->NewDriverState |= ST_NEW_BLEND;
   ctx->Multisample.SampleAlphaToOne = state;
      case GL_SAMPLE_COVERAGE_ARB:
      if (ctx->Multisample.SampleCoverage == state)
         FLUSH_VERTICES(ctx, 0,
         ctx->NewDriverState |= ST_NEW_SAMPLE_STATE;
   ctx->Multisample.SampleCoverage = state;
      case GL_SAMPLE_COVERAGE_INVERT_ARB:
      if (!_mesa_is_desktop_gl(ctx))
         if (ctx->Multisample.SampleCoverageInvert == state)
         FLUSH_VERTICES(ctx, 0, GL_MULTISAMPLE_BIT);
   ctx->NewDriverState |= ST_NEW_SAMPLE_STATE;
               /* GL_ARB_sample_shading */
   case GL_SAMPLE_SHADING:
      if (!_mesa_has_ARB_sample_shading(ctx) && !_mesa_is_gles3(ctx))
         if (ctx->Multisample.SampleShading == state)
         FLUSH_VERTICES(ctx, 0,
         ctx->NewDriverState |= ctx->DriverFlags.NewSampleShading;
               /* GL_IBM_rasterpos_clip */
   case GL_RASTER_POSITION_UNCLIPPED_IBM:
      if (ctx->API != API_OPENGL_COMPAT)
         if (ctx->Transform.RasterPositionUnclipped == state)
         FLUSH_VERTICES(ctx, 0, GL_TRANSFORM_BIT | GL_ENABLE_BIT);
               /* GL_ARB_point_sprite */
   case GL_POINT_SPRITE:
      if (!(_mesa_is_desktop_gl_compat(ctx) &&
            !_mesa_has_OES_point_sprite(ctx))
      if (ctx->Point.PointSprite == state)
         FLUSH_VERTICES(ctx, _NEW_POINT | _NEW_FF_VERT_PROGRAM |
                     case GL_VERTEX_PROGRAM_ARB:
      if (!_mesa_has_ARB_vertex_program(ctx))
         if (ctx->VertexProgram.Enabled == state)
         FLUSH_VERTICES(ctx, _NEW_PROGRAM, GL_ENABLE_BIT);
   ctx->VertexProgram.Enabled = state;
   _mesa_update_vertex_processing_mode(ctx);
   _mesa_update_valid_to_render_state(ctx);
      case GL_VERTEX_PROGRAM_POINT_SIZE_ARB:
      /* This was added with ARB_vertex_program, but it is also used with
   * GLSL vertex shaders on desktop.
   */
   if (!_mesa_has_ARB_vertex_program(ctx) &&
      ctx->API != API_OPENGL_CORE)
      if (ctx->VertexProgram.PointSizeEnabled == state)
         FLUSH_VERTICES(ctx, ctx->st->lower_point_size ? _NEW_PROGRAM : 0,
         ctx->NewDriverState |= ST_NEW_RASTERIZER;
   ctx->VertexProgram.PointSizeEnabled = state;
      case GL_VERTEX_PROGRAM_TWO_SIDE_ARB:
      if (!_mesa_has_ARB_vertex_program(ctx))
         if (ctx->VertexProgram.TwoSideEnabled == state)
         FLUSH_VERTICES(ctx, 0, GL_ENABLE_BIT);
   if (ctx->st->lower_two_sided_color) {
      /* TODO: this could be smaller, but most drivers don't get here */
   ctx->NewDriverState |= ST_NEW_VS_STATE |
            }
   ctx->NewDriverState |= ST_NEW_RASTERIZER;
               /* GL_NV_texture_rectangle */
   case GL_TEXTURE_RECTANGLE_NV:
      if (!_mesa_has_NV_texture_rectangle(ctx))
         if (!enable_texture(ctx, state, TEXTURE_RECT_BIT)) {
                     /* GL_EXT_stencil_two_side */
   case GL_STENCIL_TEST_TWO_SIDE_EXT:
      if (!_mesa_has_EXT_stencil_two_side(ctx))
         if (ctx->Stencil.TestTwoSide == state)
         FLUSH_VERTICES(ctx, 0,
         ctx->NewDriverState |= ST_NEW_DSA;
   ctx->Stencil.TestTwoSide = state;
   if (state) {
         } else {
                     case GL_FRAGMENT_PROGRAM_ARB:
      if (!_mesa_has_ARB_fragment_program(ctx))
         if (ctx->FragmentProgram.Enabled == state)
         FLUSH_VERTICES(ctx, _NEW_PROGRAM, GL_ENABLE_BIT);
   ctx->FragmentProgram.Enabled = state;
               /* GL_EXT_depth_bounds_test */
   case GL_DEPTH_BOUNDS_TEST_EXT:
      if (!_mesa_has_EXT_depth_bounds_test(ctx))
         if (ctx->Depth.BoundsTest == state)
         FLUSH_VERTICES(ctx, 0, GL_DEPTH_BUFFER_BIT | GL_ENABLE_BIT);
   ctx->NewDriverState |= ST_NEW_DSA;
               case GL_DEPTH_CLAMP:
      if (!_mesa_has_ARB_depth_clamp(ctx) &&
      !_mesa_has_EXT_depth_clamp(ctx))
      if (ctx->Transform.DepthClampNear == state &&
      ctx->Transform.DepthClampFar == state)
      FLUSH_VERTICES(ctx, 0,
         ctx->NewDriverState |= ST_NEW_RASTERIZER;
   ctx->Transform.DepthClampNear = state;
               case GL_DEPTH_CLAMP_NEAR_AMD:
      if (!_mesa_has_AMD_depth_clamp_separate(ctx))
         if (ctx->Transform.DepthClampNear == state)
         FLUSH_VERTICES(ctx, 0,
         ctx->NewDriverState |= ST_NEW_RASTERIZER;
               case GL_DEPTH_CLAMP_FAR_AMD:
      if (!_mesa_has_AMD_depth_clamp_separate(ctx))
         if (ctx->Transform.DepthClampFar == state)
         FLUSH_VERTICES(ctx, 0,
         ctx->NewDriverState |= ST_NEW_RASTERIZER;
               case GL_FRAGMENT_SHADER_ATI:
   if (!_mesa_has_ATI_fragment_shader(ctx))
         if (ctx->ATIFragmentShader.Enabled == state)
         FLUSH_VERTICES(ctx, _NEW_PROGRAM, GL_ENABLE_BIT);
   ctx->ATIFragmentShader.Enabled = state;
            case GL_TEXTURE_CUBE_MAP_SEAMLESS:
      if (!_mesa_has_ARB_seamless_cube_map(ctx))
         if (ctx->Texture.CubeMapSeamless != state) {
      FLUSH_VERTICES(ctx, _NEW_TEXTURE_OBJECT, 0);
                  case GL_RASTERIZER_DISCARD:
      if (!(_mesa_has_EXT_transform_feedback(ctx) || _mesa_is_gles3(ctx)))
         if (ctx->RasterDiscard != state) {
      FLUSH_VERTICES(ctx, 0, 0);
   ctx->NewDriverState |= ST_NEW_RASTERIZER;
                  case GL_TILE_RASTER_ORDER_FIXED_MESA:
      if (!_mesa_has_MESA_tile_raster_order(ctx))
         if (ctx->TileRasterOrderFixed != state) {
      FLUSH_VERTICES(ctx, 0, GL_ENABLE_BIT);
   ctx->NewDriverState |= ST_NEW_RASTERIZER;
                  case GL_TILE_RASTER_ORDER_INCREASING_X_MESA:
      if (!_mesa_has_MESA_tile_raster_order(ctx))
         if (ctx->TileRasterOrderIncreasingX != state) {
      FLUSH_VERTICES(ctx, 0, GL_ENABLE_BIT);
   ctx->NewDriverState |= ST_NEW_RASTERIZER;
                  case GL_TILE_RASTER_ORDER_INCREASING_Y_MESA:
      if (!_mesa_has_MESA_tile_raster_order(ctx))
         if (ctx->TileRasterOrderIncreasingY != state) {
      FLUSH_VERTICES(ctx, 0, GL_ENABLE_BIT);
   ctx->NewDriverState |= ST_NEW_RASTERIZER;
                  /* GL 3.1 primitive restart.  Note: this enum is different from
   * GL_PRIMITIVE_RESTART_NV (which is client state).
   */
   case GL_PRIMITIVE_RESTART:
      if (!_mesa_is_desktop_gl(ctx) || ctx->Version < 31) {
         }
   if (ctx->Array.PrimitiveRestart != state) {
      ctx->Array.PrimitiveRestart = state;
                  case GL_PRIMITIVE_RESTART_FIXED_INDEX:
      if (!_mesa_is_gles3(ctx) && !_mesa_has_ARB_ES3_compatibility(ctx))
         if (ctx->Array.PrimitiveRestartFixedIndex != state) {
      ctx->Array.PrimitiveRestartFixedIndex = state;
                  /* GL3.0 - GL_framebuffer_sRGB */
   case GL_FRAMEBUFFER_SRGB_EXT:
      if (!_mesa_has_EXT_framebuffer_sRGB(ctx) &&
      !_mesa_has_EXT_sRGB_write_control(ctx))
                  /* GL_OES_EGL_image_external */
   case GL_TEXTURE_EXTERNAL_OES:
      if (!_mesa_has_OES_EGL_image_external(ctx))
         if (!enable_texture(ctx, state, TEXTURE_EXTERNAL_BIT)) {
                     /* ARB_texture_multisample */
   case GL_SAMPLE_MASK:
      if (!_mesa_has_ARB_texture_multisample(ctx) && !_mesa_is_gles31(ctx))
         if (ctx->Multisample.SampleMask == state)
         FLUSH_VERTICES(ctx, 0, 0);
   ctx->NewDriverState |= ST_NEW_SAMPLE_STATE;
               case GL_BLEND_ADVANCED_COHERENT_KHR:
      if (!_mesa_has_KHR_blend_equation_advanced_coherent(ctx))
         if (ctx->Color.BlendCoherent == state)
         FLUSH_VERTICES(ctx, 0, GL_COLOR_BUFFER_BIT);
   ctx->NewDriverState |= ST_NEW_BLEND;
               case GL_BLACKHOLE_RENDER_INTEL:
      if (!_mesa_has_INTEL_blackhole_render(ctx))
         if (ctx->IntelBlackholeRender == state)
         FLUSH_VERTICES(ctx, 0, 0);
   ctx->IntelBlackholeRender = state;
               default:
      }
         invalid_enum_error:
      _mesa_error(ctx, GL_INVALID_ENUM, "gl%s(%s)",
      }
         /**
   * Enable GL capability.  Called by glEnable()
   * \param cap  state to enable.
   */
   void GLAPIENTRY
   _mesa_Enable( GLenum cap )
   {
                  }
         /**
   * Disable GL capability.  Called by glDisable()
   * \param cap  state to disable.
   */
   void GLAPIENTRY
   _mesa_Disable( GLenum cap )
   {
                  }
            /**
   * Enable/disable an indexed state var.
   */
   void
   _mesa_set_enablei(struct gl_context *ctx, GLenum cap,
         {
      assert(state == 0 || state == 1);
   switch (cap) {
   case GL_BLEND:
      if (!ctx->Extensions.EXT_draw_buffers2) {
         }
   if (index >= ctx->Const.MaxDrawBuffers) {
      _mesa_error(ctx, GL_INVALID_VALUE, "%s(index=%u)",
            }
   if (((ctx->Color.BlendEnabled >> index) & 1) != state) {
               if (state)
                        _mesa_flush_vertices_for_blend_adv(ctx, enabled,
         ctx->PopAttribState |= GL_ENABLE_BIT;
   ctx->Color.BlendEnabled = enabled;
   _mesa_update_allow_draw_out_of_order(ctx);
      }
      case GL_SCISSOR_TEST:
      if (index >= ctx->Const.MaxViewports) {
      _mesa_error(ctx, GL_INVALID_VALUE, "%s(index=%u)",
            }
   if (((ctx->Scissor.EnableFlags >> index) & 1) != state) {
      FLUSH_VERTICES(ctx, 0,
         ctx->NewDriverState |= ST_NEW_SCISSOR | ST_NEW_RASTERIZER;
   if (state)
         else
      }
      /* EXT_direct_state_access */
   case GL_TEXTURE_1D:
   case GL_TEXTURE_2D:
   case GL_TEXTURE_3D:
   case GL_TEXTURE_CUBE_MAP:
   case GL_TEXTURE_GEN_S:
   case GL_TEXTURE_GEN_T:
   case GL_TEXTURE_GEN_R:
   case GL_TEXTURE_GEN_Q:
   case GL_TEXTURE_RECTANGLE_ARB: {
      const GLuint curTexUnitSave = ctx->Texture.CurrentUnit;
   if (index >= MAX2(ctx->Const.MaxCombinedTextureImageUnits,
            _mesa_error(ctx, GL_INVALID_VALUE, "%s(index=%u)",
            }
   _mesa_ActiveTexture(GL_TEXTURE0 + index);
   _mesa_set_enable( ctx, cap, state );
   _mesa_ActiveTexture(GL_TEXTURE0 + curTexUnitSave);
      }
   default:
         }
         invalid_enum_error:
      _mesa_error(ctx, GL_INVALID_ENUM, "%s(cap=%s)",
            }
         void GLAPIENTRY
   _mesa_Disablei( GLenum cap, GLuint index )
   {
      GET_CURRENT_CONTEXT(ctx);
      }
         void GLAPIENTRY
   _mesa_Enablei( GLenum cap, GLuint index )
   {
      GET_CURRENT_CONTEXT(ctx);
      }
         GLboolean GLAPIENTRY
   _mesa_IsEnabledi( GLenum cap, GLuint index )
   {
      GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_WITH_RETVAL(ctx, 0);
   switch (cap) {
   case GL_BLEND:
      if (index >= ctx->Const.MaxDrawBuffers) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glIsEnabledIndexed(index=%u)",
            }
      case GL_SCISSOR_TEST:
      if (index >= ctx->Const.MaxViewports) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glIsEnabledIndexed(index=%u)",
            }
      /* EXT_direct_state_access */
   case GL_TEXTURE_1D:
   case GL_TEXTURE_2D:
   case GL_TEXTURE_3D:
   case GL_TEXTURE_CUBE_MAP:
   case GL_TEXTURE_GEN_S:
   case GL_TEXTURE_GEN_T:
   case GL_TEXTURE_GEN_R:
   case GL_TEXTURE_GEN_Q:
   case GL_TEXTURE_RECTANGLE_ARB: {
      GLboolean state;
   const GLuint curTexUnitSave = ctx->Texture.CurrentUnit;
   if (index >= MAX2(ctx->Const.MaxCombinedTextureImageUnits,
            _mesa_error(ctx, GL_INVALID_VALUE, "glIsEnabledIndexed(index=%u)",
            }
   _mesa_ActiveTexture(GL_TEXTURE0 + index);
   state = _mesa_IsEnabled(cap);
   _mesa_ActiveTexture(GL_TEXTURE0 + curTexUnitSave);
      }
   default:
      _mesa_error(ctx, GL_INVALID_ENUM, "glIsEnabledIndexed(cap=%s)",
               }
            /**
   * Helper function to determine whether a texture target is enabled.
   */
   static GLboolean
   is_texture_enabled(struct gl_context *ctx, GLbitfield bit)
   {
      const struct gl_fixedfunc_texture_unit *const texUnit =
            if (!texUnit)
               }
         /**
   * Return simple enable/disable state.
   *
   * \param cap  state variable to query.
   *
   * Returns the state of the specified capability from the current GL context.
   * For the capabilities associated with extensions verifies that those
   * extensions are effectively present before reporting.
   */
   GLboolean GLAPIENTRY
   _mesa_IsEnabled( GLenum cap )
   {
      GET_CURRENT_CONTEXT(ctx);
            switch (cap) {
      case GL_ALPHA_TEST:
      if (ctx->API != API_OPENGL_COMPAT && ctx->API != API_OPENGLES)
            case GL_AUTO_NORMAL:
      if (ctx->API != API_OPENGL_COMPAT)
            case GL_BLEND:
         case GL_CLIP_DISTANCE0: /* aka GL_CLIP_PLANE0 */
   case GL_CLIP_DISTANCE1:
   case GL_CLIP_DISTANCE2:
   case GL_CLIP_DISTANCE3:
   case GL_CLIP_DISTANCE4:
   case GL_CLIP_DISTANCE5:
   case GL_CLIP_DISTANCE6:
   case GL_CLIP_DISTANCE7: {
                                 }
   case GL_COLOR_MATERIAL:
      if (ctx->API != API_OPENGL_COMPAT && ctx->API != API_OPENGLES)
            case GL_CULL_FACE:
         case GL_DEBUG_OUTPUT:
   case GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB:
         case GL_DEPTH_TEST:
         case GL_DITHER:
         case GL_FOG:
      if (ctx->API != API_OPENGL_COMPAT && ctx->API != API_OPENGLES)
            case GL_LIGHTING:
      if (ctx->API != API_OPENGL_COMPAT && ctx->API != API_OPENGLES)
            case GL_LIGHT0:
   case GL_LIGHT1:
   case GL_LIGHT2:
   case GL_LIGHT3:
   case GL_LIGHT4:
   case GL_LIGHT5:
   case GL_LIGHT6:
   case GL_LIGHT7:
      if (ctx->API != API_OPENGL_COMPAT && ctx->API != API_OPENGLES)
            case GL_LINE_SMOOTH:
      if (!_mesa_is_desktop_gl(ctx) && ctx->API != API_OPENGLES)
            case GL_LINE_STIPPLE:
      if (ctx->API != API_OPENGL_COMPAT)
            case GL_INDEX_LOGIC_OP:
      if (ctx->API != API_OPENGL_COMPAT)
            case GL_COLOR_LOGIC_OP:
      if (!_mesa_is_desktop_gl(ctx) && ctx->API != API_OPENGLES)
            case GL_MAP1_COLOR_4:
      if (ctx->API != API_OPENGL_COMPAT)
            case GL_MAP1_INDEX:
      if (ctx->API != API_OPENGL_COMPAT)
            case GL_MAP1_NORMAL:
      if (ctx->API != API_OPENGL_COMPAT)
            case GL_MAP1_TEXTURE_COORD_1:
      if (ctx->API != API_OPENGL_COMPAT)
            case GL_MAP1_TEXTURE_COORD_2:
      if (ctx->API != API_OPENGL_COMPAT)
            case GL_MAP1_TEXTURE_COORD_3:
      if (ctx->API != API_OPENGL_COMPAT)
            case GL_MAP1_TEXTURE_COORD_4:
      if (ctx->API != API_OPENGL_COMPAT)
            case GL_MAP1_VERTEX_3:
      if (ctx->API != API_OPENGL_COMPAT)
            case GL_MAP1_VERTEX_4:
      if (ctx->API != API_OPENGL_COMPAT)
            case GL_MAP2_COLOR_4:
      if (ctx->API != API_OPENGL_COMPAT)
            case GL_MAP2_INDEX:
      if (ctx->API != API_OPENGL_COMPAT)
            case GL_MAP2_NORMAL:
      if (ctx->API != API_OPENGL_COMPAT)
            case GL_MAP2_TEXTURE_COORD_1:
      if (ctx->API != API_OPENGL_COMPAT)
            case GL_MAP2_TEXTURE_COORD_2:
      if (ctx->API != API_OPENGL_COMPAT)
            case GL_MAP2_TEXTURE_COORD_3:
      if (ctx->API != API_OPENGL_COMPAT)
            case GL_MAP2_TEXTURE_COORD_4:
      if (ctx->API != API_OPENGL_COMPAT)
            case GL_MAP2_VERTEX_3:
      if (ctx->API != API_OPENGL_COMPAT)
            case GL_MAP2_VERTEX_4:
      if (ctx->API != API_OPENGL_COMPAT)
            case GL_NORMALIZE:
      if (ctx->API != API_OPENGL_COMPAT && ctx->API != API_OPENGLES)
            case GL_POINT_SMOOTH:
      if (ctx->API != API_OPENGL_COMPAT && ctx->API != API_OPENGLES)
            case GL_POLYGON_SMOOTH:
      if (!_mesa_is_desktop_gl(ctx))
            case GL_POLYGON_STIPPLE:
      if (ctx->API != API_OPENGL_COMPAT)
            case GL_POLYGON_OFFSET_POINT:
      if (!_mesa_is_desktop_gl(ctx))
            case GL_POLYGON_OFFSET_LINE:
      if (!_mesa_is_desktop_gl(ctx))
            case GL_POLYGON_OFFSET_FILL:
         case GL_RESCALE_NORMAL_EXT:
      if (ctx->API != API_OPENGL_COMPAT && ctx->API != API_OPENGLES)
            case GL_SCISSOR_TEST:
         case GL_STENCIL_TEST:
         case GL_TEXTURE_1D:
      if (ctx->API != API_OPENGL_COMPAT)
            case GL_TEXTURE_2D:
      if (ctx->API != API_OPENGL_COMPAT && ctx->API != API_OPENGLES)
            case GL_TEXTURE_3D:
      if (ctx->API != API_OPENGL_COMPAT && ctx->API != API_OPENGLES)
            case GL_TEXTURE_GEN_S:
   case GL_TEXTURE_GEN_T:
   case GL_TEXTURE_GEN_R:
   case GL_TEXTURE_GEN_Q:
      {
                                    if (texUnit) {
      GLbitfield coordBit = S_BIT << (cap - GL_TEXTURE_GEN_S);
         }
      case GL_TEXTURE_GEN_STR_OES:
      {
                                    if (texUnit) {
                                 /* client-side state */
   case GL_VERTEX_ARRAY:
      if (ctx->API != API_OPENGL_COMPAT && ctx->API != API_OPENGLES)
            case GL_NORMAL_ARRAY:
      if (ctx->API != API_OPENGL_COMPAT && ctx->API != API_OPENGLES)
            case GL_COLOR_ARRAY:
      if (ctx->API != API_OPENGL_COMPAT && ctx->API != API_OPENGLES)
            case GL_INDEX_ARRAY:
      if (ctx->API != API_OPENGL_COMPAT)
            case GL_TEXTURE_COORD_ARRAY:
      if (ctx->API != API_OPENGL_COMPAT && ctx->API != API_OPENGLES)
         return !!(ctx->Array.VAO->Enabled &
      case GL_EDGE_FLAG_ARRAY:
      if (ctx->API != API_OPENGL_COMPAT)
            case GL_FOG_COORDINATE_ARRAY_EXT:
      if (ctx->API != API_OPENGL_COMPAT)
            case GL_SECONDARY_COLOR_ARRAY_EXT:
      if (ctx->API != API_OPENGL_COMPAT)
            case GL_POINT_SIZE_ARRAY_OES:
      if (ctx->API != API_OPENGLES)
               /* GL_ARB_texture_cube_map */
   case GL_TEXTURE_CUBE_MAP:
      if (ctx->API != API_OPENGL_COMPAT && ctx->API != API_OPENGLES)
               /* GL_EXT_secondary_color */
   case GL_COLOR_SUM_EXT:
      if (ctx->API != API_OPENGL_COMPAT)
               /* GL_ARB_multisample */
   case GL_MULTISAMPLE_ARB:
      if (!_mesa_is_desktop_gl(ctx) && ctx->API != API_OPENGLES)
            case GL_SAMPLE_ALPHA_TO_COVERAGE_ARB:
         case GL_SAMPLE_ALPHA_TO_ONE_ARB:
      if (!_mesa_is_desktop_gl(ctx) && ctx->API != API_OPENGLES)
            case GL_SAMPLE_COVERAGE_ARB:
         case GL_SAMPLE_COVERAGE_INVERT_ARB:
      if (!_mesa_is_desktop_gl(ctx))
               /* GL_IBM_rasterpos_clip */
   case GL_RASTER_POSITION_UNCLIPPED_IBM:
      if (ctx->API != API_OPENGL_COMPAT)
               /* GL_ARB_point_sprite */
   case GL_POINT_SPRITE:
      if (!(_mesa_is_desktop_gl_compat(ctx) &&
            !_mesa_has_OES_point_sprite(ctx))
            case GL_VERTEX_PROGRAM_ARB:
      if (!_mesa_has_ARB_vertex_program(ctx))
            case GL_VERTEX_PROGRAM_POINT_SIZE_ARB:
      /* This was added with ARB_vertex_program, but it is also used with
   * GLSL vertex shaders on desktop.
   */
   if (!_mesa_has_ARB_vertex_program(ctx) &&
      ctx->API != API_OPENGL_CORE)
         case GL_VERTEX_PROGRAM_TWO_SIDE_ARB:
      if (!_mesa_has_ARB_vertex_program(ctx))
               /* GL_NV_texture_rectangle */
   case GL_TEXTURE_RECTANGLE_NV:
      if (!_mesa_has_NV_texture_rectangle(ctx))
               /* GL_EXT_stencil_two_side */
   case GL_STENCIL_TEST_TWO_SIDE_EXT:
      if (!_mesa_has_EXT_stencil_two_side(ctx))
               case GL_FRAGMENT_PROGRAM_ARB:
      if (!_mesa_has_ARB_fragment_program(ctx))
               /* GL_EXT_depth_bounds_test */
   case GL_DEPTH_BOUNDS_TEST_EXT:
      if (!_mesa_has_EXT_depth_bounds_test(ctx))
               /* GL_ARB_depth_clamp */
   case GL_DEPTH_CLAMP:
      if (!_mesa_has_ARB_depth_clamp(ctx) &&
      !_mesa_has_EXT_depth_clamp(ctx))
                  case GL_DEPTH_CLAMP_NEAR_AMD:
      if (!_mesa_has_AMD_depth_clamp_separate(ctx))
               case GL_DEPTH_CLAMP_FAR_AMD:
      if (!_mesa_has_AMD_depth_clamp_separate(ctx))
               case GL_FRAGMENT_SHADER_ATI:
      if (!_mesa_has_ATI_fragment_shader(ctx))
               case GL_TEXTURE_CUBE_MAP_SEAMLESS:
      if (!_mesa_has_ARB_seamless_cube_map(ctx))
               case GL_RASTERIZER_DISCARD:
      if (!(_mesa_has_EXT_transform_feedback(ctx) || _mesa_is_gles3(ctx)))
               /* GL_NV_primitive_restart */
   case GL_PRIMITIVE_RESTART_NV:
      if (!_mesa_has_NV_primitive_restart(ctx))
               /* GL 3.1 primitive restart */
   case GL_PRIMITIVE_RESTART:
      if (!_mesa_is_desktop_gl(ctx) || ctx->Version < 31) {
                     case GL_PRIMITIVE_RESTART_FIXED_INDEX:
      if (!_mesa_is_gles3(ctx) && !_mesa_has_ARB_ES3_compatibility(ctx))
               /* GL3.0 - GL_framebuffer_sRGB */
   case GL_FRAMEBUFFER_SRGB_EXT:
      if (!_mesa_has_EXT_framebuffer_sRGB(ctx) &&
      !_mesa_has_EXT_sRGB_write_control(ctx))
            /* GL_OES_EGL_image_external */
   case GL_TEXTURE_EXTERNAL_OES:
      if (!_mesa_has_OES_EGL_image_external(ctx))
               /* ARB_texture_multisample */
   case GL_SAMPLE_MASK:
      if (!_mesa_has_ARB_texture_multisample(ctx) && !_mesa_is_gles31(ctx))
               /* ARB_sample_shading */
   case GL_SAMPLE_SHADING:
      if (!_mesa_has_ARB_sample_shading(ctx) && !_mesa_is_gles3(ctx))
               case GL_BLEND_ADVANCED_COHERENT_KHR:
      if (!_mesa_has_KHR_blend_equation_advanced_coherent(ctx))
               case GL_CONSERVATIVE_RASTERIZATION_INTEL:
      if (!_mesa_has_INTEL_conservative_rasterization(ctx))
               case GL_CONSERVATIVE_RASTERIZATION_NV:
      if (!_mesa_has_NV_conservative_raster(ctx))
               case GL_TILE_RASTER_ORDER_FIXED_MESA:
      if (!_mesa_has_MESA_tile_raster_order(ctx))
               case GL_TILE_RASTER_ORDER_INCREASING_X_MESA:
      if (!_mesa_has_MESA_tile_raster_order(ctx))
               case GL_TILE_RASTER_ORDER_INCREASING_Y_MESA:
      if (!_mesa_has_MESA_tile_raster_order(ctx))
               case GL_BLACKHOLE_RENDER_INTEL:
      if (!_mesa_has_INTEL_blackhole_render(ctx))
               default:
                     invalid_enum_error:
      _mesa_error(ctx, GL_INVALID_ENUM, "glIsEnabled(%s)",
            }
