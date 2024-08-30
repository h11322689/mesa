   /**
   * \file blend.c
   * Blending operations.
   */
      /*
   * Mesa 3-D graphics library
   *
   * Copyright (C) 1999-2006  Brian Paul   All Rights Reserved.
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
   #include "blend.h"
   #include "context.h"
   #include "draw_validate.h"
   #include "enums.h"
   #include "macros.h"
   #include "mtypes.h"
   #include "state.h"
   #include "api_exec_decl.h"
            /**
   * Check if given blend source factor is legal.
   * \return GL_TRUE if legal, GL_FALSE otherwise.
   */
   static GLboolean
   legal_src_factor(const struct gl_context *ctx, GLenum factor)
   {
      switch (factor) {
   case GL_SRC_COLOR:
   case GL_ONE_MINUS_SRC_COLOR:
   case GL_ZERO:
   case GL_ONE:
   case GL_DST_COLOR:
   case GL_ONE_MINUS_DST_COLOR:
   case GL_SRC_ALPHA:
   case GL_ONE_MINUS_SRC_ALPHA:
   case GL_DST_ALPHA:
   case GL_ONE_MINUS_DST_ALPHA:
   case GL_SRC_ALPHA_SATURATE:
         case GL_CONSTANT_COLOR:
   case GL_ONE_MINUS_CONSTANT_COLOR:
   case GL_CONSTANT_ALPHA:
   case GL_ONE_MINUS_CONSTANT_ALPHA:
         case GL_SRC1_COLOR:
   case GL_SRC1_ALPHA:
   case GL_ONE_MINUS_SRC1_COLOR:
   case GL_ONE_MINUS_SRC1_ALPHA:
      return ctx->API != API_OPENGLES
      default:
            }
         /**
   * Check if given blend destination factor is legal.
   * \return GL_TRUE if legal, GL_FALSE otherwise.
   */
   static GLboolean
   legal_dst_factor(const struct gl_context *ctx, GLenum factor)
   {
      switch (factor) {
   case GL_DST_COLOR:
   case GL_ONE_MINUS_DST_COLOR:
   case GL_ZERO:
   case GL_ONE:
   case GL_SRC_COLOR:
   case GL_ONE_MINUS_SRC_COLOR:
   case GL_SRC_ALPHA:
   case GL_ONE_MINUS_SRC_ALPHA:
   case GL_DST_ALPHA:
   case GL_ONE_MINUS_DST_ALPHA:
         case GL_CONSTANT_COLOR:
   case GL_ONE_MINUS_CONSTANT_COLOR:
   case GL_CONSTANT_ALPHA:
   case GL_ONE_MINUS_CONSTANT_ALPHA:
         case GL_SRC_ALPHA_SATURATE:
      return (ctx->API != API_OPENGLES
            case GL_SRC1_COLOR:
   case GL_SRC1_ALPHA:
   case GL_ONE_MINUS_SRC1_COLOR:
   case GL_ONE_MINUS_SRC1_ALPHA:
      return ctx->API != API_OPENGLES
      default:
            }
         /**
   * Check if src/dest RGB/A blend factors are legal.  If not generate
   * a GL error.
   * \return GL_TRUE if factors are legal, GL_FALSE otherwise.
   */
   static GLboolean
   validate_blend_factors(struct gl_context *ctx, const char *func,
               {
      if (!legal_src_factor(ctx, sfactorRGB)) {
      _mesa_error(ctx, GL_INVALID_ENUM,
                           if (!legal_dst_factor(ctx, dfactorRGB)) {
      _mesa_error(ctx, GL_INVALID_ENUM,
                           if (sfactorA != sfactorRGB && !legal_src_factor(ctx, sfactorA)) {
      _mesa_error(ctx, GL_INVALID_ENUM,
                           if (dfactorA != dfactorRGB && !legal_dst_factor(ctx, dfactorA)) {
      _mesa_error(ctx, GL_INVALID_ENUM,
                              }
         static GLboolean
   blend_factor_is_dual_src(GLenum factor)
   {
      return (factor == GL_SRC1_COLOR ||
   factor == GL_SRC1_ALPHA ||
   factor == GL_ONE_MINUS_SRC1_COLOR ||
      }
      static bool
   update_uses_dual_src(struct gl_context *ctx, int buf)
   {
      bool uses_dual_src =
      (blend_factor_is_dual_src(ctx->Color.Blend[buf].SrcRGB) ||
   blend_factor_is_dual_src(ctx->Color.Blend[buf].DstRGB) ||
   blend_factor_is_dual_src(ctx->Color.Blend[buf].SrcA) ||
         if (((ctx->Color._BlendUsesDualSrc >> buf) & 0x1) != uses_dual_src) {
      if (uses_dual_src)
         else
            }
      }
         /**
   * Return the number of per-buffer blend states to update in
   * glBlendFunc, glBlendFuncSeparate, glBlendEquation, etc.
   */
   static inline unsigned
   num_buffers(const struct gl_context *ctx)
   {
      return ctx->Extensions.ARB_draw_buffers_blend
      }
         /* Returns true if there was no change */
   static bool
   skip_blend_state_update(const struct gl_context *ctx,
               {
      /* Check if we're really changing any state.  If not, return early. */
   if (ctx->Color._BlendFuncPerBuffer) {
               /* Check all per-buffer states */
   for (unsigned buf = 0; buf < numBuffers; buf++) {
      if (ctx->Color.Blend[buf].SrcRGB != sfactorRGB ||
      ctx->Color.Blend[buf].DstRGB != dfactorRGB ||
   ctx->Color.Blend[buf].SrcA != sfactorA ||
   ctx->Color.Blend[buf].DstA != dfactorA) {
            }
   else {
      /* only need to check 0th per-buffer state */
   if (ctx->Color.Blend[0].SrcRGB != sfactorRGB ||
      ctx->Color.Blend[0].DstRGB != dfactorRGB ||
   ctx->Color.Blend[0].SrcA != sfactorA ||
   ctx->Color.Blend[0].DstA != dfactorA) {
                     }
         static void
   blend_func_separate(struct gl_context *ctx,
               {
      FLUSH_VERTICES(ctx, 0, GL_COLOR_BUFFER_BIT);
            const unsigned numBuffers = num_buffers(ctx);
   for (unsigned buf = 0; buf < numBuffers; buf++) {
      ctx->Color.Blend[buf].SrcRGB = sfactorRGB;
   ctx->Color.Blend[buf].DstRGB = dfactorRGB;
   ctx->Color.Blend[buf].SrcA = sfactorA;
               GLbitfield old_blend_uses_dual_src = ctx->Color._BlendUsesDualSrc;
   update_uses_dual_src(ctx, 0);
   /* We have to replicate the bit to all color buffers. */
   if (ctx->Color._BlendUsesDualSrc & 0x1)
         else
            if (ctx->Color._BlendUsesDualSrc != old_blend_uses_dual_src)
               }
         /**
   * Specify the blending operation.
   *
   * \param sfactor source factor operator.
   * \param dfactor destination factor operator.
   *
   * \sa glBlendFunc, glBlendFuncSeparateEXT
   */
   void GLAPIENTRY
   _mesa_BlendFunc( GLenum sfactor, GLenum dfactor )
   {
               if (skip_blend_state_update(ctx, sfactor, dfactor, sfactor, dfactor))
            if (!validate_blend_factors(ctx, "glBlendFunc",
                           }
         void GLAPIENTRY
   _mesa_BlendFunc_no_error(GLenum sfactor, GLenum dfactor)
   {
               if (skip_blend_state_update(ctx, sfactor, dfactor, sfactor, dfactor))
               }
         /**
   * Set the separate blend source/dest factors for all draw buffers.
   *
   * \param sfactorRGB RGB source factor operator.
   * \param dfactorRGB RGB destination factor operator.
   * \param sfactorA alpha source factor operator.
   * \param dfactorA alpha destination factor operator.
   */
   void GLAPIENTRY
   _mesa_BlendFuncSeparate( GLenum sfactorRGB, GLenum dfactorRGB,
         {
               if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "glBlendFuncSeparate %s %s %s %s\n",
               _mesa_enum_to_string(sfactorRGB),
               if (skip_blend_state_update(ctx, sfactorRGB, dfactorRGB, sfactorA, dfactorA))
            if (!validate_blend_factors(ctx, "glBlendFuncSeparate",
                                 }
         void GLAPIENTRY
   _mesa_BlendFuncSeparate_no_error(GLenum sfactorRGB, GLenum dfactorRGB,
         {
               if (skip_blend_state_update(ctx, sfactorRGB, dfactorRGB, sfactorA, dfactorA))
               }
         void GLAPIENTRY
   _mesa_BlendFunciARB_no_error(GLuint buf, GLenum sfactor, GLenum dfactor)
   {
      _mesa_BlendFuncSeparateiARB_no_error(buf, sfactor, dfactor, sfactor,
      }
         /**
   * Set blend source/dest factors for one color buffer/target.
   */
   void GLAPIENTRY
   _mesa_BlendFunciARB(GLuint buf, GLenum sfactor, GLenum dfactor)
   {
         }
         static ALWAYS_INLINE void
   blend_func_separatei(GLuint buf, GLenum sfactorRGB, GLenum dfactorRGB,
         {
               if (!no_error) {
      if (!ctx->Extensions.ARB_draw_buffers_blend) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glBlendFunc[Separate]i()");
               if (buf >= ctx->Const.MaxDrawBuffers) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glBlendFuncSeparatei(buffer=%u)",
                        if (ctx->Color.Blend[buf].SrcRGB == sfactorRGB &&
      ctx->Color.Blend[buf].DstRGB == dfactorRGB &&
   ctx->Color.Blend[buf].SrcA == sfactorA &&
   ctx->Color.Blend[buf].DstA == dfactorA)
         if (!no_error && !validate_blend_factors(ctx, "glBlendFuncSeparatei",
                              FLUSH_VERTICES(ctx, 0, GL_COLOR_BUFFER_BIT);
            ctx->Color.Blend[buf].SrcRGB = sfactorRGB;
   ctx->Color.Blend[buf].DstRGB = dfactorRGB;
   ctx->Color.Blend[buf].SrcA = sfactorA;
   ctx->Color.Blend[buf].DstA = dfactorA;
   if (update_uses_dual_src(ctx, buf))
            }
         void GLAPIENTRY
   _mesa_BlendFuncSeparateiARB_no_error(GLuint buf, GLenum sfactorRGB,
               {
      blend_func_separatei(buf, sfactorRGB, dfactorRGB, sfactorA, dfactorA,
      }
         /**
   * Set separate blend source/dest factors for one color buffer/target.
   */
   void GLAPIENTRY
   _mesa_BlendFuncSeparateiARB(GLuint buf, GLenum sfactorRGB, GLenum dfactorRGB,
         {
      blend_func_separatei(buf, sfactorRGB, dfactorRGB, sfactorA, dfactorA,
      }
         /**
   * Return true if \p mode is a legal blending equation, excluding
   * GL_KHR_blend_equation_advanced modes.
   */
   static bool
   legal_simple_blend_equation(const struct gl_context *ctx, GLenum mode)
   {
      switch (mode) {
   case GL_FUNC_ADD:
   case GL_FUNC_SUBTRACT:
   case GL_FUNC_REVERSE_SUBTRACT:
   case GL_MIN:
   case GL_MAX:
         default:
            }
      static enum gl_advanced_blend_mode
   advanced_blend_mode_from_gl_enum(GLenum mode)
   {
      switch (mode) {
   case GL_MULTIPLY_KHR:
         case GL_SCREEN_KHR:
         case GL_OVERLAY_KHR:
         case GL_DARKEN_KHR:
         case GL_LIGHTEN_KHR:
         case GL_COLORDODGE_KHR:
         case GL_COLORBURN_KHR:
         case GL_HARDLIGHT_KHR:
         case GL_SOFTLIGHT_KHR:
         case GL_DIFFERENCE_KHR:
         case GL_EXCLUSION_KHR:
         case GL_HSL_HUE_KHR:
         case GL_HSL_SATURATION_KHR:
         case GL_HSL_COLOR_KHR:
         case GL_HSL_LUMINOSITY_KHR:
         default:
            }
      /**
   * If \p mode is one of the advanced blending equations defined by
   * GL_KHR_blend_equation_advanced (and the extension is supported),
   * return the corresponding BLEND_* enum.  Otherwise, return BLEND_NONE
   * (which can also be treated as false).
   */
   static enum gl_advanced_blend_mode
   advanced_blend_mode(const struct gl_context *ctx, GLenum mode)
   {
      return _mesa_has_KHR_blend_equation_advanced(ctx) ?
      }
      static void
   set_advanced_blend_mode(struct gl_context *ctx,
         {
      if (ctx->Color._AdvancedBlendMode != advanced_mode) {
      ctx->Color._AdvancedBlendMode = advanced_mode;
         }
      /* This is really an extension function! */
   void GLAPIENTRY
   _mesa_BlendEquation( GLenum mode )
   {
      GET_CURRENT_CONTEXT(ctx);
   const unsigned numBuffers = num_buffers(ctx);
   unsigned buf;
   bool changed = false;
            if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "glBlendEquation(%s)\n",
         if (ctx->Color._BlendEquationPerBuffer) {
      /* Check all per-buffer states */
   for (buf = 0; buf < numBuffers; buf++) {
      if (ctx->Color.Blend[buf].EquationRGB != mode ||
      ctx->Color.Blend[buf].EquationA != mode) {
   changed = true;
            }
   else {
      /* only need to check 0th per-buffer state */
   if (ctx->Color.Blend[0].EquationRGB != mode ||
      ctx->Color.Blend[0].EquationA != mode) {
                  if (!changed)
               if (!legal_simple_blend_equation(ctx, mode) && !advanced_mode) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glBlendEquation");
               _mesa_flush_vertices_for_blend_adv(ctx, ctx->Color.BlendEnabled,
            for (buf = 0; buf < numBuffers; buf++) {
      ctx->Color.Blend[buf].EquationRGB = mode;
      }
   ctx->Color._BlendEquationPerBuffer = GL_FALSE;
      }
         /**
   * Set blend equation for one color buffer/target.
   */
   static void
   blend_equationi(struct gl_context *ctx, GLuint buf, GLenum mode,
         {
      if (ctx->Color.Blend[buf].EquationRGB == mode &&
      ctx->Color.Blend[buf].EquationA == mode)
         _mesa_flush_vertices_for_blend_adv(ctx, ctx->Color.BlendEnabled,
         ctx->Color.Blend[buf].EquationRGB = mode;
   ctx->Color.Blend[buf].EquationA = mode;
            if (buf == 0)
      }
         void GLAPIENTRY
   _mesa_BlendEquationiARB_no_error(GLuint buf, GLenum mode)
   {
               enum gl_advanced_blend_mode advanced_mode = advanced_blend_mode(ctx, mode);
      }
         void GLAPIENTRY
   _mesa_BlendEquationiARB(GLuint buf, GLenum mode)
   {
      GET_CURRENT_CONTEXT(ctx);
            if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "glBlendEquationi(%u, %s)\n",
         if (buf >= ctx->Const.MaxDrawBuffers) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glBlendEquationi(buffer=%u)",
                     if (!legal_simple_blend_equation(ctx, mode) && !advanced_mode) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glBlendEquationi");
                  }
         static void
   blend_equation_separate(struct gl_context *ctx, GLenum modeRGB, GLenum modeA,
         {
      const unsigned numBuffers = num_buffers(ctx);
   unsigned buf;
            if (ctx->Color._BlendEquationPerBuffer) {
      /* Check all per-buffer states */
   for (buf = 0; buf < numBuffers; buf++) {
      if (ctx->Color.Blend[buf].EquationRGB != modeRGB ||
      ctx->Color.Blend[buf].EquationA != modeA) {
   changed = true;
            } else {
      /* only need to check 0th per-buffer state */
   if (ctx->Color.Blend[0].EquationRGB != modeRGB ||
      ctx->Color.Blend[0].EquationA != modeA) {
                  if (!changed)
            if (!no_error) {
      if ((modeRGB != modeA) && !ctx->Extensions.EXT_blend_equation_separate) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                     /* Only allow simple blending equations.
   * The GL_KHR_blend_equation_advanced spec says:
   *
   *    "NOTE: These enums are not accepted by the <modeRGB> or <modeAlpha>
   *     parameters of BlendEquationSeparate or BlendEquationSeparatei."
   */
   if (!legal_simple_blend_equation(ctx, modeRGB)) {
      _mesa_error(ctx, GL_INVALID_ENUM,
                     if (!legal_simple_blend_equation(ctx, modeA)) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glBlendEquationSeparateEXT(modeA)");
                           for (buf = 0; buf < numBuffers; buf++) {
      ctx->Color.Blend[buf].EquationRGB = modeRGB;
      }
   ctx->Color._BlendEquationPerBuffer = GL_FALSE;
      }
         void GLAPIENTRY
   _mesa_BlendEquationSeparate_no_error(GLenum modeRGB, GLenum modeA)
   {
      GET_CURRENT_CONTEXT(ctx);
      }
         void GLAPIENTRY
   _mesa_BlendEquationSeparate(GLenum modeRGB, GLenum modeA)
   {
               if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "glBlendEquationSeparateEXT(%s %s)\n",
                  }
         static ALWAYS_INLINE void
   blend_equation_separatei(struct gl_context *ctx, GLuint buf, GLenum modeRGB,
         {
      if (ctx->Color.Blend[buf].EquationRGB == modeRGB &&
      ctx->Color.Blend[buf].EquationA == modeA)
         if (!no_error) {
      /* Only allow simple blending equations.
   * The GL_KHR_blend_equation_advanced spec says:
   *
   *    "NOTE: These enums are not accepted by the <modeRGB> or <modeAlpha>
   *     parameters of BlendEquationSeparate or BlendEquationSeparatei."
   */
   if (!legal_simple_blend_equation(ctx, modeRGB)) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glBlendEquationSeparatei(modeRGB)");
               if (!legal_simple_blend_equation(ctx, modeA)) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glBlendEquationSeparatei(modeA)");
                  _mesa_flush_vertices_for_blend_state(ctx);
   ctx->Color.Blend[buf].EquationRGB = modeRGB;
   ctx->Color.Blend[buf].EquationA = modeA;
   ctx->Color._BlendEquationPerBuffer = GL_TRUE;
      }
         void GLAPIENTRY
   _mesa_BlendEquationSeparateiARB_no_error(GLuint buf, GLenum modeRGB,
         {
      GET_CURRENT_CONTEXT(ctx);
      }
         /**
   * Set separate blend equations for one color buffer/target.
   */
   void GLAPIENTRY
   _mesa_BlendEquationSeparateiARB(GLuint buf, GLenum modeRGB, GLenum modeA)
   {
               if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "glBlendEquationSeparatei(%u, %s %s)\n", buf,
               if (buf >= ctx->Const.MaxDrawBuffers) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glBlendEquationSeparatei(buffer=%u)",
                        }
         /**
   * Set the blending color.
   *
   * \param red red color component.
   * \param green green color component.
   * \param blue blue color component.
   * \param alpha alpha color component.
   *
   * \sa glBlendColor().
   *
   * Clamps the parameters and updates gl_colorbuffer_attrib::BlendColor.  On a
   * change, flushes the vertices and notifies the driver via
   * dd_function_table::BlendColor callback.
   */
   void GLAPIENTRY
   _mesa_BlendColor( GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha )
   {
      GLfloat tmp[4];
            tmp[0] = red;
   tmp[1] = green;
   tmp[2] = blue;
            if (TEST_EQ_4V(tmp, ctx->Color.BlendColorUnclamped))
            FLUSH_VERTICES(ctx, 0, GL_COLOR_BUFFER_BIT);
   ctx->NewDriverState |= ST_NEW_BLEND_COLOR;
            ctx->Color.BlendColor[0] = CLAMP(tmp[0], 0.0F, 1.0F);
   ctx->Color.BlendColor[1] = CLAMP(tmp[1], 0.0F, 1.0F);
   ctx->Color.BlendColor[2] = CLAMP(tmp[2], 0.0F, 1.0F);
      }
         /**
   * Specify the alpha test function.
   *
   * \param func alpha comparison function.
   * \param ref reference value.
   *
   * Verifies the parameters and updates gl_colorbuffer_attrib. 
   * On a change, flushes the vertices and notifies the driver via
   * dd_function_table::AlphaFunc callback.
   */
   void GLAPIENTRY
   _mesa_AlphaFunc( GLenum func, GLclampf ref )
   {
               if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "glAlphaFunc(%s, %f)\n",
         if (ctx->Color.AlphaFunc == func && ctx->Color.AlphaRefUnclamped == ref)
            switch (func) {
   case GL_NEVER:
   case GL_LESS:
   case GL_EQUAL:
   case GL_LEQUAL:
   case GL_GREATER:
   case GL_NOTEQUAL:
   case GL_GEQUAL:
   case GL_ALWAYS:
      FLUSH_VERTICES(ctx, 0, GL_COLOR_BUFFER_BIT);
   ctx->NewDriverState |= ctx->DriverFlags.NewAlphaTest;
   ctx->Color.AlphaFunc = func;
   ctx->Color.AlphaRefUnclamped = ref;
   ctx->Color.AlphaRef = CLAMP(ref, 0.0F, 1.0F);
         default:
      _mesa_error( ctx, GL_INVALID_ENUM, "glAlphaFunc(func)" );
         }
      static const enum gl_logicop_mode color_logicop_mapping[16] = {
      COLOR_LOGICOP_CLEAR,
   COLOR_LOGICOP_AND,
   COLOR_LOGICOP_AND_REVERSE,
   COLOR_LOGICOP_COPY,
   COLOR_LOGICOP_AND_INVERTED,
   COLOR_LOGICOP_NOOP,
   COLOR_LOGICOP_XOR,
   COLOR_LOGICOP_OR,
   COLOR_LOGICOP_NOR,
   COLOR_LOGICOP_EQUIV,
   COLOR_LOGICOP_INVERT,
   COLOR_LOGICOP_OR_REVERSE,
   COLOR_LOGICOP_COPY_INVERTED,
   COLOR_LOGICOP_OR_INVERTED,
   COLOR_LOGICOP_NAND,
      };
      static ALWAYS_INLINE void
   logic_op(struct gl_context *ctx, GLenum opcode, bool no_error)
   {
      if (ctx->Color.LogicOp == opcode)
            if (!no_error) {
      switch (opcode) {
      case GL_CLEAR:
   case GL_SET:
   case GL_COPY:
   case GL_COPY_INVERTED:
   case GL_NOOP:
   case GL_INVERT:
   case GL_AND:
   case GL_NAND:
   case GL_OR:
   case GL_NOR:
   case GL_XOR:
   case GL_EQUIV:
   case GL_AND_REVERSE:
   case GL_AND_INVERTED:
   case GL_OR_REVERSE:
   case GL_OR_INVERTED:
         default:
      _mesa_error( ctx, GL_INVALID_ENUM, "glLogicOp" );
               FLUSH_VERTICES(ctx, 0, GL_COLOR_BUFFER_BIT);
   ctx->NewDriverState |= ST_NEW_BLEND;
   ctx->Color.LogicOp = opcode;
   ctx->Color._LogicOp = color_logicop_mapping[opcode & 0x0f];
      }
         /**
   * Specify a logic pixel operation for color index rendering.
   *
   * \param opcode operation.
   *
   * Verifies that \p opcode is a valid enum and updates
   * gl_colorbuffer_attrib::LogicOp.
   * On a change, flushes the vertices and notifies the driver via the
   * dd_function_table::LogicOpcode callback.
   */
   void GLAPIENTRY
   _mesa_LogicOp( GLenum opcode )
   {
               if (MESA_VERBOSE & VERBOSE_API)
               }
         void GLAPIENTRY
   _mesa_LogicOp_no_error(GLenum opcode)
   {
      GET_CURRENT_CONTEXT(ctx);
      }
         void GLAPIENTRY
   _mesa_IndexMask( GLuint mask )
   {
               if (ctx->Color.IndexMask == mask)
            FLUSH_VERTICES(ctx, 0, GL_COLOR_BUFFER_BIT);
   ctx->NewDriverState |= ST_NEW_BLEND;
      }
         /**
   * Enable or disable writing of frame buffer color components.
   *
   * \param red whether to mask writing of the red color component.
   * \param green whether to mask writing of the green color component.
   * \param blue whether to mask writing of the blue color component.
   * \param alpha whether to mask writing of the alpha color component.
   *
   * \sa glColorMask().
   *
   * Sets the appropriate value of gl_colorbuffer_attrib::ColorMask.  On a
   * change, flushes the vertices and notifies the driver via the
   * dd_function_table::ColorMask callback.
   */
   void GLAPIENTRY
   _mesa_ColorMask( GLboolean red, GLboolean green,
         {
               if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "glColorMask(%d, %d, %d, %d)\n",
         GLbitfield mask = (!!red) |
                              if (ctx->Color.ColorMask == mask)
            FLUSH_VERTICES(ctx, 0, GL_COLOR_BUFFER_BIT);
   ctx->NewDriverState |= ST_NEW_BLEND;
   ctx->Color.ColorMask = mask;
      }
         /**
   * For GL_EXT_draw_buffers2 and GL3
   */
   void GLAPIENTRY
   _mesa_ColorMaski(GLuint buf, GLboolean red, GLboolean green,
         {
               if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "glColorMaski %u %d %d %d %d\n",
         if (buf >= ctx->Const.MaxDrawBuffers) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glColorMaski(buf=%u)", buf);
               GLbitfield mask = (!!red) |
                        if (GET_COLORMASK(ctx->Color.ColorMask, buf) == mask)
            FLUSH_VERTICES(ctx, 0, GL_COLOR_BUFFER_BIT);
   ctx->NewDriverState |= ST_NEW_BLEND;
   ctx->Color.ColorMask &= ~(0xf << (4 * buf));
   ctx->Color.ColorMask |= mask << (4 * buf);
      }
         void GLAPIENTRY
   _mesa_ClampColor(GLenum target, GLenum clamp)
   {
               /* Check for both the extension and the GL version, since the Intel driver
   * does not advertise the extension in core profiles.
   */
   if (ctx->Version <= 30 && !ctx->Extensions.ARB_color_buffer_float) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glClampColor()");
               if (clamp != GL_TRUE && clamp != GL_FALSE && clamp != GL_FIXED_ONLY_ARB) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glClampColorARB(clamp)");
               switch (target) {
   case GL_CLAMP_VERTEX_COLOR_ARB:
      if (_mesa_is_desktop_gl_core(ctx))
         FLUSH_VERTICES(ctx, _NEW_LIGHT_STATE, GL_LIGHTING_BIT | GL_ENABLE_BIT);
   ctx->Light.ClampVertexColor = clamp;
   _mesa_update_clamp_vertex_color(ctx, ctx->DrawBuffer);
      case GL_CLAMP_FRAGMENT_COLOR_ARB:
      if (_mesa_is_desktop_gl_core(ctx))
         if (ctx->Color.ClampFragmentColor != clamp) {
      FLUSH_VERTICES(ctx, 0, GL_COLOR_BUFFER_BIT | GL_ENABLE_BIT);
   ctx->Color.ClampFragmentColor = clamp;
      }
      case GL_CLAMP_READ_COLOR_ARB:
      ctx->Color.ClampReadColor = clamp;
   ctx->PopAttribState |= GL_COLOR_BUFFER_BIT | GL_ENABLE_BIT;
      default:
         }
         invalid_enum:
      _mesa_error(ctx, GL_INVALID_ENUM, "glClampColor(%s)",
      }
      static GLboolean
   get_clamp_color(const struct gl_framebuffer *fb, GLenum clamp)
   {
      if (clamp == GL_TRUE || clamp == GL_FALSE)
            assert(clamp == GL_FIXED_ONLY);
   if (!fb)
               }
      GLboolean
   _mesa_get_clamp_fragment_color(const struct gl_context *ctx,
         {
         }
      GLboolean
   _mesa_get_clamp_vertex_color(const struct gl_context *ctx,
         {
         }
      GLboolean
   _mesa_get_clamp_read_color(const struct gl_context *ctx,
         {
         }
      /**
   * Update the ctx->Color._ClampFragmentColor field
   */
   void
   _mesa_update_clamp_fragment_color(struct gl_context *ctx,
         {
               /* Don't clamp if:
   * - there is no colorbuffer
   * - all colorbuffers are unsigned normalized, so clamping has no effect
   * - there is an integer colorbuffer
   */
   if (!drawFb || !drawFb->_HasSNormOrFloatColorBuffer ||
      drawFb->_IntegerBuffers)
      else
            if (ctx->Color._ClampFragmentColor == clamp)
            ctx->NewState |= _NEW_FRAG_CLAMP; /* for state constants */
   ctx->NewDriverState |= ctx->DriverFlags.NewFragClamp;
      }
      /**
   * Update the ctx->Color._ClampVertexColor field
   */
   void
   _mesa_update_clamp_vertex_color(struct gl_context *ctx,
         {
      ctx->Light._ClampVertexColor =
      }
      /**********************************************************************/
   /** \name Initialization */
   /*@{*/
      /**
   * Initialization of the context's Color attribute group.
   *
   * \param ctx GL context.
   *
   * Initializes the related fields in the context color attribute group,
   * __struct gl_contextRec::Color.
   */
   void _mesa_init_color( struct gl_context * ctx )
   {
               /* Color buffer group */
   ctx->Color.IndexMask = ~0u;
   ctx->Color.ColorMask = 0xffffffff;
   ctx->Color.ClearIndex = 0;
   ASSIGN_4V( ctx->Color.ClearColor.f, 0, 0, 0, 0 );
   ctx->Color.AlphaEnabled = GL_FALSE;
   ctx->Color.AlphaFunc = GL_ALWAYS;
   ctx->Color.AlphaRef = 0;
   ctx->Color.BlendEnabled = 0x0;
   for (i = 0; i < ARRAY_SIZE(ctx->Color.Blend); i++) {
      ctx->Color.Blend[i].SrcRGB = GL_ONE;
   ctx->Color.Blend[i].DstRGB = GL_ZERO;
   ctx->Color.Blend[i].SrcA = GL_ONE;
   ctx->Color.Blend[i].DstA = GL_ZERO;
   ctx->Color.Blend[i].EquationRGB = GL_FUNC_ADD;
      }
   ASSIGN_4V( ctx->Color.BlendColor, 0.0, 0.0, 0.0, 0.0 );
   ASSIGN_4V( ctx->Color.BlendColorUnclamped, 0.0, 0.0, 0.0, 0.0 );
   ctx->Color.IndexLogicOpEnabled = GL_FALSE;
   ctx->Color.ColorLogicOpEnabled = GL_FALSE;
   ctx->Color.LogicOp = GL_COPY;
   ctx->Color._LogicOp = COLOR_LOGICOP_COPY;
            /* GL_FRONT is not possible on GLES. Instead GL_BACK will render to either
   * the front or the back buffer depending on the config */
   if (ctx->Visual.doubleBufferMode || _mesa_is_gles(ctx)) {
         }
   else {
                  ctx->Color.ClampFragmentColor = _mesa_is_desktop_gl_compat(ctx) ?
         ctx->Color._ClampFragmentColor = GL_FALSE;
            /* GLES 1/2/3 behaves as though GL_FRAMEBUFFER_SRGB is always enabled
   * if EGL_KHR_gl_colorspace has been used to request sRGB.
   */
               }
      /*@}*/
