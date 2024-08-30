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
      #include "anv_nir.h"
   #include "anv_private.h"
   #include "nir/nir.h"
   #include "nir/nir_builder.h"
   #include "vk_nir_convert_ycbcr.h"
      struct ycbcr_state {
      nir_builder *builder;
   nir_def *image_size;
   nir_tex_instr *origin_tex;
   nir_deref_instr *tex_deref;
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
   const struct vk_ycbcr_conversion *conversion = state->conversion;
   nir_def *image_size = get_texture_size(state, state->tex_deref);
   nir_def *comp[4] = { NULL, };
            for (c = 0; c < ARRAY_SIZE(conversion->state.chroma_offsets); c++) {
      if (plane_format->denominator_scales[c] > 1 &&
      conversion->state.chroma_offsets[c] == VK_CHROMA_LOCATION_COSITED_EVEN) {
   comp[c] = implicit_downsampled_coord(b,
                  } else {
                     /* Leave other coordinates untouched */
   for (; c < old_coords->num_components; c++)
               }
      static nir_def *
   create_plane_tex_instr_implicit(struct ycbcr_state *state,
         {
      nir_builder *b = state->builder;
   const struct vk_ycbcr_conversion *conversion = state->conversion;
   const struct anv_format_plane *plane_format =
         nir_tex_instr *old_tex = state->origin_tex;
            for (uint32_t i = 0; i < old_tex->num_srcs; i++) {
               switch (old_tex->src[i].src_type) {
   case nir_tex_src_coord:
      if (plane_format->has_chroma && conversion->state.chroma_reconstruction) {
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
   channel_to_component(enum isl_channel_select channel)
   {
      switch (channel) {
   case ISL_CHANNEL_SELECT_RED:
         case ISL_CHANNEL_SELECT_GREEN:
         case ISL_CHANNEL_SELECT_BLUE:
         case ISL_CHANNEL_SELECT_ALPHA:
         default:
      unreachable("invalid channel");
         }
      static enum isl_channel_select
   swizzle_channel(struct isl_swizzle swizzle, unsigned channel)
   {
      switch (channel) {
   case 0:
         case 1:
         case 2:
         case 3:
         default:
      unreachable("invalid channel");
         }
      static bool
   anv_nir_lower_ycbcr_textures_instr(nir_builder *builder,
               {
               if (instr->type != nir_instr_type_tex)
                     int deref_src_idx = nir_tex_instr_src_index(tex, nir_tex_src_texture_deref);
   assert(deref_src_idx >= 0);
            nir_variable *var = nir_deref_instr_get_variable(deref);
   const struct anv_descriptor_set_layout *set_layout =
         const struct anv_descriptor_set_binding_layout *binding =
            /* For the following instructions, we don't apply any change and let the
   * instruction apply to the first plane.
   */
   if (tex->op == nir_texop_txs ||
      tex->op == nir_texop_query_levels ||
   tex->op == nir_texop_lod)
         if (binding->immutable_samplers == NULL)
            assert(tex->texture_index == 0);
   unsigned array_index = 0;
   if (deref->deref_type != nir_deref_type_var) {
      assert(deref->deref_type == nir_deref_type_array);
   if (!nir_src_is_const(deref->arr.index))
         array_index = nir_src_as_uint(deref->arr.index);
      }
            if (sampler->conversion == NULL)
            struct ycbcr_state state = {
      .builder = builder,
   .origin_tex = tex,
   .tex_deref = deref,
                        const struct anv_format *format = anv_get_format(state.conversion->state.format);
   const struct isl_format_layout *y_isl_layout = NULL;
   for (uint32_t p = 0; p < format->n_planes; p++) {
      if (!format->planes[p].has_chroma)
      }
   assert(y_isl_layout != NULL);
            /* |ycbcr_comp| holds components in the order : Cr-Y-Cb */
   nir_def *zero = nir_imm_float(builder, 0.0f);
   nir_def *one = nir_imm_float(builder, 1.0f);
   /* Use extra 2 channels for following swizzle */
            uint8_t ycbcr_bpcs[5];
            /* Go through all the planes and gather the samples into a |ycbcr_comp|
   * while applying a swizzle required by the spec:
   *
   *    R, G, B should respectively map to Cr, Y, Cb
   */
   for (uint32_t p = 0; p < format->n_planes; p++) {
      const struct anv_format_plane *plane_format = &format->planes[p];
            for (uint32_t pc = 0; pc < 4; pc++) {
      enum isl_channel_select ycbcr_swizzle =
                                       /* Also compute the number of bits for each component. */
   const struct isl_format_layout *isl_layout =
                        /* Now remaps components to the order specified by the conversion. */
   nir_def *swizzled_comp[4] = { NULL, };
            for (uint32_t i = 0; i < ARRAY_SIZE(state.conversion->state.mapping); i++) {
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
                  nir_def *result = nir_vec(builder, swizzled_comp, 4);
   if (state.conversion->state.ycbcr_model != VK_SAMPLER_YCBCR_MODEL_CONVERSION_RGB_IDENTITY) {
      result = nir_convert_ycbcr_to_rgb(builder,
                                 nir_def_rewrite_uses(&tex->def, result);
               }
      bool
   anv_nir_lower_ycbcr_textures(nir_shader *shader,
         {
      return nir_shader_instructions_pass(shader,
                        }
