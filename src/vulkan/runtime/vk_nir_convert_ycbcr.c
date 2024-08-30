   /*
   * Copyright © 2017 Intel Corporation
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sublicense,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the next
   * paragraph) shall be included in all copies or substantial portions of the
   * Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   * IN THE SOFTWARE.
   */
      #include "vk_nir_convert_ycbcr.h"
      #include "vk_format.h"
   #include "vk_ycbcr_conversion.h"
      #include <math.h>
      static nir_def *
   y_range(nir_builder *b,
         nir_def *y_channel,
   int bpc,
   {
      switch (range) {
   case VK_SAMPLER_YCBCR_RANGE_ITU_FULL:
         case VK_SAMPLER_YCBCR_RANGE_ITU_NARROW:
      return nir_fmul_imm(b,
                     nir_fadd_imm(b,
         default:
      unreachable("missing Ycbcr range");
         }
      static nir_def *
   chroma_range(nir_builder *b,
               nir_def *chroma_channel,
   {
      switch (range) {
   case VK_SAMPLER_YCBCR_RANGE_ITU_FULL:
      return nir_fadd(b, chroma_channel,
      case VK_SAMPLER_YCBCR_RANGE_ITU_NARROW:
      return nir_fmul_imm(b,
                     nir_fadd_imm(b,
      default:
      unreachable("missing Ycbcr range");
         }
      typedef struct nir_const_value_3_4 {
         } nir_const_value_3_4;
      static const nir_const_value_3_4 *
   ycbcr_model_to_rgb_matrix(VkSamplerYcbcrModelConversion model)
   {
      switch (model) {
   case VK_SAMPLER_YCBCR_MODEL_CONVERSION_YCBCR_601: {
      static const nir_const_value_3_4 bt601 = { {
      { { .f32 =  1.402f             }, { .f32 = 1.0f }, { .f32 =  0.0f               }, { .f32 = 0.0f } },
   { { .f32 = -0.714136286201022f }, { .f32 = 1.0f }, { .f32 = -0.344136286201022f }, { .f32 = 0.0f } },
                  }
   case VK_SAMPLER_YCBCR_MODEL_CONVERSION_YCBCR_709: {
      static const nir_const_value_3_4 bt709 = { {
      { { .f32 =  1.5748031496063f   }, { .f32 = 1.0f }, { .f32 =  0.0f               }, { .f32 = 0.0f } },
   { { .f32 = -0.468125209181067f }, { .f32 = 1.0f }, { .f32 = -0.187327487470334f }, { .f32 = 0.0f } },
                  }
   case VK_SAMPLER_YCBCR_MODEL_CONVERSION_YCBCR_2020: {
      static const nir_const_value_3_4 bt2020 = { {
      { { .f32 =  1.4746f            }, { .f32 = 1.0f }, { .f32 =  0.0f               }, { .f32 = 0.0f } },
   { { .f32 = -0.571353126843658f }, { .f32 = 1.0f }, { .f32 = -0.164553126843658f }, { .f32 = 0.0f } },
                  }
   default:
      unreachable("missing Ycbcr model");
         }
      nir_def *
   nir_convert_ycbcr_to_rgb(nir_builder *b,
                           {
      nir_def *expanded_channels =
      nir_vec4(b,
            chroma_range(b, nir_channel(b, raw_channels, 0), bpcs[0], range),
            if (model == VK_SAMPLER_YCBCR_MODEL_CONVERSION_YCBCR_IDENTITY)
            const nir_const_value_3_4 *conversion_matrix =
            nir_def *converted_channels[] = {
      nir_fdot(b, expanded_channels, nir_build_imm(b, 4, 32, conversion_matrix->v[0])),
   nir_fdot(b, expanded_channels, nir_build_imm(b, 4, 32, conversion_matrix->v[1])),
               return nir_vec4(b,
            }
      struct ycbcr_state {
      nir_builder *builder;
   nir_def *image_size;
   nir_tex_instr *origin_tex;
   nir_deref_instr *tex_deref;
   const struct vk_ycbcr_conversion_state *conversion;
      };
      /* TODO: we should probably replace this with a push constant/uniform. */
   static nir_def *
   get_texture_size(struct ycbcr_state *state, nir_deref_instr *texture)
   {
      if (state->image_size)
            nir_builder *b = state->builder;
   const struct glsl_type *type = texture->type;
            tex->op = nir_texop_txs;
   tex->sampler_dim = glsl_get_sampler_dim(type);
   tex->is_array = glsl_sampler_type_is_array(type);
   tex->is_shadow = glsl_sampler_type_is_shadow(type);
            tex->src[0] = nir_tex_src_for_ssa(nir_tex_src_texture_deref,
            nir_def_init(&tex->instr, &tex->def, nir_tex_instr_dest_size(tex), 32);
                        }
      static nir_def *
   implicit_downsampled_coord(nir_builder *b,
                     {
      return nir_fadd(b,
                  value,
   nir_frcp(b,
   }
      static nir_def *
   implicit_downsampled_coords(struct ycbcr_state *state,
               {
      nir_builder *b = state->builder;
   const struct vk_ycbcr_conversion_state *conversion = state->conversion;
   nir_def *image_size = get_texture_size(state, state->tex_deref);
   nir_def *comp[4] = { NULL, };
            for (c = 0; c < ARRAY_SIZE(conversion->chroma_offsets); c++) {
      if (format_plane->denominator_scales[c] > 1 &&
      conversion->chroma_offsets[c] == VK_CHROMA_LOCATION_COSITED_EVEN) {
   comp[c] = implicit_downsampled_coord(b,
                  } else {
                     /* Leave other coordinates untouched */
   for (; c < old_coords->num_components; c++)
               }
      static nir_def *
   create_plane_tex_instr_implicit(struct ycbcr_state *state,
         {
      nir_builder *b = state->builder;
   const struct vk_ycbcr_conversion_state *conversion = state->conversion;
   const struct vk_format_ycbcr_plane *format_plane =
         nir_tex_instr *old_tex = state->origin_tex;
            for (uint32_t i = 0; i < old_tex->num_srcs; i++) {
               switch (old_tex->src[i].src_type) {
   case nir_tex_src_coord:
      if (format_plane->has_chroma && conversion->chroma_reconstruction) {
      tex->src[i].src =
      nir_src_for_ssa(implicit_downsampled_coords(state,
               }
      default:
      tex->src[i].src = nir_src_for_ssa(old_tex->src[i].src.ssa);
         }
   tex->src[tex->num_srcs - 1] = nir_tex_src_for_ssa(nir_tex_src_plane,
         tex->sampler_dim = old_tex->sampler_dim;
            tex->op = old_tex->op;
   tex->coord_components = old_tex->coord_components;
   tex->is_new_style_shadow = old_tex->is_new_style_shadow;
            tex->texture_index = old_tex->texture_index;
   tex->sampler_index = old_tex->sampler_index;
            nir_def_init(&tex->instr, &tex->def, old_tex->def.num_components,
                     }
      static unsigned
   swizzle_to_component(VkComponentSwizzle swizzle)
   {
      switch (swizzle) {
   case VK_COMPONENT_SWIZZLE_R:
         case VK_COMPONENT_SWIZZLE_G:
         case VK_COMPONENT_SWIZZLE_B:
         case VK_COMPONENT_SWIZZLE_A:
         default:
      unreachable("invalid channel");
         }
      struct lower_ycbcr_tex_state {
      nir_vk_ycbcr_conversion_lookup_cb cb;
      };
      static bool
   lower_ycbcr_tex_instr(nir_builder *b, nir_instr *instr, void *_state)
   {
               if (instr->type != nir_instr_type_tex)
                     /* For the following instructions, we don't apply any change and let the
   * instruction apply to the first plane.
   */
   if (tex->op == nir_texop_txs ||
      tex->op == nir_texop_query_levels ||
   tex->op == nir_texop_lod)
         int deref_src_idx = nir_tex_instr_src_index(tex, nir_tex_src_texture_deref);
   assert(deref_src_idx >= 0);
            nir_variable *var = nir_deref_instr_get_variable(deref);
   uint32_t set = var->data.descriptor_set;
            assert(tex->texture_index == 0);
   unsigned array_index = 0;
   if (deref->deref_type != nir_deref_type_var) {
      assert(deref->deref_type == nir_deref_type_array);
   if (!nir_src_is_const(deref->arr.index))
                     const struct vk_ycbcr_conversion_state *conversion =
         if (conversion == NULL)
            const struct vk_format_ycbcr_info *format_ycbcr_info =
            /* This can happen if the driver hasn't done a good job of filtering on
   * sampler creation and lets through a VkYcbcrConversion object which isn't
   * actually YCbCr.  We're supposed to ignore those.
   */
   if (format_ycbcr_info == NULL)
                     VkFormat y_format = VK_FORMAT_UNDEFINED;
   for (uint32_t p = 0; p < format_ycbcr_info->n_planes; p++) {
      if (!format_ycbcr_info->planes[p].has_chroma)
      }
   assert(y_format != VK_FORMAT_UNDEFINED);
   const struct util_format_description *y_format_desc =
                  /* |ycbcr_comp| holds components in the order : Cr-Y-Cb */
   nir_def *zero = nir_imm_float(b, 0.0f);
   nir_def *one = nir_imm_float(b, 1.0f);
   /* Use extra 2 channels for following swizzle */
            uint8_t ycbcr_bpcs[5];
            /* Go through all the planes and gather the samples into a |ycbcr_comp|
   * while applying a swizzle required by the spec:
   *
   *    R, G, B should respectively map to Cr, Y, Cb
   */
   for (uint32_t p = 0; p < format_ycbcr_info->n_planes; p++) {
      const struct vk_format_ycbcr_plane *format_plane =
            struct ycbcr_state tex_state = {
      .builder = b,
   .origin_tex = tex,
   .tex_deref = deref,
   .conversion = conversion,
      };
            for (uint32_t pc = 0; pc < 4; pc++) {
      VkComponentSwizzle ycbcr_swizzle = format_plane->ycbcr_swizzle[pc];
                                 /* Also compute the number of bits for each component. */
   const struct util_format_description *plane_format_desc =
                        /* Now remaps components to the order specified by the conversion. */
   nir_def *swizzled_comp[4] = { NULL, };
            for (uint32_t i = 0; i < ARRAY_SIZE(conversion->mapping); i++) {
      /* Maps to components in |ycbcr_comp| */
   static const uint32_t swizzle_mapping[] = {
      [VK_COMPONENT_SWIZZLE_ZERO] = 4,
   [VK_COMPONENT_SWIZZLE_ONE]  = 3,
   [VK_COMPONENT_SWIZZLE_R]    = 0,
   [VK_COMPONENT_SWIZZLE_G]    = 1,
   [VK_COMPONENT_SWIZZLE_B]    = 2,
      };
            if (m == VK_COMPONENT_SWIZZLE_IDENTITY) {
      swizzled_comp[i] = ycbcr_comp[i];
      } else {
      swizzled_comp[i] = ycbcr_comp[swizzle_mapping[m]];
                  nir_def *result = nir_vec(b, swizzled_comp, 4);
   if (conversion->ycbcr_model != VK_SAMPLER_YCBCR_MODEL_CONVERSION_RGB_IDENTITY) {
      result = nir_convert_ycbcr_to_rgb(b, conversion->ycbcr_model,
                           nir_def_rewrite_uses(&tex->def, result);
               }
      bool nir_vk_lower_ycbcr_tex(nir_shader *nir,
               {
      struct lower_ycbcr_tex_state state = {
      .cb = cb,
               return nir_shader_instructions_pass(nir, lower_ycbcr_tex_instr,
                  }
