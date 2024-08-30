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
      /*
   * Authors:
   *   Keith Whitwell <keithw@vmware.com>
   *   Brian Paul
   */
         #include "main/macros.h"
   #include "main/glformats.h"
   #include "main/samplerobj.h"
   #include "main/teximage.h"
   #include "main/texobj.h"
      #include "st_context.h"
   #include "st_cb_texture.h"
   #include "st_format.h"
   #include "st_atom.h"
   #include "st_sampler_view.h"
   #include "st_texture.h"
   #include "pipe/p_context.h"
   #include "pipe/p_defines.h"
      #include "cso_cache/cso_context.h"
      #include "util/format/u_format.h"
   #include "program/prog_instruction.h"
         /**
   * Convert a gl_sampler_object to a pipe_sampler_state object.
   */
   void
   st_convert_sampler(const struct st_context *st,
                     const struct gl_texture_object *texobj,
   const struct gl_sampler_object *msamp,
   float tex_unit_lod_bias,
   struct pipe_sampler_state *sampler,
   {
                        if (texobj->_IsIntegerFormat ||
      (texobj->_IsFloat && st->ctx->Const.ForceFloat32TexNearest)) {
   sampler->min_img_filter = PIPE_TEX_FILTER_NEAREST;
   sampler->min_mip_filter = PIPE_TEX_FILTER_NEAREST;
               if (texobj->Target == GL_TEXTURE_RECTANGLE_ARB && !st->lower_rect_tex)
            /*
   * The spec says that "texture wrap modes are ignored" for seamless cube
   * maps, so normalize the CSO. This works around Apple hardware which honours
   * REPEAT modes even for seamless cube maps.
   */
   if ((texobj->Target == GL_TEXTURE_CUBE_MAP ||
      texobj->Target == GL_TEXTURE_CUBE_MAP_ARRAY) &&
            sampler->wrap_s = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   sampler->wrap_t = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
                        /* Check that only wrap modes using the border color have the first bit
   * set.
   */
   STATIC_ASSERT(PIPE_TEX_WRAP_CLAMP & 0x1);
   STATIC_ASSERT(PIPE_TEX_WRAP_CLAMP_TO_BORDER & 0x1);
   STATIC_ASSERT(PIPE_TEX_WRAP_MIRROR_CLAMP & 0x1);
   STATIC_ASSERT(PIPE_TEX_WRAP_MIRROR_CLAMP_TO_BORDER & 0x1);
   STATIC_ASSERT(((PIPE_TEX_WRAP_REPEAT |
                        if (msamp->Attrib.IsBorderColorNonZero &&
      /* This is true if wrap modes are using the border color: */
   (sampler->wrap_s | sampler->wrap_t | sampler->wrap_r) & 0x1) {
            /* From OpenGL 4.3 spec, "Combined Depth/Stencil Textures":
   *
   *    "The DEPTH_STENCIL_TEXTURE_MODE is ignored for non
   *     depth/stencil textures.
   */
            const GLboolean is_integer =
      texobj->_IsIntegerFormat ||
               if (texobj->StencilSampling && has_combined_ds)
            if (st->apply_texture_swizzle_to_border_color ||
      st->alpha_border_color_is_not_w || st->use_format_with_border_color) {
                     union pipe_color_union tmp = sampler->border_color;
   const unsigned char swz[4] =
   {
      GET_SWZ(swizzle, 0),
   GET_SWZ(swizzle, 1),
                              util_format_apply_color_swizzle(&sampler->border_color,
                        if (!ignore_srgb_decode && msamp->Attrib.sRGBDecode == GL_SKIP_DECODE_EXT)
         enum pipe_format format = st_get_sampler_view_format(st, texobj, srgb_skip_decode);
   if (st->use_format_with_border_color)
         /* alpha is not w, so set it to the first available component: */
   if (st->alpha_border_color_is_not_w && util_format_is_alpha(format)) {
      /* use x component */
      } else if (st->alpha_border_color_is_not_w && util_format_is_luminance_alpha(format)) {
      /* use y component */
      } else {
      /* not an alpha format */
   st_translate_color(&sampler->border_color,
            } else {
      st_translate_color(&sampler->border_color,
      }
               /* If sampling a depth texture and using shadow comparison */
   if (msamp->Attrib.CompareMode == GL_COMPARE_R_TO_TEXTURE) {
               if (texBaseFormat == GL_DEPTH_COMPONENT ||
      (texBaseFormat == GL_DEPTH_STENCIL && !texobj->StencilSampling))
      }
      /**
   * Get a pipe_sampler_state object from a texture unit.
   */
   void
   st_convert_sampler_from_unit(const struct st_context *st,
                     {
      const struct gl_texture_object *texobj;
   struct gl_context *ctx = st->ctx;
            texobj = ctx->Texture.Unit[texUnit]._Current;
                     st_convert_sampler(st, texobj, msamp, ctx->Texture.Unit[texUnit].LodBiasQuantized,
      }
         /**
   * Update the gallium driver's sampler state for fragment, vertex or
   * geometry shader stage.
   */
   static void
   update_shader_samplers(struct st_context *st,
                           {
      struct gl_context *ctx = st->ctx;
   GLbitfield samplers_used = prog->SamplersUsed;
   GLbitfield free_slots = ~prog->SamplersUsed;
   GLbitfield external_samplers_used = prog->ExternalSamplersUsed;
   unsigned unit, num_samplers;
   struct pipe_sampler_state local_samplers[PIPE_MAX_SAMPLERS];
            if (samplers_used == 0x0) {
      if (out_num_samplers)
                     if (!samplers)
                     /* loop over sampler units (aka tex image units) */
   for (unit = 0; samplers_used; unit++, samplers_used >>= 1) {
      struct pipe_sampler_state *sampler = samplers + unit;
            /* Don't update the sampler for TBOs. cso_context will not bind sampler
   * states that are NULL.
   */
   if (samplers_used & 1 &&
      (ctx->Texture.Unit[tex_unit]._Current->Target != GL_TEXTURE_BUFFER)) {
   st_convert_sampler_from_unit(
      st, sampler, tex_unit,
         } else {
                     /* For any external samplers with multiplaner YUV, stuff the additional
   * sampler states we need at the end.
   *
   * Just re-use the existing sampler-state from the primary slot.
   */
   while (unlikely(external_samplers_used)) {
      GLuint unit = u_bit_scan(&external_samplers_used);
   GLuint extra = 0;
   struct gl_texture_object *stObj =
                  /* if resource format matches then YUV wasn't lowered */
   if (!stObj || st_get_view_format(stObj) == stObj->pt->format)
            switch (st_get_view_format(stObj)) {
   case PIPE_FORMAT_NV12:
      if (stObj->pt->format == PIPE_FORMAT_R8_G8B8_420_UNORM)
      /* no additional views needed */
         case PIPE_FORMAT_NV21:
      if (stObj->pt->format == PIPE_FORMAT_R8_B8G8_420_UNORM)
      /* no additional views needed */
         case PIPE_FORMAT_P010:
   case PIPE_FORMAT_P012:
   case PIPE_FORMAT_P016:
   case PIPE_FORMAT_P030:
   case PIPE_FORMAT_Y210:
   case PIPE_FORMAT_Y212:
   case PIPE_FORMAT_Y216:
   case PIPE_FORMAT_YUYV:
   case PIPE_FORMAT_YVYU:
   case PIPE_FORMAT_UYVY:
   case PIPE_FORMAT_VYUY:
      if (stObj->pt->format == PIPE_FORMAT_R8G8_R8B8_UNORM ||
      stObj->pt->format == PIPE_FORMAT_R8B8_R8G8_UNORM ||
   stObj->pt->format == PIPE_FORMAT_B8R8_G8R8_UNORM ||
   stObj->pt->format == PIPE_FORMAT_G8R8_B8R8_UNORM) {
   /* no additional views needed */
               /* we need one additional sampler: */
   extra = u_bit_scan(&free_slots);
   states[extra] = sampler;
      case PIPE_FORMAT_IYUV:
      if (stObj->pt->format == PIPE_FORMAT_R8_G8_B8_420_UNORM ||
      stObj->pt->format == PIPE_FORMAT_R8_B8_G8_420_UNORM) {
   /* no additional views needed */
      }
   /* we need two additional samplers: */
   extra = u_bit_scan(&free_slots);
   states[extra] = sampler;
   extra = u_bit_scan(&free_slots);
   states[extra] = sampler;
      default:
                                       if (out_num_samplers)
      }
         void
   st_update_vertex_samplers(struct st_context *st)
   {
               update_shader_samplers(st,
                        }
         void
   st_update_tessctrl_samplers(struct st_context *st)
   {
               if (ctx->TessCtrlProgram._Current) {
      update_shader_samplers(st,
               }
         void
   st_update_tesseval_samplers(struct st_context *st)
   {
               if (ctx->TessEvalProgram._Current) {
      update_shader_samplers(st,
               }
         void
   st_update_geometry_samplers(struct st_context *st)
   {
               if (ctx->GeometryProgram._Current) {
      update_shader_samplers(st,
               }
         void
   st_update_fragment_samplers(struct st_context *st)
   {
               update_shader_samplers(st,
                        }
         void
   st_update_compute_samplers(struct st_context *st)
   {
               if (ctx->ComputeProgram._Current) {
      update_shader_samplers(st,
               }
