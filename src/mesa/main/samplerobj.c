   /*
   * Mesa 3-D graphics library
   *
   * Copyright (C) 2011  VMware, Inc.  All Rights Reserved.
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
   * \file samplerobj.c
   * \brief Functions for the GL_ARB_sampler_objects extension.
   * \author Brian Paul
   */
         #include "util/glheader.h"
   #include "main/context.h"
   #include "main/enums.h"
   #include "main/hash.h"
   #include "main/macros.h"
   #include "main/mtypes.h"
   #include "main/samplerobj.h"
   #include "main/texturebindless.h"
   #include "util/u_memory.h"
   #include "api_exec_decl.h"
      /* Take advantage of how the enums are defined. */
   const enum pipe_tex_wrap wrap_to_gallium_table[32] = {
      [GL_REPEAT & 0x1f] = PIPE_TEX_WRAP_REPEAT,
   [GL_CLAMP & 0x1f] = PIPE_TEX_WRAP_CLAMP,
   [GL_CLAMP_TO_EDGE & 0x1f] = PIPE_TEX_WRAP_CLAMP_TO_EDGE,
   [GL_CLAMP_TO_BORDER & 0x1f] = PIPE_TEX_WRAP_CLAMP_TO_BORDER,
   [GL_MIRRORED_REPEAT & 0x1f] = PIPE_TEX_WRAP_MIRROR_REPEAT,
   [GL_MIRROR_CLAMP_EXT & 0x1f] = PIPE_TEX_WRAP_MIRROR_CLAMP,
   [GL_MIRROR_CLAMP_TO_EDGE & 0x1f] = PIPE_TEX_WRAP_MIRROR_CLAMP_TO_EDGE,
      };
      struct gl_sampler_object *
   _mesa_lookup_samplerobj(struct gl_context *ctx, GLuint name)
   {
      if (name == 0)
         else
      return (struct gl_sampler_object *)
   }
      static inline struct gl_sampler_object *
   lookup_samplerobj_locked(struct gl_context *ctx, GLuint name)
   {
      return (struct gl_sampler_object *)
      }
      static void
   delete_sampler_object(struct gl_context *ctx,
         {
      _mesa_delete_sampler_handles(ctx, sampObj);
   free(sampObj->Label);
      }
      /**
   * Handle reference counting.
   */
   void
   _mesa_reference_sampler_object_(struct gl_context *ctx,
               {
               if (*ptr) {
      /* Unreference the old sampler */
                     if (p_atomic_dec_zero(&oldSamp->RefCount))
               if (samp) {
      /* reference new sampler */
                           }
         /**
   * Initialize the fields of the given sampler object.
   */
   static void
   _mesa_init_sampler_object(struct gl_sampler_object *sampObj, GLuint name)
   {
      sampObj->Name = name;
   sampObj->RefCount = 1;
   sampObj->Attrib.WrapS = GL_REPEAT;
   sampObj->Attrib.WrapT = GL_REPEAT;
   sampObj->Attrib.WrapR = GL_REPEAT;
   sampObj->Attrib.state.wrap_s = PIPE_TEX_WRAP_REPEAT;
   sampObj->Attrib.state.wrap_t = PIPE_TEX_WRAP_REPEAT;
   sampObj->Attrib.state.wrap_r = PIPE_TEX_WRAP_REPEAT;
   sampObj->Attrib.MinFilter = GL_NEAREST_MIPMAP_LINEAR;
   sampObj->Attrib.MagFilter = GL_LINEAR;
   sampObj->Attrib.state.min_img_filter = PIPE_TEX_FILTER_NEAREST;
   sampObj->Attrib.state.min_mip_filter = PIPE_TEX_MIPFILTER_LINEAR;
   sampObj->Attrib.state.mag_img_filter = PIPE_TEX_FILTER_LINEAR;
   sampObj->Attrib.state.border_color.f[0] = 0;
   sampObj->Attrib.state.border_color.f[1] = 0;
   sampObj->Attrib.state.border_color.f[2] = 0;
   sampObj->Attrib.state.border_color.f[3] = 0;
   _mesa_update_is_border_color_nonzero(sampObj);
   sampObj->Attrib.MinLod = -1000.0F;
   sampObj->Attrib.MaxLod = 1000.0F;
   sampObj->Attrib.state.min_lod = 0; /* Gallium doesn't allow negative numbers */
   sampObj->Attrib.state.max_lod = 1000;
   sampObj->Attrib.LodBias = 0.0F;
   sampObj->Attrib.state.lod_bias = 0;
   sampObj->Attrib.MaxAnisotropy = 1.0F;
   sampObj->Attrib.state.max_anisotropy = 0; /* Gallium uses 0 instead of 1. */
   sampObj->Attrib.CompareMode = GL_NONE;
   sampObj->Attrib.CompareFunc = GL_LEQUAL;
   sampObj->Attrib.state.compare_mode = PIPE_TEX_COMPARE_NONE;
   sampObj->Attrib.state.compare_func = PIPE_FUNC_LEQUAL;
   sampObj->Attrib.sRGBDecode = GL_DECODE_EXT;
   sampObj->Attrib.CubeMapSeamless = GL_FALSE;
   sampObj->Attrib.state.seamless_cube_map = false;
   sampObj->Attrib.ReductionMode = GL_WEIGHTED_AVERAGE_EXT;
   sampObj->Attrib.state.reduction_mode = PIPE_TEX_REDUCTION_WEIGHTED_AVERAGE;
            /* GL_ARB_bindless_texture */
      }
      static struct gl_sampler_object *
   _mesa_new_sampler_object(struct gl_context *ctx, GLuint name)
   {
      struct gl_sampler_object *sampObj = CALLOC_STRUCT(gl_sampler_object);
   if (sampObj) {
         }
      }
      static void
   create_samplers(struct gl_context *ctx, GLsizei count, GLuint *samplers,
         {
               if (!samplers)
                              /* Insert the ID and pointer to new sampler object into hash table */
   for (i = 0; i < count; i++) {
               sampObj = _mesa_new_sampler_object(ctx, samplers[i]);
   if (!sampObj) {
      _mesa_HashUnlockMutex(ctx->Shared->SamplerObjects);
   _mesa_error(ctx, GL_OUT_OF_MEMORY, "%s", caller);
               _mesa_HashInsertLocked(ctx->Shared->SamplerObjects, samplers[i],
                  }
      static void
   create_samplers_err(struct gl_context *ctx, GLsizei count, GLuint *samplers,
         {
         if (MESA_VERBOSE & VERBOSE_API)
            if (count < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "%s(n<0)", caller);
                  }
      void GLAPIENTRY
   _mesa_GenSamplers_no_error(GLsizei count, GLuint *samplers)
   {
      GET_CURRENT_CONTEXT(ctx);
      }
      void GLAPIENTRY
   _mesa_GenSamplers(GLsizei count, GLuint *samplers)
   {
      GET_CURRENT_CONTEXT(ctx);
      }
      void GLAPIENTRY
   _mesa_CreateSamplers_no_error(GLsizei count, GLuint *samplers)
   {
      GET_CURRENT_CONTEXT(ctx);
      }
      void GLAPIENTRY
   _mesa_CreateSamplers(GLsizei count, GLuint *samplers)
   {
      GET_CURRENT_CONTEXT(ctx);
      }
         static void
   delete_samplers(struct gl_context *ctx, GLsizei count, const GLuint *samplers)
   {
                        for (GLsizei i = 0; i < count; i++) {
      if (samplers[i]) {
      GLuint j;
                  if (sampObj) {
      /* If the sampler is currently bound, unbind it. */
   for (j = 0; j < ctx->Const.MaxCombinedTextureImageUnits; j++) {
      if (ctx->Texture.Unit[j].Sampler == sampObj) {
      FLUSH_VERTICES(ctx, _NEW_TEXTURE_OBJECT, GL_TEXTURE_BIT);
                  /* The ID is immediately freed for re-use */
   _mesa_HashRemoveLocked(ctx->Shared->SamplerObjects, samplers[i]);
   /* But the object exists until its reference count goes to zero */
                        }
         void GLAPIENTRY
   _mesa_DeleteSamplers_no_error(GLsizei count, const GLuint *samplers)
   {
      GET_CURRENT_CONTEXT(ctx);
      }
         void GLAPIENTRY
   _mesa_DeleteSamplers(GLsizei count, const GLuint *samplers)
   {
               if (count < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glDeleteSamplers(count)");
                  }
         GLboolean GLAPIENTRY
   _mesa_IsSampler(GLuint sampler)
   {
                           }
      void
   _mesa_bind_sampler(struct gl_context *ctx, GLuint unit,
         {
      if (ctx->Texture.Unit[unit].Sampler != sampObj) {
                  _mesa_reference_sampler_object(ctx, &ctx->Texture.Unit[unit].Sampler,
      }
      static ALWAYS_INLINE void
   bind_sampler(struct gl_context *ctx, GLuint unit, GLuint sampler, bool no_error)
   {
               if (sampler == 0) {
      /* Use the default sampler object, the one contained in the texture
   * object.
   */
      } else {
      /* user-defined sampler object */
   sampObj = _mesa_lookup_samplerobj(ctx, sampler);
   if (!no_error && !sampObj) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glBindSampler(sampler)");
                  /* bind new sampler */
      }
      void GLAPIENTRY
   _mesa_BindSampler_no_error(GLuint unit, GLuint sampler)
   {
      GET_CURRENT_CONTEXT(ctx);
      }
      void GLAPIENTRY
   _mesa_BindSampler(GLuint unit, GLuint sampler)
   {
               if (unit >= ctx->Const.MaxCombinedTextureImageUnits) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glBindSampler(unit %u)", unit);
                  }
         static ALWAYS_INLINE void
   bind_samplers(struct gl_context *ctx, GLuint first, GLsizei count,
         {
                        if (samplers) {
      /* Note that the error semantics for multi-bind commands differ from
   * those of other GL commands.
   *
   * The Issues section in the ARB_multi_bind spec says:
   *
   *    "(11) Typically, OpenGL specifies that if an error is generated by
   *          a command, that command has no effect.  This is somewhat
   *          unfortunate for multi-bind commands, because it would require
   *          a first pass to scan the entire list of bound objects for
   *          errors and then a second pass to actually perform the
   *          bindings.  Should we have different error semantics?
   *
   *       RESOLVED:  Yes.  In this specification, when the parameters for
   *       one of the <count> binding points are invalid, that binding
   *       point is not updated and an error will be generated.  However,
   *       other binding points in the same command will be updated if
   *       their parameters are valid and no other error occurs."
                     for (i = 0; i < count; i++) {
      const GLuint unit = first + i;
   struct gl_sampler_object * const currentSampler =
                  if (samplers[i] != 0) {
      if (currentSampler && currentSampler->Name == samplers[i])
                        /* The ARB_multi_bind spec says:
   *
   *    "An INVALID_OPERATION error is generated if any value
   *     in <samplers> is not zero or the name of an existing
   *     sampler object (per binding)."
   */
   if (!no_error && !sampObj) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
               "glBindSamplers(samplers[%d]=%u is not zero or "
         } else {
                  /* Bind the new sampler */
   if (sampObj != currentSampler) {
      _mesa_reference_sampler_object(ctx,
               ctx->NewState |= _NEW_TEXTURE_OBJECT;
                     } else {
      /* Unbind all samplers in the range <first> through <first>+<count>-1 */
   for (i = 0; i < count; i++) {
               if (ctx->Texture.Unit[unit].Sampler) {
      _mesa_reference_sampler_object(ctx,
               ctx->NewState |= _NEW_TEXTURE_OBJECT;
               }
         void GLAPIENTRY
   _mesa_BindSamplers_no_error(GLuint first, GLsizei count, const GLuint *samplers)
   {
      GET_CURRENT_CONTEXT(ctx);
      }
         void GLAPIENTRY
   _mesa_BindSamplers(GLuint first, GLsizei count, const GLuint *samplers)
   {
               /* The ARB_multi_bind spec says:
   *
   *   "An INVALID_OPERATION error is generated if <first> + <count> is
   *    greater than the number of texture image units supported by
   *    the implementation."
   */
   if (first + count > ctx->Const.MaxCombinedTextureImageUnits) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
               "glBindSamplers(first=%u + count=%d > the value of "
                  }
         /**
   * Check if a coordinate wrap mode is legal.
   * \return GL_TRUE if legal, GL_FALSE otherwise
   */
   static GLboolean
   validate_texture_wrap_mode(struct gl_context *ctx, GLenum wrap)
   {
               bool mirror_clamp =
      _mesa_has_ATI_texture_mirror_once(ctx) ||
         bool mirror_clamp_to_edge =
      _mesa_has_ARB_texture_mirror_clamp_to_edge(ctx) ||
   _mesa_has_EXT_texture_mirror_clamp_to_edge(ctx) ||
         switch (wrap) {
   case GL_CLAMP:
      /* From GL 3.0 specification section E.1 "Profiles and Deprecated
   * Features of OpenGL 3.0":
   *
   * - Texture wrap mode CLAMP - CLAMP is no longer accepted as a value of
   *   texture parameters TEXTURE_WRAP_S, TEXTURE_WRAP_T, or
   *   TEXTURE_WRAP_R.
   */
      case GL_CLAMP_TO_EDGE:
   case GL_REPEAT:
   case GL_MIRRORED_REPEAT:
   case GL_CLAMP_TO_BORDER:
         case GL_MIRROR_CLAMP_EXT:
         case GL_MIRROR_CLAMP_TO_EDGE_EXT:
         case GL_MIRROR_CLAMP_TO_BORDER_EXT:
         default:
            }
         /**
   * This is called just prior to changing any sampler object state.
   */
   static inline void
   flush(struct gl_context *ctx)
   {
         }
      #define INVALID_PARAM 0x100
   #define INVALID_PNAME 0x101
   #define INVALID_VALUE 0x102
      static GLuint
   set_sampler_wrap_s(struct gl_context *ctx, struct gl_sampler_object *samp,
         {
      if (samp->Attrib.WrapS == param)
         if (validate_texture_wrap_mode(ctx, param)) {
      flush(ctx);
   update_sampler_gl_clamp(ctx, samp, is_wrap_gl_clamp(samp->Attrib.WrapS), is_wrap_gl_clamp(param), WRAP_S);
   samp->Attrib.WrapS = param;
   samp->Attrib.state.wrap_s = wrap_to_gallium(param);
   _mesa_lower_gl_clamp(ctx, samp);
      }
      }
         static GLuint
   set_sampler_wrap_t(struct gl_context *ctx, struct gl_sampler_object *samp,
         {
      if (samp->Attrib.WrapT == param)
         if (validate_texture_wrap_mode(ctx, param)) {
      flush(ctx);
   update_sampler_gl_clamp(ctx, samp, is_wrap_gl_clamp(samp->Attrib.WrapT), is_wrap_gl_clamp(param), WRAP_T);
   samp->Attrib.WrapT = param;
   samp->Attrib.state.wrap_t = wrap_to_gallium(param);
   _mesa_lower_gl_clamp(ctx, samp);
      }
      }
         static GLuint
   set_sampler_wrap_r(struct gl_context *ctx, struct gl_sampler_object *samp,
         {
      if (samp->Attrib.WrapR == param)
         if (validate_texture_wrap_mode(ctx, param)) {
      flush(ctx);
   update_sampler_gl_clamp(ctx, samp, is_wrap_gl_clamp(samp->Attrib.WrapR), is_wrap_gl_clamp(param), WRAP_R);
   samp->Attrib.WrapR = param;
   samp->Attrib.state.wrap_r = wrap_to_gallium(param);
   _mesa_lower_gl_clamp(ctx, samp);
      }
      }
      static GLuint
   set_sampler_min_filter(struct gl_context *ctx, struct gl_sampler_object *samp,
         {
      if (samp->Attrib.MinFilter == param)
            switch (param) {
   case GL_NEAREST:
   case GL_LINEAR:
   case GL_NEAREST_MIPMAP_NEAREST:
   case GL_LINEAR_MIPMAP_NEAREST:
   case GL_NEAREST_MIPMAP_LINEAR:
   case GL_LINEAR_MIPMAP_LINEAR:
      flush(ctx);
   samp->Attrib.MinFilter = param;
   samp->Attrib.state.min_img_filter = filter_to_gallium(param);
   samp->Attrib.state.min_mip_filter = mipfilter_to_gallium(param);
   _mesa_lower_gl_clamp(ctx, samp);
      default:
            }
         static GLuint
   set_sampler_mag_filter(struct gl_context *ctx, struct gl_sampler_object *samp,
         {
      if (samp->Attrib.MagFilter == param)
            switch (param) {
   case GL_NEAREST:
   case GL_LINEAR:
      flush(ctx);
   samp->Attrib.MagFilter = param;
   samp->Attrib.state.mag_img_filter = filter_to_gallium(param);
   _mesa_lower_gl_clamp(ctx, samp);
      default:
            }
         static GLuint
   set_sampler_lod_bias(struct gl_context *ctx, struct gl_sampler_object *samp,
         {
      if (samp->Attrib.LodBias == param)
            flush(ctx);
   samp->Attrib.LodBias = param;
   samp->Attrib.state.lod_bias = util_quantize_lod_bias(param);
      }
         static GLuint
   set_sampler_border_colorf(struct gl_context *ctx,
               {
      flush(ctx);
   memcpy(samp->Attrib.state.border_color.f, params, 4 * sizeof(float));
   _mesa_update_is_border_color_nonzero(samp);
      }
         static GLuint
   set_sampler_border_colori(struct gl_context *ctx,
               {
      flush(ctx);
   memcpy(samp->Attrib.state.border_color.i, params, 4 * sizeof(float));
   _mesa_update_is_border_color_nonzero(samp);
      }
         static GLuint
   set_sampler_border_colorui(struct gl_context *ctx,
               {
      flush(ctx);
   memcpy(samp->Attrib.state.border_color.ui, params, 4 * sizeof(float));
   _mesa_update_is_border_color_nonzero(samp);
      }
         static GLuint
   set_sampler_min_lod(struct gl_context *ctx, struct gl_sampler_object *samp,
         {
      if (samp->Attrib.MinLod == param)
            flush(ctx);
   samp->Attrib.MinLod = param;
               }
         static GLuint
   set_sampler_max_lod(struct gl_context *ctx, struct gl_sampler_object *samp,
         {
      if (samp->Attrib.MaxLod == param)
            flush(ctx);
   samp->Attrib.MaxLod = param;
   samp->Attrib.state.max_lod = param;
      }
         static GLuint
   set_sampler_compare_mode(struct gl_context *ctx,
         {
      /* If GL_ARB_shadow is not supported, don't report an error.  The
   * sampler object extension spec isn't clear on this extension interaction.
   * Silences errors with Wine on older GPUs such as R200.
   */
   if (!ctx->Extensions.ARB_shadow)
            if (samp->Attrib.CompareMode == param)
            if (param == GL_NONE ||
      param == GL_COMPARE_R_TO_TEXTURE_ARB) {
   flush(ctx);
   samp->Attrib.CompareMode = param;
                  }
         static GLuint
   set_sampler_compare_func(struct gl_context *ctx,
         {
      /* If GL_ARB_shadow is not supported, don't report an error.  The
   * sampler object extension spec isn't clear on this extension interaction.
   * Silences errors with Wine on older GPUs such as R200.
   */
   if (!ctx->Extensions.ARB_shadow)
            if (samp->Attrib.CompareFunc == param)
            switch (param) {
   case GL_LEQUAL:
   case GL_GEQUAL:
   case GL_EQUAL:
   case GL_NOTEQUAL:
   case GL_LESS:
   case GL_GREATER:
   case GL_ALWAYS:
   case GL_NEVER:
      flush(ctx);
   samp->Attrib.CompareFunc = param;
   samp->Attrib.state.compare_func = func_to_gallium(param);
      default:
            }
         static GLuint
   set_sampler_max_anisotropy(struct gl_context *ctx,
         {
      if (!ctx->Extensions.EXT_texture_filter_anisotropic)
            if (samp->Attrib.MaxAnisotropy == param)
            if (param < 1.0F)
            flush(ctx);
   /* clamp to max, that's what NVIDIA does */
   samp->Attrib.MaxAnisotropy = MIN2(param, ctx->Const.MaxTextureMaxAnisotropy);
   /* gallium sets 0 for 1 */
   samp->Attrib.state.max_anisotropy = samp->Attrib.MaxAnisotropy == 1 ?
            }
         static GLuint
   set_sampler_cube_map_seamless(struct gl_context *ctx,
         {
      if (!_mesa_is_desktop_gl(ctx)
      || !ctx->Extensions.AMD_seamless_cubemap_per_texture)
         if (samp->Attrib.CubeMapSeamless == param)
            if (param != GL_TRUE && param != GL_FALSE)
            flush(ctx);
   samp->Attrib.CubeMapSeamless = param;
   samp->Attrib.state.seamless_cube_map = param;
      }
      static GLuint
   set_sampler_srgb_decode(struct gl_context *ctx,
         {
      if (!ctx->Extensions.EXT_texture_sRGB_decode)
            if (samp->Attrib.sRGBDecode == param)
            /* The EXT_texture_sRGB_decode spec says:
   *
   *    "INVALID_ENUM is generated if the <pname> parameter of
   *     TexParameter[i,f,Ii,Iui][v][EXT],
   *     MultiTexParameter[i,f,Ii,Iui][v]EXT,
   *     TextureParameter[i,f,Ii,Iui][v]EXT, SamplerParameter[i,f,Ii,Iui][v]
   *     is TEXTURE_SRGB_DECODE_EXT when the <param> parameter is not one of
   *     DECODE_EXT or SKIP_DECODE_EXT.
   *
   * Returning INVALID_PARAM makes that happen.
   */
   if (param != GL_DECODE_EXT && param != GL_SKIP_DECODE_EXT)
            flush(ctx);
   samp->Attrib.sRGBDecode = param;
      }
      static GLuint
   set_sampler_reduction_mode(struct gl_context *ctx,
         {
      if (!ctx->Extensions.EXT_texture_filter_minmax &&
      !_mesa_has_ARB_texture_filter_minmax(ctx))
         if (samp->Attrib.ReductionMode == param)
            if (param != GL_WEIGHTED_AVERAGE_EXT && param != GL_MIN && param != GL_MAX)
            flush(ctx);
   samp->Attrib.ReductionMode = param;
   samp->Attrib.state.reduction_mode = reduction_to_gallium(param);
      }
      static struct gl_sampler_object *
   sampler_parameter_error_check(struct gl_context *ctx, GLuint sampler,
         {
               sampObj = _mesa_lookup_samplerobj(ctx, sampler);
   if (!sampObj) {
      /* OpenGL 4.5 spec, section "8.2 Sampler Objects", page 176 of the PDF
   * states:
   *
   *    "An INVALID_OPERATION error is generated if sampler is not the name
   *    of a sampler object previously returned from a call to
   *    GenSamplers."
   */
   _mesa_error(ctx, GL_INVALID_OPERATION, "%s(invalid sampler)", name);
               if (!get && sampObj->HandleAllocated) {
      /* The ARB_bindless_texture spec says:
   *
   * "The error INVALID_OPERATION is generated by SamplerParameter* if
   *  <sampler> identifies a sampler object referenced by one or more
   *  texture handles."
   */
   _mesa_error(ctx, GL_INVALID_OPERATION, "%s(immutable sampler)", name);
                  }
      void GLAPIENTRY
   _mesa_SamplerParameteri(GLuint sampler, GLenum pname, GLint param)
   {
      struct gl_sampler_object *sampObj;
   GLuint res;
            sampObj = sampler_parameter_error_check(ctx, sampler, false,
         if (!sampObj)
            switch (pname) {
   case GL_TEXTURE_WRAP_S:
      res = set_sampler_wrap_s(ctx, sampObj, param);
      case GL_TEXTURE_WRAP_T:
      res = set_sampler_wrap_t(ctx, sampObj, param);
      case GL_TEXTURE_WRAP_R:
      res = set_sampler_wrap_r(ctx, sampObj, param);
      case GL_TEXTURE_MIN_FILTER:
      res = set_sampler_min_filter(ctx, sampObj, param);
      case GL_TEXTURE_MAG_FILTER:
      res = set_sampler_mag_filter(ctx, sampObj, param);
      case GL_TEXTURE_MIN_LOD:
      res = set_sampler_min_lod(ctx, sampObj, (GLfloat) param);
      case GL_TEXTURE_MAX_LOD:
      res = set_sampler_max_lod(ctx, sampObj, (GLfloat) param);
      case GL_TEXTURE_LOD_BIAS:
      res = set_sampler_lod_bias(ctx, sampObj, (GLfloat) param);
      case GL_TEXTURE_COMPARE_MODE:
      res = set_sampler_compare_mode(ctx, sampObj, param);
      case GL_TEXTURE_COMPARE_FUNC:
      res = set_sampler_compare_func(ctx, sampObj, param);
      case GL_TEXTURE_MAX_ANISOTROPY_EXT:
      res = set_sampler_max_anisotropy(ctx, sampObj, (GLfloat) param);
      case GL_TEXTURE_CUBE_MAP_SEAMLESS:
      res = set_sampler_cube_map_seamless(ctx, sampObj, param);
      case GL_TEXTURE_SRGB_DECODE_EXT:
      res = set_sampler_srgb_decode(ctx, sampObj, param);
      case GL_TEXTURE_REDUCTION_MODE_EXT:
      res = set_sampler_reduction_mode(ctx, sampObj, param);
      case GL_TEXTURE_BORDER_COLOR:
         default:
                  switch (res) {
   case GL_FALSE:
      /* no change */
      case GL_TRUE:
      /* state change - we do nothing special at this time */
      case INVALID_PNAME:
      _mesa_error(ctx, GL_INVALID_ENUM, "glSamplerParameteri(pname=%s)\n",
            case INVALID_PARAM:
      _mesa_error(ctx, GL_INVALID_ENUM, "glSamplerParameteri(param=%d)\n",
            case INVALID_VALUE:
      _mesa_error(ctx, GL_INVALID_VALUE, "glSamplerParameteri(param=%d)\n",
            default:
            }
         void GLAPIENTRY
   _mesa_SamplerParameterf(GLuint sampler, GLenum pname, GLfloat param)
   {
      struct gl_sampler_object *sampObj;
   GLuint res;
            sampObj = sampler_parameter_error_check(ctx, sampler, false,
         if (!sampObj)
            switch (pname) {
   case GL_TEXTURE_WRAP_S:
      res = set_sampler_wrap_s(ctx, sampObj, (GLint) param);
      case GL_TEXTURE_WRAP_T:
      res = set_sampler_wrap_t(ctx, sampObj, (GLint) param);
      case GL_TEXTURE_WRAP_R:
      res = set_sampler_wrap_r(ctx, sampObj, (GLint) param);
      case GL_TEXTURE_MIN_FILTER:
      res = set_sampler_min_filter(ctx, sampObj, (GLint) param);
      case GL_TEXTURE_MAG_FILTER:
      res = set_sampler_mag_filter(ctx, sampObj, (GLint) param);
      case GL_TEXTURE_MIN_LOD:
      res = set_sampler_min_lod(ctx, sampObj, param);
      case GL_TEXTURE_MAX_LOD:
      res = set_sampler_max_lod(ctx, sampObj, param);
      case GL_TEXTURE_LOD_BIAS:
      res = set_sampler_lod_bias(ctx, sampObj, param);
      case GL_TEXTURE_COMPARE_MODE:
      res = set_sampler_compare_mode(ctx, sampObj, (GLint) param);
      case GL_TEXTURE_COMPARE_FUNC:
      res = set_sampler_compare_func(ctx, sampObj, (GLint) param);
      case GL_TEXTURE_MAX_ANISOTROPY_EXT:
      res = set_sampler_max_anisotropy(ctx, sampObj, param);
      case GL_TEXTURE_CUBE_MAP_SEAMLESS:
      res = set_sampler_cube_map_seamless(ctx, sampObj, (GLboolean) param);
      case GL_TEXTURE_SRGB_DECODE_EXT:
      res = set_sampler_srgb_decode(ctx, sampObj, (GLenum) param);
      case GL_TEXTURE_REDUCTION_MODE_EXT:
      res = set_sampler_reduction_mode(ctx, sampObj, (GLenum) param);
      case GL_TEXTURE_BORDER_COLOR:
         default:
                  switch (res) {
   case GL_FALSE:
      /* no change */
      case GL_TRUE:
      /* state change - we do nothing special at this time */
      case INVALID_PNAME:
      _mesa_error(ctx, GL_INVALID_ENUM, "glSamplerParameterf(pname=%s)\n",
            case INVALID_PARAM:
      _mesa_error(ctx, GL_INVALID_ENUM, "glSamplerParameterf(param=%f)\n",
            case INVALID_VALUE:
      _mesa_error(ctx, GL_INVALID_VALUE, "glSamplerParameterf(param=%f)\n",
            default:
            }
      void GLAPIENTRY
   _mesa_SamplerParameteriv(GLuint sampler, GLenum pname, const GLint *params)
   {
      struct gl_sampler_object *sampObj;
   GLuint res;
            sampObj = sampler_parameter_error_check(ctx, sampler, false,
         if (!sampObj)
            switch (pname) {
   case GL_TEXTURE_WRAP_S:
      res = set_sampler_wrap_s(ctx, sampObj, params[0]);
      case GL_TEXTURE_WRAP_T:
      res = set_sampler_wrap_t(ctx, sampObj, params[0]);
      case GL_TEXTURE_WRAP_R:
      res = set_sampler_wrap_r(ctx, sampObj, params[0]);
      case GL_TEXTURE_MIN_FILTER:
      res = set_sampler_min_filter(ctx, sampObj, params[0]);
      case GL_TEXTURE_MAG_FILTER:
      res = set_sampler_mag_filter(ctx, sampObj, params[0]);
      case GL_TEXTURE_MIN_LOD:
      res = set_sampler_min_lod(ctx, sampObj, (GLfloat) params[0]);
      case GL_TEXTURE_MAX_LOD:
      res = set_sampler_max_lod(ctx, sampObj, (GLfloat) params[0]);
      case GL_TEXTURE_LOD_BIAS:
      res = set_sampler_lod_bias(ctx, sampObj, (GLfloat) params[0]);
      case GL_TEXTURE_COMPARE_MODE:
      res = set_sampler_compare_mode(ctx, sampObj, params[0]);
      case GL_TEXTURE_COMPARE_FUNC:
      res = set_sampler_compare_func(ctx, sampObj, params[0]);
      case GL_TEXTURE_MAX_ANISOTROPY_EXT:
      res = set_sampler_max_anisotropy(ctx, sampObj, (GLfloat) params[0]);
      case GL_TEXTURE_CUBE_MAP_SEAMLESS:
      res = set_sampler_cube_map_seamless(ctx, sampObj, params[0]);
      case GL_TEXTURE_SRGB_DECODE_EXT:
      res = set_sampler_srgb_decode(ctx, sampObj, params[0]);
      case GL_TEXTURE_REDUCTION_MODE_EXT:
      res = set_sampler_reduction_mode(ctx, sampObj, params[0]);
      case GL_TEXTURE_BORDER_COLOR:
      {
      GLfloat c[4];
   c[0] = INT_TO_FLOAT(params[0]);
   c[1] = INT_TO_FLOAT(params[1]);
   c[2] = INT_TO_FLOAT(params[2]);
   c[3] = INT_TO_FLOAT(params[3]);
      }
      default:
                  switch (res) {
   case GL_FALSE:
      /* no change */
      case GL_TRUE:
      /* state change - we do nothing special at this time */
      case INVALID_PNAME:
      _mesa_error(ctx, GL_INVALID_ENUM, "glSamplerParameteriv(pname=%s)\n",
            case INVALID_PARAM:
      _mesa_error(ctx, GL_INVALID_ENUM, "glSamplerParameteriv(param=%d)\n",
            case INVALID_VALUE:
      _mesa_error(ctx, GL_INVALID_VALUE, "glSamplerParameteriv(param=%d)\n",
            default:
            }
      void GLAPIENTRY
   _mesa_SamplerParameterfv(GLuint sampler, GLenum pname, const GLfloat *params)
   {
      struct gl_sampler_object *sampObj;
   GLuint res;
            sampObj = sampler_parameter_error_check(ctx, sampler, false,
         if (!sampObj)
            switch (pname) {
   case GL_TEXTURE_WRAP_S:
      res = set_sampler_wrap_s(ctx, sampObj, (GLint) params[0]);
      case GL_TEXTURE_WRAP_T:
      res = set_sampler_wrap_t(ctx, sampObj, (GLint) params[0]);
      case GL_TEXTURE_WRAP_R:
      res = set_sampler_wrap_r(ctx, sampObj, (GLint) params[0]);
      case GL_TEXTURE_MIN_FILTER:
      res = set_sampler_min_filter(ctx, sampObj, (GLint) params[0]);
      case GL_TEXTURE_MAG_FILTER:
      res = set_sampler_mag_filter(ctx, sampObj, (GLint) params[0]);
      case GL_TEXTURE_MIN_LOD:
      res = set_sampler_min_lod(ctx, sampObj, params[0]);
      case GL_TEXTURE_MAX_LOD:
      res = set_sampler_max_lod(ctx, sampObj, params[0]);
      case GL_TEXTURE_LOD_BIAS:
      res = set_sampler_lod_bias(ctx, sampObj, params[0]);
      case GL_TEXTURE_COMPARE_MODE:
      res = set_sampler_compare_mode(ctx, sampObj, (GLint) params[0]);
      case GL_TEXTURE_COMPARE_FUNC:
      res = set_sampler_compare_func(ctx, sampObj, (GLint) params[0]);
      case GL_TEXTURE_MAX_ANISOTROPY_EXT:
      res = set_sampler_max_anisotropy(ctx, sampObj, params[0]);
      case GL_TEXTURE_CUBE_MAP_SEAMLESS:
      res = set_sampler_cube_map_seamless(ctx, sampObj, (GLboolean) params[0]);
      case GL_TEXTURE_SRGB_DECODE_EXT:
      res = set_sampler_srgb_decode(ctx, sampObj, (GLenum) params[0]);
      case GL_TEXTURE_REDUCTION_MODE_EXT:
      res = set_sampler_reduction_mode(ctx, sampObj, (GLenum) params[0]);
      case GL_TEXTURE_BORDER_COLOR:
      res = set_sampler_border_colorf(ctx, sampObj, params);
      default:
                  switch (res) {
   case GL_FALSE:
      /* no change */
      case GL_TRUE:
      /* state change - we do nothing special at this time */
      case INVALID_PNAME:
      _mesa_error(ctx, GL_INVALID_ENUM, "glSamplerParameterfv(pname=%s)\n",
            case INVALID_PARAM:
      _mesa_error(ctx, GL_INVALID_ENUM, "glSamplerParameterfv(param=%f)\n",
            case INVALID_VALUE:
      _mesa_error(ctx, GL_INVALID_VALUE, "glSamplerParameterfv(param=%f)\n",
            default:
            }
      void GLAPIENTRY
   _mesa_SamplerParameterIiv(GLuint sampler, GLenum pname, const GLint *params)
   {
      struct gl_sampler_object *sampObj;
   GLuint res;
            sampObj = sampler_parameter_error_check(ctx, sampler, false,
         if (!sampObj)
            switch (pname) {
   case GL_TEXTURE_WRAP_S:
      res = set_sampler_wrap_s(ctx, sampObj, params[0]);
      case GL_TEXTURE_WRAP_T:
      res = set_sampler_wrap_t(ctx, sampObj, params[0]);
      case GL_TEXTURE_WRAP_R:
      res = set_sampler_wrap_r(ctx, sampObj, params[0]);
      case GL_TEXTURE_MIN_FILTER:
      res = set_sampler_min_filter(ctx, sampObj, params[0]);
      case GL_TEXTURE_MAG_FILTER:
      res = set_sampler_mag_filter(ctx, sampObj, params[0]);
      case GL_TEXTURE_MIN_LOD:
      res = set_sampler_min_lod(ctx, sampObj, (GLfloat) params[0]);
      case GL_TEXTURE_MAX_LOD:
      res = set_sampler_max_lod(ctx, sampObj, (GLfloat) params[0]);
      case GL_TEXTURE_LOD_BIAS:
      res = set_sampler_lod_bias(ctx, sampObj, (GLfloat) params[0]);
      case GL_TEXTURE_COMPARE_MODE:
      res = set_sampler_compare_mode(ctx, sampObj, params[0]);
      case GL_TEXTURE_COMPARE_FUNC:
      res = set_sampler_compare_func(ctx, sampObj, params[0]);
      case GL_TEXTURE_MAX_ANISOTROPY_EXT:
      res = set_sampler_max_anisotropy(ctx, sampObj, (GLfloat) params[0]);
      case GL_TEXTURE_CUBE_MAP_SEAMLESS:
      res = set_sampler_cube_map_seamless(ctx, sampObj, params[0]);
      case GL_TEXTURE_SRGB_DECODE_EXT:
      res = set_sampler_srgb_decode(ctx, sampObj, (GLenum) params[0]);
      case GL_TEXTURE_REDUCTION_MODE_EXT:
      res = set_sampler_reduction_mode(ctx, sampObj, (GLenum) params[0]);
      case GL_TEXTURE_BORDER_COLOR:
      res = set_sampler_border_colori(ctx, sampObj, params);
      default:
                  switch (res) {
   case GL_FALSE:
      /* no change */
      case GL_TRUE:
      /* state change - we do nothing special at this time */
      case INVALID_PNAME:
      _mesa_error(ctx, GL_INVALID_ENUM, "glSamplerParameterIiv(pname=%s)\n",
            case INVALID_PARAM:
      _mesa_error(ctx, GL_INVALID_ENUM, "glSamplerParameterIiv(param=%d)\n",
            case INVALID_VALUE:
      _mesa_error(ctx, GL_INVALID_VALUE, "glSamplerParameterIiv(param=%d)\n",
            default:
            }
         void GLAPIENTRY
   _mesa_SamplerParameterIuiv(GLuint sampler, GLenum pname, const GLuint *params)
   {
      struct gl_sampler_object *sampObj;
   GLuint res;
            sampObj = sampler_parameter_error_check(ctx, sampler, false,
         if (!sampObj)
            switch (pname) {
   case GL_TEXTURE_WRAP_S:
      res = set_sampler_wrap_s(ctx, sampObj, params[0]);
      case GL_TEXTURE_WRAP_T:
      res = set_sampler_wrap_t(ctx, sampObj, params[0]);
      case GL_TEXTURE_WRAP_R:
      res = set_sampler_wrap_r(ctx, sampObj, params[0]);
      case GL_TEXTURE_MIN_FILTER:
      res = set_sampler_min_filter(ctx, sampObj, params[0]);
      case GL_TEXTURE_MAG_FILTER:
      res = set_sampler_mag_filter(ctx, sampObj, params[0]);
      case GL_TEXTURE_MIN_LOD:
      res = set_sampler_min_lod(ctx, sampObj, (GLfloat) params[0]);
      case GL_TEXTURE_MAX_LOD:
      res = set_sampler_max_lod(ctx, sampObj, (GLfloat) params[0]);
      case GL_TEXTURE_LOD_BIAS:
      res = set_sampler_lod_bias(ctx, sampObj, (GLfloat) params[0]);
      case GL_TEXTURE_COMPARE_MODE:
      res = set_sampler_compare_mode(ctx, sampObj, params[0]);
      case GL_TEXTURE_COMPARE_FUNC:
      res = set_sampler_compare_func(ctx, sampObj, params[0]);
      case GL_TEXTURE_MAX_ANISOTROPY_EXT:
      res = set_sampler_max_anisotropy(ctx, sampObj, (GLfloat) params[0]);
      case GL_TEXTURE_CUBE_MAP_SEAMLESS:
      res = set_sampler_cube_map_seamless(ctx, sampObj, params[0]);
      case GL_TEXTURE_SRGB_DECODE_EXT:
      res = set_sampler_srgb_decode(ctx, sampObj, (GLenum) params[0]);
      case GL_TEXTURE_REDUCTION_MODE_EXT:
      res = set_sampler_reduction_mode(ctx, sampObj, (GLenum) params[0]);
      case GL_TEXTURE_BORDER_COLOR:
      res = set_sampler_border_colorui(ctx, sampObj, params);
      default:
                  switch (res) {
   case GL_FALSE:
      /* no change */
      case GL_TRUE:
      /* state change - we do nothing special at this time */
      case INVALID_PNAME:
      _mesa_error(ctx, GL_INVALID_ENUM, "glSamplerParameterIuiv(pname=%s)\n",
            case INVALID_PARAM:
      _mesa_error(ctx, GL_INVALID_ENUM, "glSamplerParameterIuiv(param=%u)\n",
            case INVALID_VALUE:
      _mesa_error(ctx, GL_INVALID_VALUE, "glSamplerParameterIuiv(param=%u)\n",
            default:
            }
         void GLAPIENTRY
   _mesa_GetSamplerParameteriv(GLuint sampler, GLenum pname, GLint *params)
   {
      struct gl_sampler_object *sampObj;
            sampObj = sampler_parameter_error_check(ctx, sampler, true,
         if (!sampObj)
            switch (pname) {
   case GL_TEXTURE_WRAP_S:
      *params = sampObj->Attrib.WrapS;
      case GL_TEXTURE_WRAP_T:
      *params = sampObj->Attrib.WrapT;
      case GL_TEXTURE_WRAP_R:
      *params = sampObj->Attrib.WrapR;
      case GL_TEXTURE_MIN_FILTER:
      *params = sampObj->Attrib.MinFilter;
      case GL_TEXTURE_MAG_FILTER:
      *params = sampObj->Attrib.MagFilter;
      case GL_TEXTURE_MIN_LOD:
      /* GL spec 'Data Conversions' section specifies that floating-point
   * value in integer Get function is rounded to nearest integer
   */
   *params = lroundf(sampObj->Attrib.MinLod);
      case GL_TEXTURE_MAX_LOD:
      /* GL spec 'Data Conversions' section specifies that floating-point
   * value in integer Get function is rounded to nearest integer
   */
   *params = lroundf(sampObj->Attrib.MaxLod);
      case GL_TEXTURE_LOD_BIAS:
      /* GL spec 'Data Conversions' section specifies that floating-point
   * value in integer Get function is rounded to nearest integer
   */
   *params = lroundf(sampObj->Attrib.LodBias);
      case GL_TEXTURE_COMPARE_MODE:
      if (!ctx->Extensions.ARB_shadow)
         *params = sampObj->Attrib.CompareMode;
      case GL_TEXTURE_COMPARE_FUNC:
      if (!ctx->Extensions.ARB_shadow)
         *params = sampObj->Attrib.CompareFunc;
      case GL_TEXTURE_MAX_ANISOTROPY_EXT:
      if (!ctx->Extensions.EXT_texture_filter_anisotropic)
         /* GL spec 'Data Conversions' section specifies that floating-point
   * value in integer Get function is rounded to nearest integer
   */
   *params = lroundf(sampObj->Attrib.MaxAnisotropy);
      case GL_TEXTURE_BORDER_COLOR:
      params[0] = FLOAT_TO_INT(sampObj->Attrib.state.border_color.f[0]);
   params[1] = FLOAT_TO_INT(sampObj->Attrib.state.border_color.f[1]);
   params[2] = FLOAT_TO_INT(sampObj->Attrib.state.border_color.f[2]);
   params[3] = FLOAT_TO_INT(sampObj->Attrib.state.border_color.f[3]);
      case GL_TEXTURE_CUBE_MAP_SEAMLESS:
      if (!ctx->Extensions.AMD_seamless_cubemap_per_texture)
         *params = sampObj->Attrib.CubeMapSeamless;
      case GL_TEXTURE_SRGB_DECODE_EXT:
      if (!ctx->Extensions.EXT_texture_sRGB_decode)
         *params = (GLenum) sampObj->Attrib.sRGBDecode;
      case GL_TEXTURE_REDUCTION_MODE_EXT:
      if (!ctx->Extensions.EXT_texture_filter_minmax &&
      !_mesa_has_ARB_texture_filter_minmax(ctx))
      *params = (GLenum) sampObj->Attrib.ReductionMode;
      default:
         }
         invalid_pname:
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetSamplerParameteriv(pname=%s)",
      }
         void GLAPIENTRY
   _mesa_GetSamplerParameterfv(GLuint sampler, GLenum pname, GLfloat *params)
   {
      struct gl_sampler_object *sampObj;
            sampObj = sampler_parameter_error_check(ctx, sampler, true,
         if (!sampObj)
            switch (pname) {
   case GL_TEXTURE_WRAP_S:
      *params = (GLfloat) sampObj->Attrib.WrapS;
      case GL_TEXTURE_WRAP_T:
      *params = (GLfloat) sampObj->Attrib.WrapT;
      case GL_TEXTURE_WRAP_R:
      *params = (GLfloat) sampObj->Attrib.WrapR;
      case GL_TEXTURE_MIN_FILTER:
      *params = (GLfloat) sampObj->Attrib.MinFilter;
      case GL_TEXTURE_MAG_FILTER:
      *params = (GLfloat) sampObj->Attrib.MagFilter;
      case GL_TEXTURE_MIN_LOD:
      *params = sampObj->Attrib.MinLod;
      case GL_TEXTURE_MAX_LOD:
      *params = sampObj->Attrib.MaxLod;
      case GL_TEXTURE_LOD_BIAS:
      *params = sampObj->Attrib.LodBias;
      case GL_TEXTURE_COMPARE_MODE:
      *params = (GLfloat) sampObj->Attrib.CompareMode;
      case GL_TEXTURE_COMPARE_FUNC:
      *params = (GLfloat) sampObj->Attrib.CompareFunc;
      case GL_TEXTURE_MAX_ANISOTROPY_EXT:
      *params = sampObj->Attrib.MaxAnisotropy;
      case GL_TEXTURE_BORDER_COLOR:
      params[0] = sampObj->Attrib.state.border_color.f[0];
   params[1] = sampObj->Attrib.state.border_color.f[1];
   params[2] = sampObj->Attrib.state.border_color.f[2];
   params[3] = sampObj->Attrib.state.border_color.f[3];
      case GL_TEXTURE_CUBE_MAP_SEAMLESS:
      if (!ctx->Extensions.AMD_seamless_cubemap_per_texture)
         *params = (GLfloat) sampObj->Attrib.CubeMapSeamless;
      case GL_TEXTURE_SRGB_DECODE_EXT:
      if (!ctx->Extensions.EXT_texture_sRGB_decode)
         *params = (GLfloat) sampObj->Attrib.sRGBDecode;
      case GL_TEXTURE_REDUCTION_MODE_EXT:
      if (!ctx->Extensions.EXT_texture_filter_minmax &&
      !_mesa_has_ARB_texture_filter_minmax(ctx))
      *params = (GLfloat) sampObj->Attrib.ReductionMode;
      default:
         }
         invalid_pname:
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetSamplerParameterfv(pname=%s)",
      }
         void GLAPIENTRY
   _mesa_GetSamplerParameterIiv(GLuint sampler, GLenum pname, GLint *params)
   {
      struct gl_sampler_object *sampObj;
            sampObj = sampler_parameter_error_check(ctx, sampler, true,
         if (!sampObj)
            switch (pname) {
   case GL_TEXTURE_WRAP_S:
      *params = sampObj->Attrib.WrapS;
      case GL_TEXTURE_WRAP_T:
      *params = sampObj->Attrib.WrapT;
      case GL_TEXTURE_WRAP_R:
      *params = sampObj->Attrib.WrapR;
      case GL_TEXTURE_MIN_FILTER:
      *params = sampObj->Attrib.MinFilter;
      case GL_TEXTURE_MAG_FILTER:
      *params = sampObj->Attrib.MagFilter;
      case GL_TEXTURE_MIN_LOD:
      *params = (GLint) sampObj->Attrib.MinLod;
      case GL_TEXTURE_MAX_LOD:
      *params = (GLint) sampObj->Attrib.MaxLod;
      case GL_TEXTURE_LOD_BIAS:
      *params = (GLint) sampObj->Attrib.LodBias;
      case GL_TEXTURE_COMPARE_MODE:
      *params = sampObj->Attrib.CompareMode;
      case GL_TEXTURE_COMPARE_FUNC:
      *params = sampObj->Attrib.CompareFunc;
      case GL_TEXTURE_MAX_ANISOTROPY_EXT:
      *params = (GLint) sampObj->Attrib.MaxAnisotropy;
      case GL_TEXTURE_BORDER_COLOR:
      params[0] = sampObj->Attrib.state.border_color.i[0];
   params[1] = sampObj->Attrib.state.border_color.i[1];
   params[2] = sampObj->Attrib.state.border_color.i[2];
   params[3] = sampObj->Attrib.state.border_color.i[3];
      case GL_TEXTURE_CUBE_MAP_SEAMLESS:
      if (!ctx->Extensions.AMD_seamless_cubemap_per_texture)
         *params = sampObj->Attrib.CubeMapSeamless;
      case GL_TEXTURE_SRGB_DECODE_EXT:
      if (!ctx->Extensions.EXT_texture_sRGB_decode)
         *params = (GLenum) sampObj->Attrib.sRGBDecode;
      case GL_TEXTURE_REDUCTION_MODE_EXT:
      if (!ctx->Extensions.EXT_texture_filter_minmax &&
      !_mesa_has_ARB_texture_filter_minmax(ctx))
      *params = (GLenum) sampObj->Attrib.ReductionMode;
      default:
         }
         invalid_pname:
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetSamplerParameterIiv(pname=%s)",
      }
         void GLAPIENTRY
   _mesa_GetSamplerParameterIuiv(GLuint sampler, GLenum pname, GLuint *params)
   {
      struct gl_sampler_object *sampObj;
            sampObj = sampler_parameter_error_check(ctx, sampler, true,
         if (!sampObj)
            switch (pname) {
   case GL_TEXTURE_WRAP_S:
      *params = sampObj->Attrib.WrapS;
      case GL_TEXTURE_WRAP_T:
      *params = sampObj->Attrib.WrapT;
      case GL_TEXTURE_WRAP_R:
      *params = sampObj->Attrib.WrapR;
      case GL_TEXTURE_MIN_FILTER:
      *params = sampObj->Attrib.MinFilter;
      case GL_TEXTURE_MAG_FILTER:
      *params = sampObj->Attrib.MagFilter;
      case GL_TEXTURE_MIN_LOD:
      *params = (GLuint) sampObj->Attrib.MinLod;
      case GL_TEXTURE_MAX_LOD:
      *params = (GLuint) sampObj->Attrib.MaxLod;
      case GL_TEXTURE_LOD_BIAS:
      *params = (GLuint) sampObj->Attrib.LodBias;
      case GL_TEXTURE_COMPARE_MODE:
      *params = sampObj->Attrib.CompareMode;
      case GL_TEXTURE_COMPARE_FUNC:
      *params = sampObj->Attrib.CompareFunc;
      case GL_TEXTURE_MAX_ANISOTROPY_EXT:
      *params = (GLuint) sampObj->Attrib.MaxAnisotropy;
      case GL_TEXTURE_BORDER_COLOR:
      params[0] = sampObj->Attrib.state.border_color.ui[0];
   params[1] = sampObj->Attrib.state.border_color.ui[1];
   params[2] = sampObj->Attrib.state.border_color.ui[2];
   params[3] = sampObj->Attrib.state.border_color.ui[3];
      case GL_TEXTURE_CUBE_MAP_SEAMLESS:
      if (!ctx->Extensions.AMD_seamless_cubemap_per_texture)
         *params = sampObj->Attrib.CubeMapSeamless;
      case GL_TEXTURE_SRGB_DECODE_EXT:
      if (!ctx->Extensions.EXT_texture_sRGB_decode)
         *params = (GLenum) sampObj->Attrib.sRGBDecode;
      case GL_TEXTURE_REDUCTION_MODE_EXT:
      if (!ctx->Extensions.EXT_texture_filter_minmax &&
      !_mesa_has_ARB_texture_filter_minmax(ctx))
      *params = (GLenum) sampObj->Attrib.ReductionMode;
      default:
         }
         invalid_pname:
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetSamplerParameterIuiv(pname=%s)",
      }
