   /*
   * Copyright © Microsoft Corporation
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * on the rights to use, copy, modify, merge, publish, distribute, sub
   * license, and/or sell copies of the Software, and to permit persons to whom
   * the Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the next
   * paragraph) shall be included in all copies or substantial portions of the
   * Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
   * THE AUTHOR(S) AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
   * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
   * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
   * USE OR OTHER DEALINGS IN THE SOFTWARE.
   */
      #include "dxil_nir_lower_int_samplers.h"
   #include "nir_builder.h"
   #include "nir_builtin_builder.h"
      typedef struct {
      unsigned n_texture_states;
   dxil_wrap_sampler_state* wrap_states;
   dxil_texture_swizzle_state* tex_swizzles;
      } sampler_states;
      static bool
   lower_sample_to_txf_for_integer_tex_filter(const nir_instr *instr,
         {
      if (instr->type != nir_instr_type_tex)
            nir_tex_instr *tex = nir_instr_as_tex(instr);
   if (tex->op != nir_texop_tex &&
      tex->op != nir_texop_txb &&
   tex->op != nir_texop_txl &&
   tex->op != nir_texop_txd)
            }
      static nir_def *
   dx_get_texture_lod(nir_builder *b, nir_tex_instr *tex)
   {
               unsigned num_srcs = 0;
   for (unsigned i = 0; i < tex->num_srcs; i++) {
      if (tex->src[i].src_type == nir_tex_src_coord ||
      tex->src[i].src_type == nir_tex_src_texture_deref ||
   tex->src[i].src_type == nir_tex_src_sampler_deref ||
   tex->src[i].src_type == nir_tex_src_texture_offset ||
   tex->src[i].src_type == nir_tex_src_sampler_offset ||
   tex->src[i].src_type == nir_tex_src_texture_handle ||
   tex->src[i].src_type == nir_tex_src_sampler_handle)
            tql = nir_tex_instr_create(b->shader, num_srcs);
   tql->op = nir_texop_lod;
   unsigned coord_components = tex->coord_components;
   if (tex->is_array)
            tql->coord_components = coord_components;
   tql->sampler_dim = tex->sampler_dim;
   tql->is_shadow = tex->is_shadow;
   tql->is_new_style_shadow = tex->is_new_style_shadow;
   tql->texture_index = tex->texture_index;
   tql->sampler_index = tex->sampler_index;
            /* The coordinate needs special handling because we might have
   * to strip the array index. Don't clutter the code  with an additional
   * check for is_array though, in the worst case we create an additional
   * move the the optimization will remove later again. */
   int coord_index = nir_tex_instr_src_index(tex, nir_tex_src_coord);
   nir_def *ssa_src = nir_trim_vector(b, tex->src[coord_index].src.ssa,
         tql->src[0].src = nir_src_for_ssa(ssa_src);
            unsigned idx = 1;
   for (unsigned i = 0; i < tex->num_srcs; i++) {
      if (tex->src[i].src_type == nir_tex_src_texture_deref ||
      tex->src[i].src_type == nir_tex_src_sampler_deref ||
   tex->src[i].src_type == nir_tex_src_texture_offset ||
   tex->src[i].src_type == nir_tex_src_sampler_offset ||
   tex->src[i].src_type == nir_tex_src_texture_handle ||
   tex->src[i].src_type == nir_tex_src_sampler_handle) {
   tql->src[idx].src = nir_src_for_ssa(tex->src[i].src.ssa);
   tql->src[idx].src_type = tex->src[i].src_type;
                  nir_def_init(&tql->instr, &tql->def, 2, 32);
            /* DirectX LOD only has a value in x channel */
      }
      typedef struct {
      nir_def *coords;
      } wrap_result_t;
      typedef struct {
      nir_def *lod;
   nir_def *size;
   int ncoord_comp;
      } wrap_lower_param_t;
      static void
   wrap_clamp_to_edge(nir_builder *b, wrap_result_t *wrap_params, nir_def *size)
   {
      /* clamp(coord, 0, size - 1) */
   wrap_params->coords = nir_fmin(b, nir_fadd_imm(b, size, -1.0f),
      }
      static void
   wrap_repeat(nir_builder *b, wrap_result_t *wrap_params, nir_def *size)
   {
      /* mod(coord, size)
   * This instruction must be exact, otherwise certain sizes result in
   * incorrect sampling */
   wrap_params->coords = nir_fmod(b, wrap_params->coords, size);
      }
      static nir_def *
   mirror(nir_builder *b, nir_def *coord)
   {
      /* coord if >= 0, otherwise -(1 + coord) */
   return nir_bcsel(b, nir_fge_imm(b, coord, 0.0f), coord,
      }
      static void
   wrap_mirror_repeat(nir_builder *b, wrap_result_t *wrap_params, nir_def *size)
   {
      /* (size − 1) − mirror(mod(coord, 2 * size) − size) */
   nir_def *coord_mod2size = nir_fmod(b, wrap_params->coords, nir_fmul_imm(b, size, 2.0f));
   nir_instr_as_alu(coord_mod2size->parent_instr)->exact = true;
   nir_def *a = nir_fsub(b, coord_mod2size, size);
      }
      static void
   wrap_mirror_clamp_to_edge(nir_builder *b, wrap_result_t *wrap_params, nir_def *size)
   {
      /* clamp(mirror(coord), 0, size - 1) */
   wrap_params->coords = nir_fmin(b, nir_fadd_imm(b, size, -1.0f),
      }
      static void
   wrap_clamp(nir_builder *b, wrap_result_t *wrap_params, nir_def *size)
   {
      nir_def *is_low = nir_flt_imm(b, wrap_params->coords, 0.0);
   nir_def *is_high = nir_fge(b, wrap_params->coords, size);
      }
      static void
   wrap_mirror_clamp(nir_builder *b, wrap_result_t *wrap_params, nir_def *size)
   {
      /* We have to take care of the boundaries */
   nir_def *is_low = nir_flt(b, wrap_params->coords, nir_fmul_imm(b, size, -1.0));
   nir_def *is_high = nir_flt(b, nir_fmul_imm(b, size, 2.0), wrap_params->coords);
            /* Within the boundaries this acts like mirror_repeat */
         }
      static wrap_result_t
   wrap_coords(nir_builder *b, nir_def *coords, enum pipe_tex_wrap wrap,
         {
               switch (wrap) {
   case PIPE_TEX_WRAP_CLAMP_TO_EDGE:
      wrap_clamp_to_edge(b, &result, size);
      case PIPE_TEX_WRAP_REPEAT:
      wrap_repeat(b, &result, size);
      case PIPE_TEX_WRAP_MIRROR_REPEAT:
      wrap_mirror_repeat(b, &result, size);
      case PIPE_TEX_WRAP_MIRROR_CLAMP:
   case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_EDGE:
      wrap_mirror_clamp_to_edge(b, &result, size);
      case PIPE_TEX_WRAP_CLAMP:
   case PIPE_TEX_WRAP_CLAMP_TO_BORDER:
      wrap_clamp(b, &result, size);
      case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_BORDER:
      wrap_mirror_clamp(b, &result, size);
      }
      }
      static nir_def *
   load_bordercolor(nir_builder *b, nir_tex_instr *tex, const dxil_wrap_sampler_state *active_state,
         {
               unsigned swizzle[4] = {
      tex_swizzle->swizzle_r,
   tex_swizzle->swizzle_g,
   tex_swizzle->swizzle_b,
               /* Avoid any possible float conversion issues */
   uint32_t border_color[4];
   memcpy(border_color, active_state->border_color, sizeof(border_color));
            nir_const_value const_value[4];
   for (int i = 0; i < ndest_comp; ++i) {
      switch (swizzle[i]) {
   case PIPE_SWIZZLE_0:
      const_value[i] = nir_const_value_for_uint(0, 32);
      case PIPE_SWIZZLE_1:
      const_value[i] = nir_const_value_for_uint(1, 32);
      case PIPE_SWIZZLE_X:
   case PIPE_SWIZZLE_Y:
   case PIPE_SWIZZLE_Z:
   case PIPE_SWIZZLE_W:
      const_value[i] = nir_const_value_for_uint(border_color[swizzle[i]], 32);
      default:
                        }
      static nir_tex_instr *
   create_txf_from_tex(nir_builder *b, nir_tex_instr *tex)
   {
               unsigned num_srcs = 0;
   for (unsigned i = 0; i < tex->num_srcs; i++) {
      if (tex->src[i].src_type == nir_tex_src_texture_deref ||
      tex->src[i].src_type == nir_tex_src_texture_offset ||
   tex->src[i].src_type == nir_tex_src_texture_handle)
            txf = nir_tex_instr_create(b->shader, num_srcs);
   txf->op = nir_texop_txf;
   txf->coord_components = tex->coord_components;
   txf->sampler_dim = tex->sampler_dim;
   txf->is_array = tex->is_array;
   txf->is_shadow = tex->is_shadow;
   txf->is_new_style_shadow = tex->is_new_style_shadow;
   txf->texture_index = tex->texture_index;
   txf->sampler_index = tex->sampler_index;
            unsigned idx = 0;
   for (unsigned i = 0; i < tex->num_srcs; i++) {
      if (tex->src[i].src_type == nir_tex_src_texture_deref ||
      tex->src[i].src_type == nir_tex_src_texture_offset ||
   tex->src[i].src_type == nir_tex_src_texture_handle) {
   txf->src[idx].src = nir_src_for_ssa(tex->src[i].src.ssa);
   txf->src[idx].src_type = tex->src[i].src_type;
                  nir_def_init(&txf->instr, &txf->def, nir_tex_instr_dest_size(txf), 32);
               }
      static nir_def *
   load_texel(nir_builder *b, nir_tex_instr *tex, wrap_lower_param_t *params)
   {
               /* Put coordinates back together */
   switch (tex->coord_components) {
   case 1:
      texcoord = params->wrap[0].coords;
      case 2:
      texcoord = nir_vec2(b, params->wrap[0].coords, params->wrap[1].coords);
      case 3:
      texcoord = nir_vec3(b, params->wrap[0].coords, params->wrap[1].coords, params->wrap[2].coords);
      default:
                           nir_tex_instr *load = create_txf_from_tex(b, tex);
   nir_tex_instr_add_src(load, nir_tex_src_lod, params->lod);
   nir_tex_instr_add_src(load, nir_tex_src_coord, texcoord);
   b->cursor = nir_after_instr(&load->instr);
      }
      typedef struct {
      const dxil_wrap_sampler_state *aws;
   float max_bias;
   nir_def *size;
      } lod_params;
      static nir_def *
   evalute_active_lod(nir_builder *b, nir_tex_instr *tex, lod_params *params)
   {
               /* Later we use min_lod for clamping the LOD to a legal value */
            /* Evaluate the LOD to be used for the texel fetch */
   if (unlikely(tex->op == nir_texop_txl)) {
      int lod_index = nir_tex_instr_src_index(tex, nir_tex_src_lod);
   /* if we have an explicite LOD, take it */
      } else if (unlikely(tex->op == nir_texop_txd)) {
      int ddx_index = nir_tex_instr_src_index(tex, nir_tex_src_ddx);
   int ddy_index = nir_tex_instr_src_index(tex, nir_tex_src_ddy);
            nir_def *grad = nir_fmax(b,
                  nir_def *r = nir_fmul(b, grad, nir_i2f32(b, params->size));
   nir_def *rho = nir_channel(b, r, 0);
   for (int i = 1; i < params->ncoord_comp; ++i)
            } else if (b->shader->info.stage == MESA_SHADER_FRAGMENT){
         } else {
      /* Only fragment shaders provide the gradient information to evaluate a LOD,
   * so force 0 otherwise */
               /* Evaluate bias according to OpenGL (4.6 (Compatibility  Profile) October 22, 2019),
   * sec. 8.14.1, eq. (8.9)
   *
   *    lod' = lambda + CLAMP(bias_texobj + bias_texunit + bias_shader)
   *
   * bias_texobj is the value of TEXTURE_LOD_BIAS for the bound texture object. ...
   * bias_textunt is the value of TEXTURE_LOD_BIAS for the current texture unit, ...
   * bias shader is the value of the optional bias parameter in the texture
   * lookup functions available to fragment shaders. ... The sum of these values
   * is clamped to the range [−bias_max, bias_max] where bias_max is the value
   * of the implementation defined constant MAX_TEXTURE_LOD_BIAS.
   * In core contexts the value bias_texunit is dropped from above equation.
   *
   * Gallium provides the value lod_bias as the sum of bias_texobj and bias_texunit
   * in compatibility contexts and as bias_texobj in core contexts, hence the
   * implementation here is the same in both cases.
   */
            if (unlikely(tex->op == nir_texop_txb)) {
      int bias_index = nir_tex_instr_src_index(tex, nir_tex_src_bias);
               lod = nir_fadd(b, lod, nir_fclamp(b, lod_bias,
                  /* Clamp lod according to ibid. eq. (8.10) */
            /* If the max lod is > max_bias = log2(max_texture_size), the lod will be clamped
   * by the number of levels, no need to clamp it againt the max_lod first. */
   if (params->aws->max_lod <= params->max_bias)
            /* Pick nearest LOD */
            /* cap actual lod by number of available levels */
      }
         static nir_def *
   lower_sample_to_txf_for_integer_tex_impl(nir_builder *b, nir_instr *instr,
         {
      sampler_states *states = (sampler_states *)options;
                     const static dxil_wrap_sampler_state default_wrap_state = {
      { 0, 0, 0, 1 },
   0,
   FLT_MIN, FLT_MAX,
   0,
   { 0, 0, 0 },
   1,
   0,
   0,
      };
                     int coord_index = nir_tex_instr_src_index(tex, nir_tex_src_coord);
   nir_def *old_coord = tex->src[coord_index].src.ssa;
   params.ncoord_comp = tex->coord_components;
   if (tex->is_array)
            /* This helper to get the texture size always uses LOD 0, and DirectX doesn't support
   * giving another LOD when querying the texture size */
                     if (active_wrap_state->last_level > 0) {
      lod_params p = {
      .aws = active_wrap_state,
   .max_bias = states->max_bias,
   .size = size0,
      };
            /* Evaluate actual level size*/
   params.size = nir_i2f32(b, nir_imax(b, nir_ishr(b, size0, params.lod),
      } else {
                  nir_def *new_coord = old_coord;
   if (!active_wrap_state->is_nonnormalized_coords) {
      /* Evaluate the integer lookup coordinates for the requested LOD, don't touch the
   * array index */
   if (!tex->is_array) {
         } else {
      nir_def *array_index = nir_channel(b, old_coord, params.ncoord_comp);
   int mask = (1 << params.ncoord_comp) - 1;
   nir_def *coord = nir_fmul(b, nir_channels(b, params.size, mask),
         switch (params.ncoord_comp) {
   case 1:
      new_coord = nir_vec2(b, coord, array_index);
      case 2:
      new_coord = nir_vec3(b, nir_channel(b, coord, 0),
                  default:
                        nir_def *coord_help[3];
   for (int i = 0; i < params.ncoord_comp; ++i)
            // Note: array index needs to be rounded to nearest before clamp rather than floored
   if (tex->is_array)
            /* Correct the texture coordinates for the offsets. */
   int offset_index = nir_tex_instr_src_index(tex, nir_tex_src_offset);
   if (offset_index >= 0) {
      nir_def *offset = tex->src[offset_index].src.ssa;
   for (int i = 0; i < params.ncoord_comp; ++i)
                                    for (int i = 0; i < params.ncoord_comp; ++i) {
      params.wrap[i] = wrap_coords(b, coord_help[i], active_wrap_state->wrap[i], nir_channel(b, params.size, i));
               if (tex->is_array)
      params.wrap[params.ncoord_comp] =
         wrap_coords(b, coord_help[params.ncoord_comp],
   } else {
      /* When we emulate a cube map by using a texture array, the coordinates are always
   * in range, and we don't have to take care of boundary conditions */
   for (unsigned i = 0; i < 3; ++i) {
      params.wrap[i].coords = coord_help[i];
                  const dxil_texture_swizzle_state one2one = {
   PIPE_SWIZZLE_X,  PIPE_SWIZZLE_Y,  PIPE_SWIZZLE_Z, PIPE_SWIZZLE_W
            nir_if *border_if = nir_push_if(b, use_border_color);
   const dxil_texture_swizzle_state *swizzle = (states->tex_swizzles && tex->sampler_index < states->n_texture_states) ?
                  nir_def *border_color = load_bordercolor(b, tex, active_wrap_state, swizzle);
   nir_if *border_else = nir_push_else(b, border_if);
   nir_def *sampler_color = load_texel(b, tex, &params);
               }
      /* Sampling from integer textures is not allowed in DirectX, so we have
   * to use texel fetches. For this we have to scale the coordiantes
   * to be integer based, and evaluate the LOD the texel fetch has to be
   * applied on, and take care of the boundary conditions .
   */
   bool
   dxil_lower_sample_to_txf_for_integer_tex(nir_shader *s,
                           {
               bool result =
         nir_shader_lower_instructions(s,
                  }
