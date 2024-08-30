   /*
   * Mesa 3-D graphics library
   *
   * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
   * Copyright (C) 2009  VMware, Inc.   All Rights Reserved.
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
      #include "accum.h"
   #include "arrayobj.h"
   #include "attrib.h"
   #include "blend.h"
   #include "buffers.h"
   #include "bufferobj.h"
   #include "context.h"
   #include "depth.h"
   #include "enable.h"
   #include "enums.h"
   #include "fog.h"
   #include "hint.h"
   #include "light.h"
   #include "lines.h"
   #include "macros.h"
   #include "matrix.h"
   #include "multisample.h"
   #include "pixelstore.h"
   #include "points.h"
   #include "polygon.h"
   #include "shared.h"
   #include "scissor.h"
   #include "stencil.h"
   #include "texobj.h"
   #include "texparam.h"
   #include "texstate.h"
   #include "varray.h"
   #include "viewport.h"
   #include "mtypes.h"
   #include "state.h"
   #include "hash.h"
   #include <stdbool.h>
   #include "util/u_memory.h"
   #include "api_exec_decl.h"
      #include "state_tracker/st_cb_texture.h"
   #include "state_tracker/st_manager.h"
   #include "state_tracker/st_sampler_view.h"
      static inline bool
   copy_texture_attribs(struct gl_texture_object *dst,
               {
      /* All pushed fields have no effect on texture buffers. */
   if (tex == TEXTURE_BUFFER_INDEX)
            /* Sampler fields have no effect on MSAA textures. */
   if (tex != TEXTURE_2D_MULTISAMPLE_INDEX &&
      tex != TEXTURE_2D_MULTISAMPLE_ARRAY_INDEX) {
   memcpy(&dst->Sampler.Attrib, &src->Sampler.Attrib,
      }
   memcpy(&dst->Attrib, &src->Attrib, sizeof(src->Attrib));
      }
         void GLAPIENTRY
   _mesa_PushAttrib(GLbitfield mask)
   {
                        if (MESA_VERBOSE & VERBOSE_API)
            if (ctx->AttribStackDepth >= MAX_ATTRIB_STACK_DEPTH) {
      _mesa_error(ctx, GL_STACK_OVERFLOW, "glPushAttrib");
               head = ctx->AttribStack[ctx->AttribStackDepth];
   if (unlikely(!head)) {
      head = CALLOC_STRUCT(gl_attrib_node);
   if (unlikely(!head)) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glPushAttrib");
      }
               head->Mask = mask;
            if (mask & GL_ACCUM_BUFFER_BIT)
            if (mask & GL_COLOR_BUFFER_BIT) {
      memcpy(&head->Color, &ctx->Color, sizeof(struct gl_colorbuffer_attrib));
   /* push the Draw FBO's DrawBuffer[] state, not ctx->Color.DrawBuffer[] */
   for (unsigned i = 0; i < ctx->Const.MaxDrawBuffers; i ++)
               if (mask & GL_CURRENT_BIT) {
      FLUSH_CURRENT(ctx, 0);
               if (mask & GL_DEPTH_BUFFER_BIT)
            if (mask & GL_ENABLE_BIT) {
      struct gl_enable_attrib_node *attr = &head->Enable;
            /* Copy enable flags from all other attributes into the enable struct. */
   attr->AlphaTest = ctx->Color.AlphaEnabled;
   attr->AutoNormal = ctx->Eval.AutoNormal;
   attr->Blend = ctx->Color.BlendEnabled;
   attr->ClipPlanes = ctx->Transform.ClipPlanesEnabled;
   attr->ColorMaterial = ctx->Light.ColorMaterialEnabled;
   attr->CullFace = ctx->Polygon.CullFlag;
   attr->DepthClampNear = ctx->Transform.DepthClampNear;
   attr->DepthClampFar = ctx->Transform.DepthClampFar;
   attr->DepthTest = ctx->Depth.Test;
   attr->Dither = ctx->Color.DitherFlag;
   attr->Fog = ctx->Fog.Enabled;
   for (i = 0; i < ctx->Const.MaxLights; i++) {
         }
   attr->Lighting = ctx->Light.Enabled;
   attr->LineSmooth = ctx->Line.SmoothFlag;
   attr->LineStipple = ctx->Line.StippleFlag;
   attr->IndexLogicOp = ctx->Color.IndexLogicOpEnabled;
   attr->ColorLogicOp = ctx->Color.ColorLogicOpEnabled;
   attr->Map1Color4 = ctx->Eval.Map1Color4;
   attr->Map1Index = ctx->Eval.Map1Index;
   attr->Map1Normal = ctx->Eval.Map1Normal;
   attr->Map1TextureCoord1 = ctx->Eval.Map1TextureCoord1;
   attr->Map1TextureCoord2 = ctx->Eval.Map1TextureCoord2;
   attr->Map1TextureCoord3 = ctx->Eval.Map1TextureCoord3;
   attr->Map1TextureCoord4 = ctx->Eval.Map1TextureCoord4;
   attr->Map1Vertex3 = ctx->Eval.Map1Vertex3;
   attr->Map1Vertex4 = ctx->Eval.Map1Vertex4;
   attr->Map2Color4 = ctx->Eval.Map2Color4;
   attr->Map2Index = ctx->Eval.Map2Index;
   attr->Map2Normal = ctx->Eval.Map2Normal;
   attr->Map2TextureCoord1 = ctx->Eval.Map2TextureCoord1;
   attr->Map2TextureCoord2 = ctx->Eval.Map2TextureCoord2;
   attr->Map2TextureCoord3 = ctx->Eval.Map2TextureCoord3;
   attr->Map2TextureCoord4 = ctx->Eval.Map2TextureCoord4;
   attr->Map2Vertex3 = ctx->Eval.Map2Vertex3;
   attr->Map2Vertex4 = ctx->Eval.Map2Vertex4;
   attr->Normalize = ctx->Transform.Normalize;
   attr->RasterPositionUnclipped = ctx->Transform.RasterPositionUnclipped;
   attr->PointSmooth = ctx->Point.SmoothFlag;
   attr->PointSprite = ctx->Point.PointSprite;
   attr->PolygonOffsetPoint = ctx->Polygon.OffsetPoint;
   attr->PolygonOffsetLine = ctx->Polygon.OffsetLine;
   attr->PolygonOffsetFill = ctx->Polygon.OffsetFill;
   attr->PolygonSmooth = ctx->Polygon.SmoothFlag;
   attr->PolygonStipple = ctx->Polygon.StippleFlag;
   attr->RescaleNormals = ctx->Transform.RescaleNormals;
   attr->Scissor = ctx->Scissor.EnableFlags;
   attr->Stencil = ctx->Stencil.Enabled;
   attr->StencilTwoSide = ctx->Stencil.TestTwoSide;
   attr->MultisampleEnabled = ctx->Multisample.Enabled;
   attr->SampleAlphaToCoverage = ctx->Multisample.SampleAlphaToCoverage;
   attr->SampleAlphaToOne = ctx->Multisample.SampleAlphaToOne;
   attr->SampleCoverage = ctx->Multisample.SampleCoverage;
   for (i = 0; i < ctx->Const.MaxTextureUnits; i++) {
      attr->Texture[i] = ctx->Texture.FixedFuncUnit[i].Enabled;
      }
   /* GL_ARB_vertex_program */
   attr->VertexProgram = ctx->VertexProgram.Enabled;
   attr->VertexProgramPointSize = ctx->VertexProgram.PointSizeEnabled;
            /* GL_ARB_fragment_program */
            /* GL_ARB_framebuffer_sRGB / GL_EXT_framebuffer_sRGB */
            /* GL_NV_conservative_raster */
               if (mask & GL_EVAL_BIT)
            if (mask & GL_FOG_BIT)
            if (mask & GL_HINT_BIT)
            if (mask & GL_LIGHTING_BIT) {
      FLUSH_CURRENT(ctx, 0);   /* flush material changes */
               if (mask & GL_LINE_BIT)
            if (mask & GL_LIST_BIT)
            if (mask & GL_PIXEL_MODE_BIT) {
      memcpy(&head->Pixel, &ctx->Pixel, sizeof(struct gl_pixel_attrib));
   /* push the Read FBO's ReadBuffer state, not ctx->Pixel.ReadBuffer */
               if (mask & GL_POINT_BIT)
            if (mask & GL_POLYGON_BIT)
            if (mask & GL_POLYGON_STIPPLE_BIT) {
      memcpy(&head->PolygonStipple, &ctx->PolygonStipple,
               if (mask & GL_SCISSOR_BIT)
            if (mask & GL_STENCIL_BUFFER_BIT)
            if (mask & GL_TEXTURE_BIT) {
                        /* copy/save the bulk of texture state here */
   head->Texture.CurrentUnit = ctx->Texture.CurrentUnit;
   memcpy(&head->Texture.FixedFuncUnit, &ctx->Texture.FixedFuncUnit,
            /* Copy/save contents of default texture objects. They are almost
   * always bound, so this can be done unconditionally.
   *
   * We save them separately, so that we don't have to save them in every
   * texture unit where they are bound. This decreases CPU overhead.
   */
   for (tex = 0; tex < NUM_TEXTURE_TARGETS; tex++) {
                                 /* copy state/contents of the currently bound texture objects */
   unsigned num_tex_used = ctx->Texture.NumCurrentTexUsed;
   for (u = 0; u < num_tex_used; u++) {
                     for (tex = 0; tex < NUM_TEXTURE_TARGETS; tex++) {
                              /* Default texture targets are saved separately above. */
   if (src->Name != 0)
         }
   head->Texture.NumTexSaved = num_tex_used;
               if (mask & GL_TRANSFORM_BIT)
            if (mask & GL_VIEWPORT_BIT) {
      memcpy(&head->Viewport.ViewportArray, &ctx->ViewportArray,
            head->Viewport.SubpixelPrecisionBias[0] = ctx->SubpixelPrecisionBias[0];
               /* GL_ARB_multisample */
   if (mask & GL_MULTISAMPLE_BIT_ARB)
            ctx->AttribStackDepth++;
      }
         #define TEST_AND_UPDATE(VALUE, NEWVALUE, ENUM) do {  \
         if ((VALUE) != (NEWVALUE))                     \
            #define TEST_AND_UPDATE_BIT(VALUE, NEW_VALUE, BIT, ENUM) do {                 \
         if (((VALUE) & BITFIELD_BIT(BIT)) != ((NEW_VALUE) & BITFIELD_BIT(BIT))) \
            #define TEST_AND_UPDATE_INDEX(VALUE, NEW_VALUE, INDEX, ENUM) do {                 \
         if (((VALUE) & BITFIELD_BIT(INDEX)) != ((NEW_VALUE) & BITFIELD_BIT(INDEX))) \
               static void
   pop_enable_group(struct gl_context *ctx, const struct gl_enable_attrib_node *enable)
   {
               TEST_AND_UPDATE(ctx->Color.AlphaEnabled, enable->AlphaTest, GL_ALPHA_TEST);
   if (ctx->Color.BlendEnabled != enable->Blend) {
      if (ctx->Extensions.EXT_draw_buffers2) {
      for (unsigned i = 0; i < ctx->Const.MaxDrawBuffers; i++) {
      TEST_AND_UPDATE_INDEX(ctx->Color.BlendEnabled, enable->Blend,
         } else {
                     if (ctx->Transform.ClipPlanesEnabled != enable->ClipPlanes) {
      for (unsigned i = 0; i < ctx->Const.MaxClipPlanes; i++) {
      TEST_AND_UPDATE_BIT(ctx->Transform.ClipPlanesEnabled,
                  TEST_AND_UPDATE(ctx->Light.ColorMaterialEnabled, enable->ColorMaterial,
                  if (!ctx->Extensions.AMD_depth_clamp_separate) {
      TEST_AND_UPDATE(ctx->Transform.DepthClampNear && ctx->Transform.DepthClampFar,
            } else {
      TEST_AND_UPDATE(ctx->Transform.DepthClampNear, enable->DepthClampNear,
         TEST_AND_UPDATE(ctx->Transform.DepthClampFar, enable->DepthClampFar,
               TEST_AND_UPDATE(ctx->Depth.Test, enable->DepthTest, GL_DEPTH_TEST);
   TEST_AND_UPDATE(ctx->Color.DitherFlag, enable->Dither, GL_DITHER);
   TEST_AND_UPDATE(ctx->Fog.Enabled, enable->Fog, GL_FOG);
   TEST_AND_UPDATE(ctx->Light.Enabled, enable->Lighting, GL_LIGHTING);
   TEST_AND_UPDATE(ctx->Line.SmoothFlag, enable->LineSmooth, GL_LINE_SMOOTH);
   TEST_AND_UPDATE(ctx->Line.StippleFlag, enable->LineStipple,
         TEST_AND_UPDATE(ctx->Color.IndexLogicOpEnabled, enable->IndexLogicOp,
         TEST_AND_UPDATE(ctx->Color.ColorLogicOpEnabled, enable->ColorLogicOp,
            TEST_AND_UPDATE(ctx->Eval.Map1Color4, enable->Map1Color4, GL_MAP1_COLOR_4);
   TEST_AND_UPDATE(ctx->Eval.Map1Index, enable->Map1Index, GL_MAP1_INDEX);
   TEST_AND_UPDATE(ctx->Eval.Map1Normal, enable->Map1Normal, GL_MAP1_NORMAL);
   TEST_AND_UPDATE(ctx->Eval.Map1TextureCoord1, enable->Map1TextureCoord1,
         TEST_AND_UPDATE(ctx->Eval.Map1TextureCoord2, enable->Map1TextureCoord2,
         TEST_AND_UPDATE(ctx->Eval.Map1TextureCoord3, enable->Map1TextureCoord3,
         TEST_AND_UPDATE(ctx->Eval.Map1TextureCoord4, enable->Map1TextureCoord4,
         TEST_AND_UPDATE(ctx->Eval.Map1Vertex3, enable->Map1Vertex3,
         TEST_AND_UPDATE(ctx->Eval.Map1Vertex4, enable->Map1Vertex4,
            TEST_AND_UPDATE(ctx->Eval.Map2Color4, enable->Map2Color4, GL_MAP2_COLOR_4);
   TEST_AND_UPDATE(ctx->Eval.Map2Index, enable->Map2Index, GL_MAP2_INDEX);
   TEST_AND_UPDATE(ctx->Eval.Map2Normal, enable->Map2Normal, GL_MAP2_NORMAL);
   TEST_AND_UPDATE(ctx->Eval.Map2TextureCoord1, enable->Map2TextureCoord1,
         TEST_AND_UPDATE(ctx->Eval.Map2TextureCoord2, enable->Map2TextureCoord2,
         TEST_AND_UPDATE(ctx->Eval.Map2TextureCoord3, enable->Map2TextureCoord3,
         TEST_AND_UPDATE(ctx->Eval.Map2TextureCoord4, enable->Map2TextureCoord4,
         TEST_AND_UPDATE(ctx->Eval.Map2Vertex3, enable->Map2Vertex3,
         TEST_AND_UPDATE(ctx->Eval.Map2Vertex4, enable->Map2Vertex4,
            TEST_AND_UPDATE(ctx->Eval.AutoNormal, enable->AutoNormal, GL_AUTO_NORMAL);
   TEST_AND_UPDATE(ctx->Transform.Normalize, enable->Normalize, GL_NORMALIZE);
   TEST_AND_UPDATE(ctx->Transform.RescaleNormals, enable->RescaleNormals,
         TEST_AND_UPDATE(ctx->Transform.RasterPositionUnclipped,
               TEST_AND_UPDATE(ctx->Point.SmoothFlag, enable->PointSmooth,
         TEST_AND_UPDATE(ctx->Point.PointSprite, enable->PointSprite,
         TEST_AND_UPDATE(ctx->Polygon.OffsetPoint, enable->PolygonOffsetPoint,
         TEST_AND_UPDATE(ctx->Polygon.OffsetLine, enable->PolygonOffsetLine,
         TEST_AND_UPDATE(ctx->Polygon.OffsetFill, enable->PolygonOffsetFill,
         TEST_AND_UPDATE(ctx->Polygon.SmoothFlag, enable->PolygonSmooth,
         TEST_AND_UPDATE(ctx->Polygon.StippleFlag, enable->PolygonStipple,
         if (ctx->Scissor.EnableFlags != enable->Scissor) {
               for (i = 0; i < ctx->Const.MaxViewports; i++) {
      TEST_AND_UPDATE_INDEX(ctx->Scissor.EnableFlags, enable->Scissor,
         }
   TEST_AND_UPDATE(ctx->Stencil.Enabled, enable->Stencil, GL_STENCIL_TEST);
   if (ctx->Extensions.EXT_stencil_two_side) {
      TEST_AND_UPDATE(ctx->Stencil.TestTwoSide, enable->StencilTwoSide,
      }
   TEST_AND_UPDATE(ctx->Multisample.Enabled, enable->MultisampleEnabled,
         TEST_AND_UPDATE(ctx->Multisample.SampleAlphaToCoverage,
               TEST_AND_UPDATE(ctx->Multisample.SampleAlphaToOne,
               TEST_AND_UPDATE(ctx->Multisample.SampleCoverage,
               /* GL_ARB_vertex_program */
   TEST_AND_UPDATE(ctx->VertexProgram.Enabled,
               TEST_AND_UPDATE(ctx->VertexProgram.PointSizeEnabled,
               TEST_AND_UPDATE(ctx->VertexProgram.TwoSideEnabled,
                  /* GL_ARB_fragment_program */
   TEST_AND_UPDATE(ctx->FragmentProgram.Enabled,
                  /* GL_ARB_framebuffer_sRGB / GL_EXT_framebuffer_sRGB */
   TEST_AND_UPDATE(ctx->Color.sRGBEnabled, enable->sRGBEnabled,
            /* GL_NV_conservative_raster */
   if (ctx->Extensions.NV_conservative_raster) {
      TEST_AND_UPDATE(ctx->ConservativeRasterization,
                              /* texture unit enables */
   for (i = 0; i < ctx->Const.MaxTextureUnits; i++) {
      const GLbitfield enabled = enable->Texture[i];
   const GLbitfield gen_enabled = enable->TexGen[i];
   const struct gl_fixedfunc_texture_unit *unit = &ctx->Texture.FixedFuncUnit[i];
   const GLbitfield old_enabled = unit->Enabled;
            if (old_enabled == enabled && old_gen_enabled == gen_enabled)
                     if (old_enabled != enabled) {
      TEST_AND_UPDATE_BIT(old_enabled, enabled, TEXTURE_1D_INDEX, GL_TEXTURE_1D);
   TEST_AND_UPDATE_BIT(old_enabled, enabled, TEXTURE_2D_INDEX, GL_TEXTURE_2D);
   TEST_AND_UPDATE_BIT(old_enabled, enabled, TEXTURE_3D_INDEX, GL_TEXTURE_3D);
   if (ctx->Extensions.NV_texture_rectangle) {
      TEST_AND_UPDATE_BIT(old_enabled, enabled, TEXTURE_RECT_INDEX,
      }
   TEST_AND_UPDATE_BIT(old_enabled, enabled, TEXTURE_CUBE_INDEX,
               if (old_gen_enabled != gen_enabled) {
      TEST_AND_UPDATE_BIT(old_gen_enabled, gen_enabled, 0, GL_TEXTURE_GEN_S);
   TEST_AND_UPDATE_BIT(old_gen_enabled, gen_enabled, 1, GL_TEXTURE_GEN_T);
   TEST_AND_UPDATE_BIT(old_gen_enabled, gen_enabled, 2, GL_TEXTURE_GEN_R);
                     }
         /**
   * Pop/restore texture attribute/group state.
   */
   static void
   pop_texture_group(struct gl_context *ctx, struct gl_texture_attrib_node *texstate)
   {
                        /* Restore fixed-function texture unit states. */
   for (u = 0; u < ctx->Const.MaxTextureUnits; u++) {
      const struct gl_fixedfunc_texture_unit *unit =
         struct gl_fixedfunc_texture_unit *destUnit =
                     /* Fast path for other drivers. */
   memcpy(destUnit, unit, sizeof(*unit));
   destUnit->_CurrentCombine = NULL;
   ctx->Texture.Unit[u].LodBias = texstate->LodBias[u];
               /* Restore saved textures. */
   unsigned num_tex_saved = texstate->NumTexSaved;
   for (u = 0; u < num_tex_saved; u++) {
                        /* Restore texture object state for each target */
   for (tgt = 0; tgt < NUM_TEXTURE_TARGETS; tgt++) {
      const struct gl_texture_object *savedObj = &texstate->SavedObj[u][tgt];
   struct gl_texture_object *texObj =
                        /* According to the OpenGL 4.6 Compatibility Profile specification,
   * table 23.17, GL_TEXTURE_BINDING_2D_MULTISAMPLE and
   * GL_TEXTURE_BINDING_2D_MULTISAMPLE_ARRAY do not belong in the
   * texture attrib group.
   */
   if (!is_msaa && texObj->Name != savedObj->Name) {
      /* We don't need to check whether the texture target is supported,
   * because we wouldn't get in this conditional block if it wasn't.
   */
   _mesa_BindTexture_no_error(texObj->Target, savedObj->Name);
               /* Default texture object states are restored separately below. */
                  /* But in the MSAA case, where the currently-bound object is not the
   * default state, we should still restore the saved default object's
   * data when that's what was saved initially.
   */
                                                /* Restore textures in units that were not used before glPushAttrib (thus
   * they were not saved) but were used after glPushAttrib. Revert
   * the bindings to Name = 0.
   */
   unsigned num_tex_changed = ctx->Texture.NumCurrentTexUsed;
   for (u = num_tex_saved; u < num_tex_changed; u++) {
               for (gl_texture_index tgt = 0; tgt < NUM_TEXTURE_TARGETS; tgt++) {
      struct gl_texture_object *texObj =
                        /* According to the OpenGL 4.6 Compatibility Profile specification,
   * table 23.17, GL_TEXTURE_BINDING_2D_MULTISAMPLE and
   * GL_TEXTURE_BINDING_2D_MULTISAMPLE_ARRAY do not belong in the
   * texture attrib group.
   */
   if (!is_msaa && texObj->Name != 0) {
      /* We don't need to check whether the texture target is supported,
   * because we wouldn't get in this conditional block if it wasn't.
   */
                     /* Restore default texture object states. */
   for (gl_texture_index tex = 0; tex < NUM_TEXTURE_TARGETS; tex++) {
      struct gl_texture_object *dst = ctx->Shared->DefaultTex[tex];
                        _mesa_ActiveTexture(GL_TEXTURE0_ARB + texstate->CurrentUnit);
      }
         #define TEST_AND_CALL1(FIELD, CALL) do { \
         if (ctx->FIELD != attr->FIELD)     \
            #define TEST_AND_CALL1_SEL(FIELD, CALL, SEL) do { \
         if (ctx->FIELD != attr->FIELD)              \
            #define TEST_AND_CALL2(FIELD1, FIELD2, CALL) do {                     \
         if (ctx->FIELD1 != attr->FIELD1 || ctx->FIELD2 != attr->FIELD2) \
               /*
   * This function is kind of long just because we have to call a lot
   * of device driver functions to update device driver state.
   *
   * XXX As it is now, most of the pop-code calls immediate-mode Mesa functions
   * in order to restore GL state.  This isn't terribly efficient but it
   * ensures that dirty flags and any derived state gets updated correctly.
   * We could at least check if the value to restore equals the current value
   * and then skip the Mesa call.
   */
   void GLAPIENTRY
   _mesa_PopAttrib(void)
   {
      struct gl_attrib_node *attr;
   GET_CURRENT_CONTEXT(ctx);
            if (ctx->AttribStackDepth == 0) {
      _mesa_error(ctx, GL_STACK_UNDERFLOW, "glPopAttrib");
               ctx->AttribStackDepth--;
                     /* Flush current attribs. This must be done before PopAttribState is
   * applied.
   */
   if (mask & GL_CURRENT_BIT)
            /* Only restore states that have been changed since glPushAttrib. */
            if (mask & GL_ACCUM_BUFFER_BIT) {
      _mesa_ClearAccum(attr->Accum.ClearColor[0],
                           if (mask & GL_COLOR_BUFFER_BIT) {
      TEST_AND_CALL1(Color.ClearIndex, ClearIndex);
   _mesa_ClearColor(attr->Color.ClearColor.f[0],
                     TEST_AND_CALL1(Color.IndexMask, IndexMask);
   if (ctx->Color.ColorMask != attr->Color.ColorMask) {
      if (!ctx->Extensions.EXT_draw_buffers2) {
      _mesa_ColorMask(GET_COLORMASK_BIT(attr->Color.ColorMask, 0, 0),
                  } else {
      for (unsigned i = 0; i < ctx->Const.MaxDrawBuffers; i++) {
      _mesa_ColorMaski(i,
                  GET_COLORMASK_BIT(attr->Color.ColorMask, i, 0),
         }
   if (memcmp(ctx->Color.DrawBuffer, attr->Color.DrawBuffer,
            /* Need to determine if more than one color output is
   * specified.  If so, call glDrawBuffersARB, else call
   * glDrawBuffer().  This is a subtle, but essential point
   * since GL_FRONT (for example) is illegal for the former
   * function, but legal for the later.
   */
                  for (i = 1; i < ctx->Const.MaxDrawBuffers; i++) {
      if (attr->Color.DrawBuffer[i] != GL_NONE) {
      multipleBuffers = GL_TRUE;
         }
   /* Call the API_level functions, not _mesa_drawbuffers()
   * since we need to do error checking on the pop'd
   * GL_DRAW_BUFFER.
   * Ex: if GL_FRONT were pushed, but we're popping with a
   * user FBO bound, GL_FRONT will be illegal and we'll need
   * to record that error.  Per OpenGL ARB decision.
   */
                                       } else {
            }
   TEST_AND_UPDATE(ctx->Color.AlphaEnabled, attr->Color.AlphaEnabled,
         TEST_AND_CALL2(Color.AlphaFunc, Color.AlphaRefUnclamped, AlphaFunc);
   if (ctx->Color.BlendEnabled != attr->Color.BlendEnabled) {
      if (ctx->Extensions.EXT_draw_buffers2) {
      for (unsigned i = 0; i < ctx->Const.MaxDrawBuffers; i++) {
      TEST_AND_UPDATE_INDEX(ctx->Color.BlendEnabled,
         }
   else {
      TEST_AND_UPDATE(ctx->Color.BlendEnabled & 0x1,
         }
   if (ctx->Color._BlendFuncPerBuffer ||
      ctx->Color._BlendEquationPerBuffer) {
   /* set blend per buffer */
   GLuint buf;
   for (buf = 0; buf < ctx->Const.MaxDrawBuffers; buf++) {
      _mesa_BlendFuncSeparateiARB(buf, attr->Color.Blend[buf].SrcRGB,
                     _mesa_BlendEquationSeparateiARB(buf,
               }
   else {
      /* set same blend modes for all buffers */
   _mesa_BlendFuncSeparate(attr->Color.Blend[0].SrcRGB,
                     /* This special case is because glBlendEquationSeparateEXT
   * cannot take GL_LOGIC_OP as a parameter.
   */
   if (attr->Color.Blend[0].EquationRGB ==
      attr->Color.Blend[0].EquationA) {
      }
   else {
      TEST_AND_CALL2(Color.Blend[0].EquationRGB,
         }
   _mesa_BlendColor(attr->Color.BlendColorUnclamped[0],
                     TEST_AND_CALL1(Color.LogicOp, LogicOp);
   TEST_AND_UPDATE(ctx->Color.ColorLogicOpEnabled,
         TEST_AND_UPDATE(ctx->Color.IndexLogicOpEnabled,
         TEST_AND_UPDATE(ctx->Color.DitherFlag, attr->Color.DitherFlag,
         if (ctx->Extensions.ARB_color_buffer_float) {
      TEST_AND_CALL1_SEL(Color.ClampFragmentColor, ClampColor,
      }
   if (ctx->Extensions.ARB_color_buffer_float || ctx->Version >= 30) {
      TEST_AND_CALL1_SEL(Color.ClampReadColor, ClampColor,
      }
   /* GL_ARB_framebuffer_sRGB / GL_EXT_framebuffer_sRGB */
   if (ctx->Extensions.EXT_framebuffer_sRGB) {
      TEST_AND_UPDATE(ctx->Color.sRGBEnabled, attr->Color.sRGBEnabled,
                  if (mask & GL_CURRENT_BIT) {
      memcpy(&ctx->Current, &attr->Current,
                     if (mask & GL_DEPTH_BUFFER_BIT) {
      TEST_AND_CALL1(Depth.Func, DepthFunc);
   TEST_AND_CALL1(Depth.Clear, ClearDepth);
   TEST_AND_UPDATE(ctx->Depth.Test, attr->Depth.Test, GL_DEPTH_TEST);
   TEST_AND_CALL1(Depth.Mask, DepthMask);
   if (ctx->Extensions.EXT_depth_bounds_test) {
      TEST_AND_UPDATE(ctx->Depth.BoundsTest, attr->Depth.BoundsTest,
                        if (mask & GL_ENABLE_BIT)
            if (mask & GL_EVAL_BIT) {
      memcpy(&ctx->Eval, &attr->Eval, sizeof(struct gl_eval_attrib));
               if (mask & GL_FOG_BIT) {
      TEST_AND_UPDATE(ctx->Fog.Enabled, attr->Fog.Enabled, GL_FOG);
   _mesa_Fogfv(GL_FOG_COLOR, attr->Fog.Color);
   TEST_AND_CALL1_SEL(Fog.Density, Fogf, GL_FOG_DENSITY);
   TEST_AND_CALL1_SEL(Fog.Start, Fogf, GL_FOG_START);
   TEST_AND_CALL1_SEL(Fog.End, Fogf, GL_FOG_END);
   TEST_AND_CALL1_SEL(Fog.Index, Fogf, GL_FOG_INDEX);
               if (mask & GL_HINT_BIT) {
      TEST_AND_CALL1_SEL(Hint.PerspectiveCorrection, Hint, GL_PERSPECTIVE_CORRECTION_HINT);
   TEST_AND_CALL1_SEL(Hint.PointSmooth, Hint, GL_POINT_SMOOTH_HINT);
   TEST_AND_CALL1_SEL(Hint.LineSmooth, Hint, GL_LINE_SMOOTH_HINT);
   TEST_AND_CALL1_SEL(Hint.PolygonSmooth, Hint, GL_POLYGON_SMOOTH_HINT);
   TEST_AND_CALL1_SEL(Hint.Fog, Hint, GL_FOG_HINT);
               if (mask & GL_LIGHTING_BIT) {
      GLuint i;
   /* lighting enable */
   TEST_AND_UPDATE(ctx->Light.Enabled, attr->Light.Enabled, GL_LIGHTING);
   /* per-light state */
   if (_math_matrix_is_dirty(ctx->ModelviewMatrixStack.Top))
            /* Fast path for other drivers. */
            memcpy(ctx->Light.LightSource, attr->Light.LightSource,
         memcpy(&ctx->Light.Model, &attr->Light.Model,
            for (i = 0; i < ctx->Const.MaxLights; i++) {
      TEST_AND_UPDATE(ctx->Light.Light[i].Enabled,
               memcpy(&ctx->Light.Light[i], &attr->Light.Light[i],
      }
   /* shade model */
   TEST_AND_CALL1(Light.ShadeModel, ShadeModel);
   /* color material */
   TEST_AND_CALL2(Light.ColorMaterialFace, Light.ColorMaterialMode,
         TEST_AND_UPDATE(ctx->Light.ColorMaterialEnabled,
         /* Shininess material is used by the fixed-func vertex program. */
   ctx->NewState |= _NEW_MATERIAL | _NEW_FF_VERT_PROGRAM;
   memcpy(&ctx->Light.Material, &attr->Light.Material,
         if (ctx->Extensions.ARB_color_buffer_float) {
                     if (mask & GL_LINE_BIT) {
      TEST_AND_UPDATE(ctx->Line.SmoothFlag, attr->Line.SmoothFlag, GL_LINE_SMOOTH);
   TEST_AND_UPDATE(ctx->Line.StippleFlag, attr->Line.StippleFlag, GL_LINE_STIPPLE);
   TEST_AND_CALL2(Line.StippleFactor, Line.StipplePattern, LineStipple);
               if (mask & GL_LIST_BIT)
            if (mask & GL_PIXEL_MODE_BIT) {
      memcpy(&ctx->Pixel, &attr->Pixel, sizeof(struct gl_pixel_attrib));
   /* XXX what other pixel state needs to be set by function calls? */
   _mesa_ReadBuffer(ctx->Pixel.ReadBuffer);
               if (mask & GL_POINT_BIT) {
      TEST_AND_CALL1(Point.Size, PointSize);
   TEST_AND_UPDATE(ctx->Point.SmoothFlag, attr->Point.SmoothFlag, GL_POINT_SMOOTH);
   _mesa_PointParameterfv(GL_DISTANCE_ATTENUATION_EXT, attr->Point.Params);
   TEST_AND_CALL1_SEL(Point.MinSize, PointParameterf, GL_POINT_SIZE_MIN_EXT);
   TEST_AND_CALL1_SEL(Point.MaxSize, PointParameterf, GL_POINT_SIZE_MAX_EXT);
            if (ctx->Point.CoordReplace != attr->Point.CoordReplace) {
      ctx->NewState |= _NEW_POINT | _NEW_FF_VERT_PROGRAM;
      }
   TEST_AND_UPDATE(ctx->Point.PointSprite, attr->Point.PointSprite,
            if ((_mesa_is_desktop_gl_compat(ctx) && ctx->Version >= 20)
      || _mesa_is_desktop_gl_core(ctx))
            if (mask & GL_POLYGON_BIT) {
      TEST_AND_CALL1(Polygon.CullFaceMode, CullFace);
   TEST_AND_CALL1(Polygon.FrontFace, FrontFace);
   TEST_AND_CALL1_SEL(Polygon.FrontMode, PolygonMode, GL_FRONT);
   TEST_AND_CALL1_SEL(Polygon.BackMode, PolygonMode, GL_BACK);
   _mesa_polygon_offset_clamp(ctx,
                     TEST_AND_UPDATE(ctx->Polygon.SmoothFlag, attr->Polygon.SmoothFlag, GL_POLYGON_SMOOTH);
   TEST_AND_UPDATE(ctx->Polygon.StippleFlag, attr->Polygon.StippleFlag, GL_POLYGON_STIPPLE);
   TEST_AND_UPDATE(ctx->Polygon.CullFlag, attr->Polygon.CullFlag, GL_CULL_FACE);
   TEST_AND_UPDATE(ctx->Polygon.OffsetPoint, attr->Polygon.OffsetPoint,
         TEST_AND_UPDATE(ctx->Polygon.OffsetLine, attr->Polygon.OffsetLine,
         TEST_AND_UPDATE(ctx->Polygon.OffsetFill, attr->Polygon.OffsetFill,
               if (mask & GL_POLYGON_STIPPLE_BIT) {
                           if (mask & GL_SCISSOR_BIT) {
               for (i = 0; i < ctx->Const.MaxViewports; i++) {
      _mesa_set_scissor(ctx, i,
                     attr->Scissor.ScissorArray[i].X,
   TEST_AND_UPDATE_INDEX(ctx->Scissor.EnableFlags,
      }
   if (ctx->Extensions.EXT_window_rectangles) {
      STATIC_ASSERT(sizeof(struct gl_scissor_rect) ==
         _mesa_WindowRectanglesEXT(
                        if (mask & GL_STENCIL_BUFFER_BIT) {
      TEST_AND_UPDATE(ctx->Stencil.Enabled, attr->Stencil.Enabled,
         TEST_AND_CALL1(Stencil.Clear, ClearStencil);
   if (ctx->Extensions.EXT_stencil_two_side) {
      TEST_AND_UPDATE(ctx->Stencil.TestTwoSide, attr->Stencil.TestTwoSide,
         _mesa_ActiveStencilFaceEXT(attr->Stencil.ActiveFace
      }
   /* front state */
   _mesa_StencilFuncSeparate(GL_FRONT,
                     TEST_AND_CALL1_SEL(Stencil.WriteMask[0], StencilMaskSeparate, GL_FRONT);
   _mesa_StencilOpSeparate(GL_FRONT, attr->Stencil.FailFunc[0],
               /* back state */
   _mesa_StencilFuncSeparate(GL_BACK,
                     TEST_AND_CALL1_SEL(Stencil.WriteMask[1], StencilMaskSeparate, GL_BACK);
   _mesa_StencilOpSeparate(GL_BACK, attr->Stencil.FailFunc[1],
                     if (mask & GL_TRANSFORM_BIT) {
      GLuint i;
   TEST_AND_CALL1(Transform.MatrixMode, MatrixMode);
   if (_math_matrix_is_dirty(ctx->ProjectionMatrixStack.Top))
            ctx->NewState |= _NEW_TRANSFORM;
            /* restore clip planes */
   for (i = 0; i < ctx->Const.MaxClipPlanes; i++) {
      const GLfloat *eyePlane = attr->Transform.EyeUserPlane[i];
   COPY_4V(ctx->Transform.EyeUserPlane[i], eyePlane);
   TEST_AND_UPDATE_BIT(ctx->Transform.ClipPlanesEnabled,
                     /* normalize/rescale */
   TEST_AND_UPDATE(ctx->Transform.Normalize, attr->Transform.Normalize,
         TEST_AND_UPDATE(ctx->Transform.RescaleNormals,
            if (!ctx->Extensions.AMD_depth_clamp_separate) {
      TEST_AND_UPDATE(ctx->Transform.DepthClampNear &&
                  } else {
      TEST_AND_UPDATE(ctx->Transform.DepthClampNear,
               TEST_AND_UPDATE(ctx->Transform.DepthClampFar,
                     if (ctx->Extensions.ARB_clip_control) {
      TEST_AND_CALL2(Transform.ClipOrigin, Transform.ClipDepthMode,
                  if (mask & GL_TEXTURE_BIT) {
      pop_texture_group(ctx, &attr->Texture);
   ctx->NewState |= _NEW_TEXTURE_OBJECT | _NEW_TEXTURE_STATE |
               if (mask & GL_VIEWPORT_BIT) {
               for (i = 0; i < ctx->Const.MaxViewports; i++) {
               if (memcmp(&ctx->ViewportArray[i].X, &vp->X, sizeof(float) * 6)) {
                              if (ctx->invalidate_on_gl_viewport)
                  if (ctx->Extensions.NV_conservative_raster) {
      GLuint biasx = attr->Viewport.SubpixelPrecisionBias[0];
   GLuint biasy = attr->Viewport.SubpixelPrecisionBias[1];
                  if (mask & GL_MULTISAMPLE_BIT_ARB) {
      TEST_AND_UPDATE(ctx->Multisample.Enabled,
                  TEST_AND_UPDATE(ctx->Multisample.SampleCoverage,
                  TEST_AND_UPDATE(ctx->Multisample.SampleAlphaToCoverage,
                  TEST_AND_UPDATE(ctx->Multisample.SampleAlphaToOne,
                  TEST_AND_CALL2(Multisample.SampleCoverageValue,
            TEST_AND_CALL1(Multisample.SampleAlphaToCoverageDitherControl,
                  }
         /**
   * Copy gl_pixelstore_attrib from src to dst, updating buffer
   * object refcounts.
   */
   static void
   copy_pixelstore(struct gl_context *ctx,
               {
      dst->Alignment = src->Alignment;
   dst->RowLength = src->RowLength;
   dst->SkipPixels = src->SkipPixels;
   dst->SkipRows = src->SkipRows;
   dst->ImageHeight = src->ImageHeight;
   dst->SkipImages = src->SkipImages;
   dst->SwapBytes = src->SwapBytes;
   dst->LsbFirst = src->LsbFirst;
   dst->Invert = src->Invert;
      }
         #define GL_CLIENT_PACK_BIT (1<<20)
   #define GL_CLIENT_UNPACK_BIT (1<<21)
      static void
   copy_vertex_attrib_array(struct gl_context *ctx,
               {
      dst->Ptr            = src->Ptr;
   dst->RelativeOffset = src->RelativeOffset;
   dst->Format         = src->Format;
   dst->Stride         = src->Stride;
   dst->BufferBindingIndex = src->BufferBindingIndex;
   dst->_EffBufferBindingIndex = src->_EffBufferBindingIndex;
      }
      static void
   copy_vertex_buffer_binding(struct gl_context *ctx,
               {
      dst->Offset          = src->Offset;
   dst->Stride          = src->Stride;
   dst->InstanceDivisor = src->InstanceDivisor;
   dst->_BoundArrays    = src->_BoundArrays;
   dst->_EffBoundArrays = src->_EffBoundArrays;
               }
      /**
   * Copy gl_vertex_array_object from src to dest.
   * 'dest' must be in an initialized state.
   */
   static void
   copy_array_object(struct gl_context *ctx,
                     {
      /* skip Name */
            while (copy_attrib_mask) {
               copy_vertex_attrib_array(ctx, &dest->VertexAttrib[i], &src->VertexAttrib[i]);
               /* Enabled must be the same than on push */
   dest->Enabled = src->Enabled;
   dest->_EnabledWithMapMode = src->_EnabledWithMapMode;
   /* The bitmask of bound VBOs needs to match the VertexBinding array */
   dest->VertexAttribBufferMask = src->VertexAttribBufferMask;
   dest->NonZeroDivisorMask = src->NonZeroDivisorMask;
   dest->_AttributeMapMode = src->_AttributeMapMode;
      }
      /**
   * Copy gl_array_attrib from src to dest.
   * 'dest' must be in an initialized state.
   */
   static void
   copy_array_attrib(struct gl_context *ctx,
                     struct gl_array_attrib *dest,
   {
      /* skip ArrayObj */
   /* skip DefaultArrayObj, Objects */
   dest->ActiveTexture = src->ActiveTexture;
   dest->LockFirst = src->LockFirst;
   dest->LockCount = src->LockCount;
   dest->PrimitiveRestart = src->PrimitiveRestart;
   dest->PrimitiveRestartFixedIndex = src->PrimitiveRestartFixedIndex;
   dest->RestartIndex = src->RestartIndex;
   memcpy(dest->_PrimitiveRestart, src->_PrimitiveRestart,
         memcpy(dest->_RestartIndex, src->_RestartIndex, sizeof(src->_RestartIndex));
   /* skip NewState */
            if (!vbo_deleted)
            /* skip ArrayBufferObj */
      }
      /**
   * Save the content of src to dest.
   */
   static void
   save_array_attrib(struct gl_context *ctx,
               {
      /* Set the Name, needed for restore, but do never overwrite.
   * Needs to match value in the object hash. */
   dest->VAO->Name = src->VAO->Name;
   dest->VAO->NonDefaultStateMask = src->VAO->NonDefaultStateMask;
   /* And copy all of the rest. */
            /* Just reference them here */
   _mesa_reference_buffer_object(ctx, &dest->ArrayBufferObj,
         _mesa_reference_buffer_object(ctx, &dest->VAO->IndexBufferObj,
      }
      /**
   * Restore the content of src to dest.
   */
   static void
   restore_array_attrib(struct gl_context *ctx,
               {
               /* The ARB_vertex_array_object spec says:
   *
   *     "BindVertexArray fails and an INVALID_OPERATION error is generated
   *     if array is not a name returned from a previous call to
   *     GenVertexArrays, or if such a name has since been deleted with
   *     DeleteVertexArrays."
   *
   * Therefore popping a deleted VAO cannot magically recreate it.
   */
   if (!is_vao_name_zero && !_mesa_IsVertexArray(src->VAO->Name))
                     /* Restore or recreate the buffer objects by the names ... */
   if (is_vao_name_zero || !src->ArrayBufferObj ||
      _mesa_IsBuffer(src->ArrayBufferObj->Name)) {
   /* ... and restore its content */
   dest->VAO->NonDefaultStateMask |= src->VAO->NonDefaultStateMask;
   copy_array_attrib(ctx, dest, src, false,
            _mesa_BindBuffer(GL_ARRAY_BUFFER_ARB,
            } else {
                  if (is_vao_name_zero || !src->VAO->IndexBufferObj ||
      _mesa_IsBuffer(src->VAO->IndexBufferObj->Name)) {
   _mesa_BindBuffer(GL_ELEMENT_ARRAY_BUFFER_ARB,
                     _mesa_update_edgeflag_state_vao(ctx);
   _mesa_set_varying_vp_inputs(ctx, ctx->VertexProgram._VPModeInputFilter &
      }
         void GLAPIENTRY
   _mesa_PushClientAttrib(GLbitfield mask)
   {
                        if (ctx->ClientAttribStackDepth >= MAX_CLIENT_ATTRIB_STACK_DEPTH) {
      _mesa_error(ctx, GL_STACK_OVERFLOW, "glPushClientAttrib");
               head = &ctx->ClientAttribStack[ctx->ClientAttribStackDepth];
            if (mask & GL_CLIENT_PIXEL_STORE_BIT) {
      copy_pixelstore(ctx, &head->Pack, &ctx->Pack);
               if (mask & GL_CLIENT_VERTEX_ARRAY_BIT) {
      _mesa_initialize_vao(ctx, &head->VAO, 0);
   /* Use the VAO declared within the node instead of allocating it. */
   head->Array.VAO = &head->VAO;
                  }
         void GLAPIENTRY
   _mesa_PopClientAttrib(void)
   {
                        if (ctx->ClientAttribStackDepth == 0) {
      _mesa_error(ctx, GL_STACK_UNDERFLOW, "glPopClientAttrib");
               ctx->ClientAttribStackDepth--;
            if (head->Mask & GL_CLIENT_PIXEL_STORE_BIT) {
      copy_pixelstore(ctx, &ctx->Pack, &head->Pack);
            copy_pixelstore(ctx, &ctx->Unpack, &head->Unpack);
               if (head->Mask & GL_CLIENT_VERTEX_ARRAY_BIT) {
               /* _mesa_unbind_array_object_vbos can't use NonDefaultStateMask because
   * it's used by internal VAOs which don't always update the mask, so do
   * it manually here.
   */
   GLbitfield mask = head->VAO.NonDefaultStateMask;
   while (mask) {
      unsigned i = u_bit_scan(&mask);
               _mesa_reference_buffer_object(ctx, &head->VAO.IndexBufferObj, NULL);
         }
      void GLAPIENTRY
   _mesa_ClientAttribDefaultEXT( GLbitfield mask )
   {
      if (mask & GL_CLIENT_PIXEL_STORE_BIT) {
      _mesa_PixelStorei(GL_UNPACK_SWAP_BYTES, GL_FALSE);
   _mesa_PixelStorei(GL_UNPACK_LSB_FIRST, GL_FALSE);
   _mesa_PixelStorei(GL_UNPACK_IMAGE_HEIGHT, 0);
   _mesa_PixelStorei(GL_UNPACK_SKIP_IMAGES, 0);
   _mesa_PixelStorei(GL_UNPACK_ROW_LENGTH, 0);
   _mesa_PixelStorei(GL_UNPACK_SKIP_ROWS, 0);
   _mesa_PixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
   _mesa_PixelStorei(GL_UNPACK_ALIGNMENT, 4);
   _mesa_PixelStorei(GL_PACK_SWAP_BYTES, GL_FALSE);
   _mesa_PixelStorei(GL_PACK_LSB_FIRST, GL_FALSE);
   _mesa_PixelStorei(GL_PACK_IMAGE_HEIGHT, 0);
   _mesa_PixelStorei(GL_PACK_SKIP_IMAGES, 0);
   _mesa_PixelStorei(GL_PACK_ROW_LENGTH, 0);
   _mesa_PixelStorei(GL_PACK_SKIP_ROWS, 0);
   _mesa_PixelStorei(GL_PACK_SKIP_PIXELS, 0);
            _mesa_BindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
      }
   if (mask & GL_CLIENT_VERTEX_ARRAY_BIT) {
      GET_CURRENT_CONTEXT(ctx);
            _mesa_BindBuffer(GL_ARRAY_BUFFER, 0);
            _mesa_DisableClientState(GL_EDGE_FLAG_ARRAY);
            _mesa_DisableClientState(GL_INDEX_ARRAY);
            _mesa_DisableClientState(GL_SECONDARY_COLOR_ARRAY);
            _mesa_DisableClientState(GL_FOG_COORD_ARRAY);
            for (i = 0; i < ctx->Const.MaxTextureCoordUnits; i++) {
      _mesa_ClientActiveTexture(GL_TEXTURE0 + i);
   _mesa_DisableClientState(GL_TEXTURE_COORD_ARRAY);
               _mesa_DisableClientState(GL_COLOR_ARRAY);
            _mesa_DisableClientState(GL_NORMAL_ARRAY);
            _mesa_DisableClientState(GL_VERTEX_ARRAY);
            for (i = 0; i < ctx->Const.Program[MESA_SHADER_VERTEX].MaxAttribs; i++) {
      _mesa_DisableVertexAttribArray(i);
                        _mesa_PrimitiveRestartIndex_no_error(0);
   if (ctx->Version >= 31)
         else if (_mesa_has_NV_primitive_restart(ctx))
            if (_mesa_has_ARB_ES3_compatibility(ctx))
         }
      void GLAPIENTRY
   _mesa_PushClientAttribDefaultEXT( GLbitfield mask )
   {
      _mesa_PushClientAttrib(mask);
      }
         /**
   * Free any attribute state data that might be attached to the context.
   */
   void
   _mesa_free_attrib_data(struct gl_context *ctx)
   {
      for (unsigned i = 0; i < ARRAY_SIZE(ctx->AttribStack); i++)
      }
         void
   _mesa_init_attrib(struct gl_context *ctx)
   {
      /* Renderer and client attribute stacks */
   ctx->AttribStackDepth = 0;
      }
