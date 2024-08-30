   /*
   * Copyright © 2023 Valve Corporation
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
   *
   */
      #include "ac_nir.h"
   #include "nir_builder.h"
      /**
   * Build a manual selection sequence for cube face sc/tc coordinates and
   * major axis vector (multiplied by 2 for consistency) for the given
   * vec3 \p coords, for the face implied by \p selcoords.
   *
   * For the major axis, we always adjust the sign to be in the direction of
   * selcoords.ma; i.e., a positive out_ma means that coords is pointed towards
   * the selcoords major axis.
   */
   static void
   build_cube_select(nir_builder *b, nir_def *ma, nir_def *id, nir_def *deriv,
         {
      nir_def *deriv_x = nir_channel(b, deriv, 0);
   nir_def *deriv_y = nir_channel(b, deriv, 1);
            nir_def *is_ma_positive = nir_fge_imm(b, ma, 0.0);
   nir_def *sgn_ma =
                  nir_def *is_ma_z = nir_fge_imm(b, id, 4.0);
   nir_def *is_ma_y = nir_fge_imm(b, id, 2.0);
   is_ma_y = nir_iand(b, is_ma_y, nir_inot(b, is_ma_z));
            /* Select sc */
   nir_def *tmp = nir_bcsel(b, is_not_ma_x, deriv_x, deriv_z);
   nir_def *sgn =
                  /* Select tc */
   tmp = nir_bcsel(b, is_ma_y, deriv_z, deriv_y);
   sgn = nir_bcsel(b, is_ma_y, sgn_ma, nir_imm_float(b, -1.0));
            /* Select ma */
   tmp = nir_bcsel(b, is_ma_z, deriv_z, nir_bcsel(b, is_ma_y, deriv_y, deriv_x));
      }
      static void
   prepare_cube_coords(nir_builder *b, nir_tex_instr *tex, nir_def **coord, nir_src *ddx,
         {
      nir_def *coords[NIR_MAX_VEC_COMPONENTS] = {0};
   for (unsigned i = 0; i < (*coord)->num_components; i++)
            /* Section 8.9 (Texture Functions) of the GLSL 4.50 spec says:
   *
   *    "For Array forms, the array layer used will be
   *
   *       max(0, min(d−1, floor(layer+0.5)))
   *
   *     where d is the depth of the texture array and layer
   *     comes from the component indicated in the tables below.
   *     Workaroudn for an issue where the layer is taken from a
   *     helper invocation which happens to fall on a different
   *     layer due to extrapolation."
   *
   * GFX8 and earlier attempt to implement this in hardware by
   * clamping the value of coords[2] = (8 * layer) + face.
   * Unfortunately, this means that the we end up with the wrong
   * face when clamping occurs.
   *
   * Clamp the layer earlier to work around the issue.
   */
   if (tex->is_array && options->gfx_level <= GFX8 && coords[3])
            nir_def *cube_coords = nir_cube_amd(b, nir_vec(b, coords, 3));
   nir_def *sc = nir_channel(b, cube_coords, 1);
   nir_def *tc = nir_channel(b, cube_coords, 0);
   nir_def *ma = nir_channel(b, cube_coords, 2);
   nir_def *invma = nir_frcp(b, nir_fabs(b, ma));
            if (ddx || ddy) {
      sc = nir_fmul(b, sc, invma);
            /* Convert cube derivatives to 2D derivatives. */
   for (unsigned i = 0; i < 2; i++) {
      /* Transform the derivative alongside the texture
   * coordinate. Mathematically, the correct formula is
   * as follows. Assume we're projecting onto the +Z face
   * and denote by dx/dh the derivative of the (original)
   * X texture coordinate with respect to horizontal
   * window coordinates. The projection onto the +Z face
   * plane is:
   *
   *   f(x,z) = x/z
   *
   * Then df/dh = df/dx * dx/dh + df/dz * dz/dh
   *            = 1/z * dx/dh - x/z * 1/z * dz/dh.
   *
   * This motivatives the implementation below.
   *
   * Whether this actually gives the expected results for
   * apps that might feed in derivatives obtained via
   * finite differences is anyone's guess. The OpenGL spec
   * seems awfully quiet about how textureGrad for cube
   * maps should be handled.
   */
                                                      sc = nir_fadd_imm(b, sc, 1.5);
      } else {
      sc = nir_ffma_imm2(b, sc, invma, 1.5);
               if (tex->is_array && coords[3])
                        }
      static bool
   lower_array_layer_round_even(nir_builder *b, nir_tex_instr *tex, nir_def **coords)
   {
      int coord_index = nir_tex_instr_src_index(tex, nir_tex_src_coord);
   if (coord_index < 0 || nir_tex_instr_src_type(tex, coord_index) != nir_type_float)
            unsigned layer = tex->coord_components - 1;
   nir_def *rounded_layer = nir_fround_even(b, nir_channel(b, *coords, layer));
   *coords = nir_vector_insert_imm(b, *coords, rounded_layer, layer);
      }
      static bool
   lower_tex_coords(nir_builder *b, nir_tex_instr *tex, nir_def **coords,
         {
      bool progress = false;
   if ((options->lower_array_layer_round_even || tex->sampler_dim == GLSL_SAMPLER_DIM_CUBE) &&
      tex->is_array && tex->op != nir_texop_lod)
         if (tex->sampler_dim != GLSL_SAMPLER_DIM_CUBE &&
      !(tex->sampler_dim == GLSL_SAMPLER_DIM_1D && options->gfx_level == GFX9))
         int ddx_idx = nir_tex_instr_src_index(tex, nir_tex_src_ddx);
   int ddy_idx = nir_tex_instr_src_index(tex, nir_tex_src_ddy);
   nir_src *ddx = ddx_idx >= 0 ? &tex->src[ddx_idx].src : NULL;
            if (tex->sampler_dim == GLSL_SAMPLER_DIM_1D) {
      nir_def *y =
         if (tex->is_array && (*coords)->num_components > 1) {
      nir_def *x = nir_channel(b, *coords, 0);
   nir_def *idx = nir_channel(b, *coords, 1);
      } else {
                  int offset_src = nir_tex_instr_src_index(tex, nir_tex_src_offset);
   if (offset_src >= 0) {
      nir_src *offset = &tex->src[offset_src].src;
   nir_def *zero = nir_imm_intN_t(b, 0, offset->ssa->bit_size);
               if (ddx || ddy) {
      nir_def *def = nir_vec2(b, ddx->ssa, nir_imm_floatN_t(b, 0.0, ddx->ssa->bit_size));
   nir_src_rewrite(ddx, def);
   def = nir_vec2(b, ddy->ssa, nir_imm_floatN_t(b, 0.0, ddy->ssa->bit_size));
         } else if (tex->sampler_dim == GLSL_SAMPLER_DIM_CUBE) {
                     }
      static bool
   lower_tex(nir_builder *b, nir_instr *instr, void *options_)
   {
      const ac_nir_lower_tex_options *options = options_;
   if (instr->type != nir_instr_type_tex)
            nir_tex_instr *tex = nir_instr_as_tex(instr);
   int coord_idx = nir_tex_instr_src_index(tex, nir_tex_src_coord);
   if (coord_idx < 0 || nir_tex_instr_src_index(tex, nir_tex_src_backend1) >= 0)
            b->cursor = nir_before_instr(instr);
   nir_def *coords = tex->src[coord_idx].src.ssa;
   if (lower_tex_coords(b, tex, &coords, options)) {
      tex->coord_components = coords->num_components;
   nir_src_rewrite(&tex->src[coord_idx].src, coords);
                  }
      typedef struct {
      nir_intrinsic_instr *bary;
      } coord_info;
      static bool
   can_move_coord(nir_scalar scalar, coord_info *info)
   {
      if (scalar.def->bit_size != 32)
            if (nir_scalar_is_const(scalar))
            if (!nir_scalar_is_intrinsic(scalar))
            nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(scalar.def->parent_instr);
   if (intrin->intrinsic == nir_intrinsic_load_input) {
      info->bary = NULL;
   info->load = intrin;
               if (intrin->intrinsic != nir_intrinsic_load_interpolated_input)
            nir_scalar coord_x = nir_scalar_resolved(intrin->src[0].ssa, 0);
   nir_scalar coord_y = nir_scalar_resolved(intrin->src[0].ssa, 1);
   if (!nir_scalar_is_intrinsic(coord_x) || coord_x.comp != 0 ||
      !nir_scalar_is_intrinsic(coord_y) || coord_y.comp != 1)
         nir_intrinsic_instr *intrin_x = nir_instr_as_intrinsic(coord_x.def->parent_instr);
   nir_intrinsic_instr *intrin_y = nir_instr_as_intrinsic(coord_y.def->parent_instr);
   if (intrin_x->intrinsic != intrin_y->intrinsic ||
      (intrin_x->intrinsic != nir_intrinsic_load_barycentric_sample &&
   intrin_x->intrinsic != nir_intrinsic_load_barycentric_pixel &&
   intrin_x->intrinsic != nir_intrinsic_load_barycentric_centroid) ||
   nir_intrinsic_interp_mode(intrin_x) != nir_intrinsic_interp_mode(intrin_y))
         info->bary = intrin_x;
               }
      struct move_tex_coords_state {
      const ac_nir_lower_tex_options *options;
   unsigned num_wqm_vgprs;
      };
      static nir_def *
   build_coordinate(struct move_tex_coords_state *state, nir_scalar scalar, coord_info info)
   {
               if (nir_scalar_is_const(scalar))
            ASSERTED nir_src offset = *nir_get_io_offset_src(info.load);
            nir_def *zero = nir_imm_int(b, 0);
   nir_def *res;
   if (info.bary) {
      enum glsl_interp_mode interp_mode = nir_intrinsic_interp_mode(info.bary);
   nir_def *bary = nir_load_system_value(b, info.bary->intrinsic, interp_mode, 2, 32);
      } else {
         }
   nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(res->parent_instr);
   nir_intrinsic_set_base(intrin, nir_intrinsic_base(info.load));
   nir_intrinsic_set_component(intrin, nir_intrinsic_component(info.load) + scalar.comp);
   nir_intrinsic_set_dest_type(intrin, nir_intrinsic_dest_type(info.load));
   nir_intrinsic_set_io_semantics(intrin, nir_intrinsic_io_semantics(info.load));
      }
      static bool
   move_tex_coords(struct move_tex_coords_state *state, nir_function_impl *impl, nir_instr *instr)
   {
      nir_tex_instr *tex = nir_instr_as_tex(instr);
   if (tex->op != nir_texop_tex && tex->op != nir_texop_txb && tex->op != nir_texop_lod)
            switch (tex->sampler_dim) {
   case GLSL_SAMPLER_DIM_1D:
   case GLSL_SAMPLER_DIM_2D:
   case GLSL_SAMPLER_DIM_3D:
   case GLSL_SAMPLER_DIM_CUBE:
   case GLSL_SAMPLER_DIM_EXTERNAL:
         case GLSL_SAMPLER_DIM_RECT:
   case GLSL_SAMPLER_DIM_BUF:
   case GLSL_SAMPLER_DIM_MS:
   case GLSL_SAMPLER_DIM_SUBPASS:
   case GLSL_SAMPLER_DIM_SUBPASS_MS:
                  if (nir_tex_instr_src_index(tex, nir_tex_src_min_lod) != -1)
            nir_tex_src *src = &tex->src[nir_tex_instr_src_index(tex, nir_tex_src_coord)];
   nir_scalar components[NIR_MAX_VEC_COMPONENTS];
   coord_info infos[NIR_MAX_VEC_COMPONENTS];
   bool can_move_all = true;
   for (unsigned i = 0; i < tex->coord_components; i++) {
      components[i] = nir_scalar_resolved(src->src.ssa, i);
      }
   if (!can_move_all)
            int coord_base = 0;
   unsigned linear_vgpr_size = tex->coord_components;
   if (tex->sampler_dim == GLSL_SAMPLER_DIM_1D && state->options->gfx_level == GFX9)
         if (tex->sampler_dim == GLSL_SAMPLER_DIM_CUBE && tex->is_array)
         for (unsigned i = 0; i < tex->num_srcs; i++) {
      switch (tex->src[i].src_type) {
   case nir_tex_src_offset:
   case nir_tex_src_bias:
   case nir_tex_src_comparator:
      coord_base++;
   linear_vgpr_size++;
      default:
                     if (state->num_wqm_vgprs + linear_vgpr_size > state->options->max_wqm_vgprs)
            for (unsigned i = 0; i < tex->coord_components; i++)
            nir_def *linear_vgpr = nir_vec_scalars(&state->toplevel_b, components, tex->coord_components);
                     nir_tex_instr_remove_src(tex, nir_tex_instr_src_index(tex, nir_tex_src_coord));
                     int offset_src = nir_tex_instr_src_index(tex, nir_tex_src_offset);
   if (offset_src >= 0) /* Workaround requirement in nir_tex_instr_src_size(). */
                        }
      static bool
   move_fddxy(struct move_tex_coords_state *state, nir_function_impl *impl, nir_alu_instr *instr)
   {
      switch (instr->op) {
   case nir_op_fddx:
   case nir_op_fddy:
   case nir_op_fddx_fine:
   case nir_op_fddy_fine:
   case nir_op_fddx_coarse:
   case nir_op_fddy_coarse:
         default:
                  unsigned num_components = instr->def.num_components;
   nir_scalar components[NIR_MAX_VEC_COMPONENTS];
   coord_info infos[NIR_MAX_VEC_COMPONENTS];
   bool can_move_all = true;
   for (unsigned i = 0; i < num_components; i++) {
      components[i] = nir_scalar_chase_alu_src(nir_get_scalar(&instr->def, i), 0);
   components[i] = nir_scalar_chase_movs(components[i]);
      }
   if (!can_move_all || state->num_wqm_vgprs + num_components > state->options->max_wqm_vgprs)
            for (unsigned i = 0; i < num_components; i++) {
      nir_def *def = build_coordinate(state, components[i], infos[i]);
               nir_def *def = nir_vec_scalars(&state->toplevel_b, components, num_components);
   def = nir_build_alu1(&state->toplevel_b, instr->op, def);
                        }
      static bool
   move_coords_from_divergent_cf(struct move_tex_coords_state *state, nir_function_impl *impl,
         {
      bool progress = false;
   foreach_list_typed (nir_cf_node, cf_node, node, cf_list) {
      switch (cf_node->type) {
   case nir_cf_node_block: {
                        nir_foreach_instr (instr, block) {
                     if (instr->type == nir_instr_type_tex && (divergent_cf || *divergent_discard)) {
         } else if (instr->type == nir_instr_type_alu && (divergent_cf || *divergent_discard)) {
         } else if (instr->type == nir_instr_type_intrinsic) {
      nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(instr);
   switch (intrin->intrinsic) {
   case nir_intrinsic_discard:
   case nir_intrinsic_terminate:
      if (divergent_cf)
            case nir_intrinsic_discard_if:
   case nir_intrinsic_terminate_if:
      if (divergent_cf || nir_src_is_divergent(intrin->src[0]))
            default:
                        if (top_level && !*divergent_discard)
            }
   case nir_cf_node_if: {
      nir_if *nif = nir_cf_node_as_if(cf_node);
   bool divergent_discard_then = *divergent_discard;
   bool divergent_discard_else = *divergent_discard;
   bool then_else_divergent = divergent_cf || nir_src_is_divergent(nif->condition);
   progress |= move_coords_from_divergent_cf(state, impl, &nif->then_list,
         progress |= move_coords_from_divergent_cf(state, impl, &nif->else_list,
         *divergent_discard |= divergent_discard_then || divergent_discard_else;
      }
   case nir_cf_node_loop: {
      nir_loop *loop = nir_cf_node_as_loop(cf_node);
   assert(!nir_loop_has_continue_construct(loop));
   progress |=
            }
   case nir_cf_node_function:
                        }
      bool
   ac_nir_lower_tex(nir_shader *nir, const ac_nir_lower_tex_options *options)
   {
      bool progress = false;
   if (options->fix_derivs_in_divergent_cf) {
               struct move_tex_coords_state state;
   state.toplevel_b = nir_builder_create(impl);
   state.options = options;
            bool divergent_discard = false;
   if (move_coords_from_divergent_cf(&state, impl, &impl->body, &divergent_discard, false))
         else
               progress |= nir_shader_instructions_pass(
               }
