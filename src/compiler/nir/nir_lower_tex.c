   /*
   * Copyright © 2015 Broadcom
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
      /*
   * This lowering pass supports (as configured via nir_lower_tex_options)
   * various texture related conversions:
   *   + texture projector lowering: converts the coordinate division for
   *     texture projection to be done in ALU instructions instead of
   *     asking the texture operation to do so.
   *   + lowering RECT: converts the un-normalized RECT texture coordinates
   *     to normalized coordinates with txs plus ALU instructions
   *   + saturate s/t/r coords: to emulate certain texture clamp/wrap modes,
   *     inserts instructions to clamp specified coordinates to [0.0, 1.0].
   *     Note that this automatically triggers texture projector lowering if
   *     needed, since clamping must happen after projector lowering.
   *   + YUV-to-RGB conversion: to allow sampling YUV values as RGB values
   *     according to a specific YUV color space and range.
   */
      #include "nir.h"
   #include "nir_builder.h"
   #include "nir_builtin_builder.h"
   #include "nir_format_convert.h"
      typedef struct nir_const_value_3_4 {
         } nir_const_value_3_4;
      static const nir_const_value_3_4 bt601_limited_range_csc_coeffs = { {
      { { .f32 = 1.16438356f }, { .f32 = 1.16438356f }, { .f32 = 1.16438356f } },
   { { .f32 = 0.0f }, { .f32 = -0.39176229f }, { .f32 = 2.01723214f } },
      } };
   static const nir_const_value_3_4 bt601_full_range_csc_coeffs = { {
      { { .f32 = 1.0f }, { .f32 = 1.0f }, { .f32 = 1.0f } },
   { { .f32 = 0.0f }, { .f32 = -0.34413629f }, { .f32 = 1.772f } },
      } };
   static const nir_const_value_3_4 bt709_limited_range_csc_coeffs = { {
      { { .f32 = 1.16438356f }, { .f32 = 1.16438356f }, { .f32 = 1.16438356f } },
   { { .f32 = 0.0f }, { .f32 = -0.21324861f }, { .f32 = 2.11240179f } },
      } };
   static const nir_const_value_3_4 bt709_full_range_csc_coeffs = { {
      { { .f32 = 1.0f }, { .f32 = 1.0f }, { .f32 = 1.0f } },
   { { .f32 = 0.0f }, { .f32 = -0.18732427f }, { .f32 = 1.8556f } },
      } };
   static const nir_const_value_3_4 bt2020_limited_range_csc_coeffs = { {
      { { .f32 = 1.16438356f }, { .f32 = 1.16438356f }, { .f32 = 1.16438356f } },
   { { .f32 = 0.0f }, { .f32 = -0.18732610f }, { .f32 = 2.14177232f } },
      } };
   static const nir_const_value_3_4 bt2020_full_range_csc_coeffs = { {
      { { .f32 = 1.0f }, { .f32 = 1.0f }, { .f32 = 1.0f } },
   { { .f32 = 0.0f }, { .f32 = -0.16455313f }, { .f32 = 1.88140000f } },
      } };
      static const float bt601_limited_range_csc_offsets[3] = {
         };
   static const float bt601_full_range_csc_offsets[3] = {
         };
   static const float bt709_limited_range_csc_offsets[3] = {
         };
   static const float bt709_full_range_csc_offsets[3] = {
         };
   static const float bt2020_limited_range_csc_offsets[3] = {
         };
   static const float bt2020_full_range_csc_offsets[3] = {
         };
      static bool
   project_src(nir_builder *b, nir_tex_instr *tex)
   {
      nir_def *proj = nir_steal_tex_src(tex, nir_tex_src_projector);
   if (!proj)
            b->cursor = nir_before_instr(&tex->instr);
            /* Walk through the sources projecting the arguments. */
   for (unsigned i = 0; i < tex->num_srcs; i++) {
      switch (tex->src[i].src_type) {
   case nir_tex_src_coord:
   case nir_tex_src_comparator:
         default:
         }
   nir_def *unprojected =
                  /* Array indices don't get projected, so make an new vector with the
   * coordinate's array index untouched.
   */
   if (tex->is_array && tex->src[i].src_type == nir_tex_src_coord) {
      switch (tex->coord_components) {
   case 4:
      projected = nir_vec4(b,
                              case 3:
      projected = nir_vec3(b,
                        case 2:
      projected = nir_vec2(b,
                  default:
      unreachable("bad texture coord count for array");
                                 }
      static bool
   lower_offset(nir_builder *b, nir_tex_instr *tex)
   {
      nir_def *offset = nir_steal_tex_src(tex, nir_tex_src_offset);
   if (!offset)
            int coord_index = nir_tex_instr_src_index(tex, nir_tex_src_coord);
                              nir_def *offset_coord;
   if (nir_tex_instr_src_type(tex, coord_index) == nir_type_float) {
      if (tex->sampler_dim == GLSL_SAMPLER_DIM_RECT) {
         } else {
               if (b->shader->options->has_texture_scaling) {
      nir_def *idx = nir_imm_int(b, tex->texture_index);
      } else {
      nir_def *txs = nir_i2f32(b, nir_get_texture_size(b, tex));
               offset_coord = nir_fadd(b, coord,
                     } else {
                  if (tex->is_array) {
      /* The offset is not applied to the array index */
   if (tex->coord_components == 2) {
      offset_coord = nir_vec2(b, nir_channel(b, offset_coord, 0),
      } else if (tex->coord_components == 3) {
      offset_coord = nir_vec3(b, nir_channel(b, offset_coord, 0),
            } else {
                                 }
      static void
   lower_rect(nir_builder *b, nir_tex_instr *tex)
   {
      /* Set the sampler_dim to 2D here so that get_texture_size picks up the
   * right dimensionality.
   */
            nir_def *txs = nir_i2f32(b, nir_get_texture_size(b, tex));
   nir_def *scale = nir_frcp(b, txs);
            if (coord_index != -1) {
      nir_def *coords =
               }
      static void
   lower_rect_tex_scale(nir_builder *b, nir_tex_instr *tex)
   {
               nir_def *idx = nir_imm_int(b, tex->texture_index);
   nir_def *scale = nir_load_texture_scale(b, 32, idx);
            if (coord_index != -1) {
      nir_def *coords =
               }
      static void
   lower_lod(nir_builder *b, nir_tex_instr *tex, nir_def *lod)
   {
      assert(tex->op == nir_texop_tex || tex->op == nir_texop_txb);
   assert(nir_tex_instr_src_index(tex, nir_tex_src_lod) < 0);
   assert(nir_tex_instr_src_index(tex, nir_tex_src_ddx) < 0);
            /* If we have a bias, add it in */
   nir_def *bias = nir_steal_tex_src(tex, nir_tex_src_bias);
   if (bias)
            /* If we have a minimum LOD, clamp LOD accordingly */
   nir_def *min_lod = nir_steal_tex_src(tex, nir_tex_src_min_lod);
   if (min_lod)
            nir_tex_instr_add_src(tex, nir_tex_src_lod, lod);
      }
      static void
   lower_implicit_lod(nir_builder *b, nir_tex_instr *tex)
   {
      b->cursor = nir_before_instr(&tex->instr);
      }
      static void
   lower_zero_lod(nir_builder *b, nir_tex_instr *tex)
   {
               if (tex->op == nir_texop_lod) {
      nir_def_rewrite_uses(&tex->def, nir_imm_int(b, 0));
   nir_instr_remove(&tex->instr);
                  }
      static nir_def *
   sample_plane(nir_builder *b, nir_tex_instr *tex, int plane,
         {
      assert(nir_tex_instr_dest_size(tex) == 4);
   assert(nir_alu_type_get_base_type(tex->dest_type) == nir_type_float);
   assert(tex->op == nir_texop_tex);
            nir_tex_instr *plane_tex =
         for (unsigned i = 0; i < tex->num_srcs; i++) {
      plane_tex->src[i].src = nir_src_for_ssa(tex->src[i].src.ssa);
      }
   plane_tex->src[tex->num_srcs] = nir_tex_src_for_ssa(nir_tex_src_plane,
         plane_tex->op = nir_texop_tex;
   plane_tex->sampler_dim = GLSL_SAMPLER_DIM_2D;
   plane_tex->dest_type = nir_type_float | tex->def.bit_size;
            plane_tex->texture_index = tex->texture_index;
            nir_def_init(&plane_tex->instr, &plane_tex->def, 4,
                     /* If scaling_factor is set, return a scaled value. */
   if (options->scale_factors[tex->texture_index])
      return nir_fmul_imm(b, &plane_tex->def,
            }
      static void
   convert_yuv_to_rgb(nir_builder *b, nir_tex_instr *tex,
                     nir_def *y, nir_def *u, nir_def *v,
   {
         const float *offset_vals;
   const nir_const_value_3_4 *m;
   assert((options->bt709_external & options->bt2020_external) == 0);
   if (options->yuv_full_range_external & (1u << texture_index)) {
      if (options->bt709_external & (1u << texture_index)) {
      m = &bt709_full_range_csc_coeffs;
      } else if (options->bt2020_external & (1u << texture_index)) {
      m = &bt2020_full_range_csc_coeffs;
      } else {
      m = &bt601_full_range_csc_coeffs;
         } else {
      if (options->bt709_external & (1u << texture_index)) {
      m = &bt709_limited_range_csc_coeffs;
      } else if (options->bt2020_external & (1u << texture_index)) {
      m = &bt2020_limited_range_csc_coeffs;
      } else {
      m = &bt601_limited_range_csc_coeffs;
                           nir_def *offset =
      nir_vec4(b,
            nir_imm_floatN_t(b, offset_vals[0], a->bit_size),
                     nir_def *m0 = nir_f2fN(b, nir_build_imm(b, 4, 32, m->v[0]), bit_size);
   nir_def *m1 = nir_f2fN(b, nir_build_imm(b, 4, 32, m->v[1]), bit_size);
            nir_def *result =
               }
      static void
   lower_y_uv_external(nir_builder *b, nir_tex_instr *tex,
               {
               nir_def *y = sample_plane(b, tex, 0, options);
            convert_yuv_to_rgb(b, tex,
                     nir_channel(b, y, 0),
   nir_channel(b, uv, 0),
      }
      static void
   lower_y_vu_external(nir_builder *b, nir_tex_instr *tex,
               {
               nir_def *y = sample_plane(b, tex, 0, options);
            convert_yuv_to_rgb(b, tex,
                     nir_channel(b, y, 0),
   nir_channel(b, vu, 1),
      }
      static void
   lower_y_u_v_external(nir_builder *b, nir_tex_instr *tex,
               {
               nir_def *y = sample_plane(b, tex, 0, options);
   nir_def *u = sample_plane(b, tex, 1, options);
            convert_yuv_to_rgb(b, tex,
                     nir_channel(b, y, 0),
   nir_channel(b, u, 0),
      }
      static void
   lower_yx_xuxv_external(nir_builder *b, nir_tex_instr *tex,
               {
               nir_def *y = sample_plane(b, tex, 0, options);
            convert_yuv_to_rgb(b, tex,
                     nir_channel(b, y, 0),
   nir_channel(b, xuxv, 1),
      }
      static void
   lower_yx_xvxu_external(nir_builder *b, nir_tex_instr *tex,
               {
               nir_def *y = sample_plane(b, tex, 0, options);
            convert_yuv_to_rgb(b, tex,
                     nir_channel(b, y, 0),
   nir_channel(b, xvxu, 3),
      }
      static void
   lower_xy_uxvx_external(nir_builder *b, nir_tex_instr *tex,
               {
               nir_def *y = sample_plane(b, tex, 0, options);
            convert_yuv_to_rgb(b, tex,
                     nir_channel(b, y, 1),
   nir_channel(b, uxvx, 0),
      }
      static void
   lower_xy_vxux_external(nir_builder *b, nir_tex_instr *tex,
               {
               nir_def *y = sample_plane(b, tex, 0, options);
            convert_yuv_to_rgb(b, tex,
                     nir_channel(b, y, 1),
   nir_channel(b, vxux, 2),
      }
      static void
   lower_ayuv_external(nir_builder *b, nir_tex_instr *tex,
               {
                        convert_yuv_to_rgb(b, tex,
                     nir_channel(b, ayuv, 2),
   nir_channel(b, ayuv, 1),
      }
      static void
   lower_y41x_external(nir_builder *b, nir_tex_instr *tex,
               {
                        convert_yuv_to_rgb(b, tex,
                     nir_channel(b, y41x, 1),
   nir_channel(b, y41x, 0),
      }
      static void
   lower_xyuv_external(nir_builder *b, nir_tex_instr *tex,
               {
                        convert_yuv_to_rgb(b, tex,
                     nir_channel(b, xyuv, 2),
   nir_channel(b, xyuv, 1),
      }
      static void
   lower_yuv_external(nir_builder *b, nir_tex_instr *tex,
               {
                        convert_yuv_to_rgb(b, tex,
                     nir_channel(b, yuv, 0),
   nir_channel(b, yuv, 1),
      }
      static void
   lower_yu_yv_external(nir_builder *b, nir_tex_instr *tex,
               {
                        convert_yuv_to_rgb(b, tex,
                     nir_channel(b, yuv, 1),
   nir_channel(b, yuv, 2),
      }
      static void
   lower_yv_yu_external(nir_builder *b, nir_tex_instr *tex,
               {
                        convert_yuv_to_rgb(b, tex,
                     nir_channel(b, yuv, 2),
   nir_channel(b, yuv, 1),
      }
      /*
   * Converts a nir_texop_txd instruction to nir_texop_txl with the given lod
   * computed from the gradients.
   */
   static void
   replace_gradient_with_lod(nir_builder *b, nir_def *lod, nir_tex_instr *tex)
   {
               nir_tex_instr_remove_src(tex, nir_tex_instr_src_index(tex, nir_tex_src_ddx));
            /* If we have a minimum LOD, clamp LOD accordingly */
   nir_def *min_lod = nir_steal_tex_src(tex, nir_tex_src_min_lod);
   if (min_lod)
            nir_tex_instr_add_src(tex, nir_tex_src_lod, lod);
      }
      static void
   lower_gradient_cube_map(nir_builder *b, nir_tex_instr *tex)
   {
      assert(tex->sampler_dim == GLSL_SAMPLER_DIM_CUBE);
            /* Use textureSize() to get the width and height of LOD 0 */
            /* Cubemap texture lookups first generate a texture coordinate normalized
   * to [-1, 1] on the appropiate face. The appropiate face is determined
   * by which component has largest magnitude and its sign. The texture
   * coordinate is the quotient of the remaining texture coordinates against
   * that absolute value of the component of largest magnitude. This
   * division requires that the computing of the derivative of the texel
   * coordinate must use the quotient rule. The high level GLSL code is as
   * follows:
   *
   * Step 1: selection
   *
   * vec3 abs_p, Q, dQdx, dQdy;
   * abs_p = abs(ir->coordinate);
   * if (abs_p.x >= max(abs_p.y, abs_p.z)) {
   *    Q = ir->coordinate.yzx;
   *    dQdx = ir->lod_info.grad.dPdx.yzx;
   *    dQdy = ir->lod_info.grad.dPdy.yzx;
   * }
   * if (abs_p.y >= max(abs_p.x, abs_p.z)) {
   *    Q = ir->coordinate.xzy;
   *    dQdx = ir->lod_info.grad.dPdx.xzy;
   *    dQdy = ir->lod_info.grad.dPdy.xzy;
   * }
   * if (abs_p.z >= max(abs_p.x, abs_p.y)) {
   *    Q = ir->coordinate;
   *    dQdx = ir->lod_info.grad.dPdx;
   *    dQdy = ir->lod_info.grad.dPdy;
   * }
   *
   * Step 2: use quotient rule to compute derivative. The normalized to
   * [-1, 1] texel coordinate is given by Q.xy / (sign(Q.z) * Q.z). We are
   * only concerned with the magnitudes of the derivatives whose values are
   * not affected by the sign. We drop the sign from the computation.
   *
   * vec2 dx, dy;
   * float recip;
   *
   * recip = 1.0 / Q.z;
   * dx = recip * ( dQdx.xy - Q.xy * (dQdx.z * recip) );
   * dy = recip * ( dQdy.xy - Q.xy * (dQdy.z * recip) );
   *
   * Step 3: compute LOD. At this point we have the derivatives of the
   * texture coordinates normalized to [-1,1]. We take the LOD to be
   *  result = log2(max(sqrt(dot(dx, dx)), sqrt(dy, dy)) * 0.5 * L)
   *         = -1.0 + log2(max(sqrt(dot(dx, dx)), sqrt(dy, dy)) * L)
   *         = -1.0 + log2(sqrt(max(dot(dx, dx), dot(dy,dy))) * L)
   *         = -1.0 + log2(sqrt(L * L * max(dot(dx, dx), dot(dy,dy))))
   *         = -1.0 + 0.5 * log2(L * L * max(dot(dx, dx), dot(dy,dy)))
   * where L is the dimension of the cubemap. The code is:
   *
   * float M, result;
   * M = max(dot(dx, dx), dot(dy, dy));
   * L = textureSize(sampler, 0).x;
   * result = -1.0 + 0.5 * log2(L * L * M);
            /* coordinate */
   nir_def *p =
            /* unmodified dPdx, dPdy values */
   nir_def *dPdx =
         nir_def *dPdy =
            nir_def *abs_p = nir_fabs(b, p);
   nir_def *abs_p_x = nir_channel(b, abs_p, 0);
   nir_def *abs_p_y = nir_channel(b, abs_p, 1);
            /* 1. compute selector */
            nir_def *cond_z = nir_fge(b, abs_p_z, nir_fmax(b, abs_p_x, abs_p_y));
            unsigned yzx[3] = { 1, 2, 0 };
            Q = nir_bcsel(b, cond_z,
               p,
            dQdx = nir_bcsel(b, cond_z,
                  dPdx,
         dQdy = nir_bcsel(b, cond_z,
                  dPdy,
                  /* tmp = Q.xy * recip;
   * dx = recip * ( dQdx.xy - (tmp * dQdx.z) );
   * dy = recip * ( dQdy.xy - (tmp * dQdy.z) );
   */
            nir_def *Q_xy = nir_trim_vector(b, Q, 2);
            nir_def *dQdx_xy = nir_trim_vector(b, dQdx, 2);
   nir_def *dQdx_z = nir_channel(b, dQdx, 2);
   nir_def *dx =
            nir_def *dQdy_xy = nir_trim_vector(b, dQdy, 2);
   nir_def *dQdy_z = nir_channel(b, dQdy, 2);
   nir_def *dy =
            /* M = max(dot(dx, dx), dot(dy, dy)); */
            /* size has textureSize() of LOD 0 */
            /* lod = -1.0 + 0.5 * log2(L * L * M); */
   nir_def *lod =
      nir_fadd(b,
            nir_imm_float(b, -1.0f),
            /* 3. Replace the gradient instruction with an equivalent lod instruction */
      }
      static void
   lower_gradient(nir_builder *b, nir_tex_instr *tex)
   {
      /* Cubes are more complicated and have their own function */
   if (tex->sampler_dim == GLSL_SAMPLER_DIM_CUBE) {
      lower_gradient_cube_map(b, tex);
               assert(tex->sampler_dim != GLSL_SAMPLER_DIM_CUBE);
            /* Use textureSize() to get the width and height of LOD 0 */
   unsigned component_mask;
   switch (tex->sampler_dim) {
   case GLSL_SAMPLER_DIM_3D:
      component_mask = 7;
      case GLSL_SAMPLER_DIM_1D:
      component_mask = 1;
      default:
      component_mask = 3;
               nir_def *size =
      nir_channels(b, nir_i2f32(b, nir_get_texture_size(b, tex)),
         /* Scale the gradients by width and height.  Effectively, the incoming
   * gradients are s'(x,y), t'(x,y), and r'(x,y) from equation 3.19 in the
   * GL 3.0 spec; we want u'(x,y), which is w_t * s'(x,y).
   */
   nir_def *ddx =
         nir_def *ddy =
            nir_def *dPdx = nir_fmul(b, ddx, size);
            nir_def *rho;
   if (dPdx->num_components == 1) {
         } else {
      rho = nir_fmax(b,
                     /* lod = log2(rho).  We're ignoring GL state biases for now. */
            /* Replace the gradient instruction with an equivalent lod instruction */
      }
      /* tex(s, coord) = txd(s, coord, dfdx(coord), dfdy(coord)) */
   static nir_tex_instr *
   lower_tex_to_txd(nir_builder *b, nir_tex_instr *tex)
   {
      b->cursor = nir_after_instr(&tex->instr);
            txd->op = nir_texop_txd;
   txd->sampler_dim = tex->sampler_dim;
   txd->dest_type = tex->dest_type;
   txd->coord_components = tex->coord_components;
   txd->texture_index = tex->texture_index;
   txd->sampler_index = tex->sampler_index;
   txd->is_array = tex->is_array;
   txd->is_shadow = tex->is_shadow;
            /* reuse existing srcs */
   for (unsigned i = 0; i < tex->num_srcs; i++) {
      txd->src[i].src = nir_src_for_ssa(tex->src[i].src.ssa);
      }
   int coord_idx = nir_tex_instr_src_index(tex, nir_tex_src_coord);
   assert(coord_idx >= 0);
   nir_def *coord = tex->src[coord_idx].src.ssa;
   /* don't take the derivative of the array index */
   if (tex->is_array)
         nir_def *dfdx = nir_fddx(b, coord);
   nir_def *dfdy = nir_fddy(b, coord);
   txd->src[tex->num_srcs] = nir_tex_src_for_ssa(nir_tex_src_ddx, dfdx);
            nir_def_init(&txd->instr, &txd->def,
               nir_builder_instr_insert(b, &txd->instr);
   nir_def_rewrite_uses(&tex->def, &txd->def);
   nir_instr_remove(&tex->instr);
      }
      /* txb(s, coord, bias) = txl(s, coord, lod(s, coord).y + bias) */
   static nir_tex_instr *
   lower_txb_to_txl(nir_builder *b, nir_tex_instr *tex)
   {
      b->cursor = nir_after_instr(&tex->instr);
            txl->op = nir_texop_txl;
   txl->sampler_dim = tex->sampler_dim;
   txl->dest_type = tex->dest_type;
   txl->coord_components = tex->coord_components;
   txl->texture_index = tex->texture_index;
   txl->sampler_index = tex->sampler_index;
   txl->is_array = tex->is_array;
   txl->is_shadow = tex->is_shadow;
            /* reuse all but bias src */
   for (int i = 0; i < tex->num_srcs; i++) {
      if (tex->src[i].src_type != nir_tex_src_bias) {
      txl->src[i].src = nir_src_for_ssa(tex->src[i].src.ssa);
         }
            int bias_idx = nir_tex_instr_src_index(tex, nir_tex_src_bias);
   assert(bias_idx >= 0);
   lod = nir_fadd(b, lod, tex->src[bias_idx].src.ssa);
            nir_def_init(&txl->instr, &txl->def,
               nir_builder_instr_insert(b, &txl->instr);
   nir_def_rewrite_uses(&tex->def, &txl->def);
   nir_instr_remove(&tex->instr);
      }
      static nir_tex_instr *
   saturate_src(nir_builder *b, nir_tex_instr *tex, unsigned sat_mask)
   {
      if (tex->op == nir_texop_tex)
         else if (tex->op == nir_texop_txb)
            b->cursor = nir_before_instr(&tex->instr);
            if (coord_index != -1) {
      nir_def *src =
            /* split src into components: */
                     for (unsigned j = 0; j < tex->coord_components; j++)
            /* clamp requested components, array index does not get clamped: */
   unsigned ncomp = tex->coord_components;
   if (tex->is_array)
            for (unsigned j = 0; j < ncomp; j++) {
      if ((1 << j) & sat_mask) {
      if (tex->sampler_dim == GLSL_SAMPLER_DIM_RECT) {
      /* non-normalized texture coords, so clamp to texture
   * size rather than [0.0, 1.0]
   */
   nir_def *txs = nir_i2f32(b, nir_get_texture_size(b, tex));
   comp[j] = nir_fmax(b, comp[j], nir_imm_float(b, 0.0));
      } else {
                        /* and move the result back into a single vecN: */
               }
      }
      static nir_def *
   get_zero_or_one(nir_builder *b, nir_alu_type type, uint8_t swizzle_val)
   {
                        if (swizzle_val == 4) {
         } else {
      assert(swizzle_val == 5);
   if (type == nir_type_float32)
         else
                  }
      static void
   swizzle_tg4_broadcom(nir_builder *b, nir_tex_instr *tex)
   {
               assert(nir_tex_instr_dest_size(tex) == 4);
   unsigned swiz[4] = { 2, 3, 1, 0 };
            nir_def_rewrite_uses_after(&tex->def, swizzled,
      }
      static void
   swizzle_result(nir_builder *b, nir_tex_instr *tex, const uint8_t swizzle[4])
   {
               nir_def *swizzled;
   if (tex->op == nir_texop_tg4) {
      if (swizzle[tex->component] < 4) {
      /* This one's easy */
   tex->component = swizzle[tex->component];
      } else {
            } else {
      assert(nir_tex_instr_dest_size(tex) == 4);
   if (swizzle[0] < 4 && swizzle[1] < 4 &&
      swizzle[2] < 4 && swizzle[3] < 4) {
   unsigned swiz[4] = { swizzle[0], swizzle[1], swizzle[2], swizzle[3] };
   /* We have no 0s or 1s, just emit a swizzling MOV */
      } else {
      nir_scalar srcs[4];
   for (unsigned i = 0; i < 4; i++) {
      if (swizzle[i] < 4) {
         } else {
            }
                  nir_def_rewrite_uses_after(&tex->def, swizzled,
      }
      static void
   linearize_srgb_result(nir_builder *b, nir_tex_instr *tex)
   {
      assert(nir_tex_instr_dest_size(tex) == 4);
                     nir_def *rgb =
            /* alpha is untouched: */
   nir_def *result = nir_vec4(b,
                              nir_def_rewrite_uses_after(&tex->def, result,
      }
      /**
   * Lowers texture instructions from giving a vec4 result to a vec2 of f16,
   * i16, or u16, or a single unorm4x8 value.
   *
   * Note that we don't change the destination num_components, because
   * nir_tex_instr_dest_size() will still return 4.  The driver is just expected
   * to not store the other channels, given that nothing at the NIR level will
   * read them.
   */
   static bool
   lower_tex_packing(nir_builder *b, nir_tex_instr *tex,
         {
                        assert(options->lower_tex_packing_cb);
   enum nir_lower_tex_packing packing =
            switch (packing) {
   case nir_lower_tex_packing_none:
            case nir_lower_tex_packing_16: {
               switch (nir_alu_type_get_base_type(tex->dest_type)) {
   case nir_type_float:
      switch (nir_tex_instr_dest_size(tex)) {
   case 1:
      assert(tex->is_shadow && tex->is_new_style_shadow);
   color = nir_unpack_half_2x16_split_x(b, nir_channel(b, color, 0));
      case 2: {
      nir_def *rg = nir_channel(b, color, 0);
   color = nir_vec2(b,
                  }
   case 4: {
      nir_def *rg = nir_channel(b, color, 0);
   nir_def *ba = nir_channel(b, color, 1);
   color = nir_vec4(b,
                  nir_unpack_half_2x16_split_x(b, rg),
         }
   default:
                     case nir_type_int:
                  case nir_type_uint:
                  default:
         }
               case nir_lower_tex_packing_8:
      assert(nir_alu_type_get_base_type(tex->dest_type) == nir_type_float);
   color = nir_unpack_unorm_4x8(b, nir_channel(b, color, 0));
               nir_def_rewrite_uses_after(&tex->def, color,
            }
      static bool
   sampler_index_lt(nir_tex_instr *tex, unsigned max)
   {
                        int sampler_offset_idx =
         if (sampler_offset_idx >= 0) {
      if (!nir_src_is_const(tex->src[sampler_offset_idx].src))
                           }
      static bool
   lower_tg4_offsets(nir_builder *b, nir_tex_instr *tex)
   {
      assert(tex->op == nir_texop_tg4);
   assert(nir_tex_instr_has_explicit_tg4_offsets(tex));
                     nir_scalar dest[5] = { 0 };
   nir_def *residency = NULL;
   for (unsigned i = 0; i < 4; ++i) {
      nir_tex_instr *tex_copy = nir_tex_instr_create(b->shader, tex->num_srcs + 1);
   tex_copy->op = tex->op;
   tex_copy->coord_components = tex->coord_components;
   tex_copy->sampler_dim = tex->sampler_dim;
   tex_copy->is_array = tex->is_array;
   tex_copy->is_shadow = tex->is_shadow;
   tex_copy->is_new_style_shadow = tex->is_new_style_shadow;
   tex_copy->is_sparse = tex->is_sparse;
   tex_copy->is_gather_implicit_lod = tex->is_gather_implicit_lod;
   tex_copy->component = tex->component;
   tex_copy->dest_type = tex->dest_type;
   tex_copy->texture_index = tex->texture_index;
   tex_copy->sampler_index = tex->sampler_index;
            for (unsigned j = 0; j < tex->num_srcs; ++j) {
      tex_copy->src[j].src = nir_src_for_ssa(tex->src[j].src.ssa);
               nir_def *offset = nir_imm_ivec2(b, tex->tg4_offsets[i][0],
         nir_tex_src src = nir_tex_src_for_ssa(nir_tex_src_offset, offset);
            nir_def_init(&tex_copy->instr, &tex_copy->def,
                     dest[i] = nir_get_scalar(&tex_copy->def, 3);
   if (tex->is_sparse) {
      nir_def *code = nir_channel(b, &tex_copy->def, 4);
   if (residency)
         else
         }
            nir_def *res = nir_vec_scalars(b, dest, tex->def.num_components);
   nir_def_rewrite_uses(&tex->def, res);
               }
      static bool
   nir_lower_txs_lod(nir_builder *b, nir_tex_instr *tex)
   {
      int lod_idx = nir_tex_instr_src_index(tex, nir_tex_src_lod);
   if (lod_idx < 0 ||
      (nir_src_is_const(tex->src[lod_idx].src) &&
   nir_src_as_int(tex->src[lod_idx].src) == 0))
                  b->cursor = nir_before_instr(&tex->instr);
            /* Replace the non-0-LOD in the initial TXS operation by a 0-LOD. */
            /* TXS(LOD) = max(TXS(0) >> LOD, 1)
   * But we do min(TXS(0), TXS(LOD)) to catch the case of a null surface,
   * which should return 0, not 1.
   */
   b->cursor = nir_after_instr(&tex->instr);
   nir_def *minified = nir_imin(b, &tex->def,
                  /* Make sure the component encoding the array size (if any) is not
   * minified.
   */
   if (tex->is_array) {
               assert(dest_size <= ARRAY_SIZE(comp));
   for (unsigned i = 0; i < dest_size - 1; i++)
            comp[dest_size - 1] = nir_channel(b, &tex->def, dest_size - 1);
               nir_def_rewrite_uses_after(&tex->def, minified,
            }
      static void
   nir_lower_txs_cube_array(nir_builder *b, nir_tex_instr *tex)
   {
      assert(tex->sampler_dim == GLSL_SAMPLER_DIM_CUBE && tex->is_array);
                     assert(tex->def.num_components == 3);
   nir_def *size = &tex->def;
   size = nir_vec3(b, nir_channel(b, size, 1),
                           }
      /* Adjust the sample index according to AMD FMASK (fragment mask).
   *
   * For uncompressed MSAA surfaces, FMASK should return 0x76543210,
   * which is the identity mapping. Each nibble says which physical sample
   * should be fetched to get that sample.
   *
   * For example, 0x11111100 means there are only 2 samples stored and
   * the second sample covers 3/4 of the pixel. When reading samples 0
   * and 1, return physical sample 0 (determined by the first two 0s
   * in FMASK), otherwise return physical sample 1.
   *
   * The sample index should be adjusted as follows:
   *   sample_index = ubfe(fmask, sample_index * 4, 3);
   *
   * Only extract 3 bits because EQAA can generate number 8 in FMASK, which
   * means the physical sample index is unknown. We can map 8 to any valid
   * sample index, and extracting only 3 bits will map it to 0, which works
   * with all MSAA modes.
   */
   static void
   nir_lower_ms_txf_to_fragment_fetch(nir_builder *b, nir_tex_instr *tex)
   {
                        /* Create FMASK fetch. */
   assert(tex->texture_index == 0);
   nir_tex_instr *fmask_fetch = nir_tex_instr_create(b->shader, tex->num_srcs - 1);
   fmask_fetch->op = nir_texop_fragment_mask_fetch_amd;
   fmask_fetch->coord_components = tex->coord_components;
   fmask_fetch->sampler_dim = tex->sampler_dim;
   fmask_fetch->is_array = tex->is_array;
   fmask_fetch->texture_non_uniform = tex->texture_non_uniform;
   fmask_fetch->dest_type = nir_type_uint32;
            fmask_fetch->num_srcs = 0;
   for (unsigned i = 0; i < tex->num_srcs; i++) {
      if (tex->src[i].src_type == nir_tex_src_ms_index)
         nir_tex_src *src = &fmask_fetch->src[fmask_fetch->num_srcs++];
   src->src = nir_src_for_ssa(tex->src[i].src.ssa);
                        /* Obtain new sample index. */
   int ms_index = nir_tex_instr_src_index(tex, nir_tex_src_ms_index);
   assert(ms_index >= 0);
   nir_src sample = tex->src[ms_index].src;
   nir_def *new_sample = nir_ubfe(b, &fmask_fetch->def,
            /* Update instruction. */
   tex->op = nir_texop_fragment_fetch_amd;
      }
      static void
   nir_lower_samples_identical_to_fragment_fetch(nir_builder *b, nir_tex_instr *tex)
   {
               nir_tex_instr *fmask_fetch = nir_instr_as_tex(nir_instr_clone(b->shader, &tex->instr));
   fmask_fetch->op = nir_texop_fragment_mask_fetch_amd;
   fmask_fetch->dest_type = nir_type_uint32;
   nir_def_init(&fmask_fetch->instr, &fmask_fetch->def, 1, 32);
            nir_def_rewrite_uses(&tex->def, nir_ieq_imm(b, &fmask_fetch->def, 0));
      }
      static void
   nir_lower_lod_zero_width(nir_builder *b, nir_tex_instr *tex)
   {
      int coord_index = nir_tex_instr_src_index(tex, nir_tex_src_coord);
                     nir_def *is_zero = nir_imm_true(b);
   for (unsigned i = 0; i < tex->coord_components; i++) {
               /* Compute the sum of the absolute values of derivatives. */
   nir_def *dfdx = nir_fddx(b, coord);
   nir_def *dfdy = nir_fddy(b, coord);
            /* Check if the sum is 0. */
               /* Replace the raw LOD by -FLT_MAX if the sum is 0 for all coordinates. */
   nir_def *adjusted_lod =
      nir_bcsel(b, is_zero, nir_imm_float(b, -FLT_MAX),
         nir_def *def =
               }
      static bool
   lower_index_to_offset(nir_builder *b, nir_tex_instr *tex)
   {
      bool progress = false;
            for (unsigned i = 0; i < tex->num_srcs; i++) {
      unsigned *index;
   switch (tex->src[i].src_type) {
   case nir_tex_src_texture_offset:
      index = &tex->texture_index;
      case nir_tex_src_sampler_offset:
      index = &tex->sampler_index;
      default:
                  /* If there's no base index, there's nothing to lower */
   if ((*index) == 0)
            nir_def *sum = nir_iadd_imm(b, tex->src[i].src.ssa, *index);
   nir_src_rewrite(&tex->src[i].src, sum);
   *index = 0;
                  }
      static bool
   nir_lower_tex_block(nir_block *block, nir_builder *b,
               {
               nir_foreach_instr_safe(instr, block) {
      if (instr->type != nir_instr_type_tex)
            nir_tex_instr *tex = nir_instr_as_tex(instr);
            /* mask of src coords to saturate (clamp): */
   unsigned sat_mask = 0;
   /* ignore saturate for txf ops: these don't use samplers and can't GL_CLAMP */
   if (nir_tex_instr_need_sampler(tex)) {
      if ((1 << tex->sampler_index) & options->saturate_r)
         if ((1 << tex->sampler_index) & options->saturate_t)
         if ((1 << tex->sampler_index) & options->saturate_s)
               if (options->lower_index_to_offset)
            /* If we are clamping any coords, we must lower projector first
   * as clamping happens *after* projection:
   */
   if (lower_txp || sat_mask ||
      (options->lower_txp_array && tex->is_array)) {
               if ((tex->op == nir_texop_txf && options->lower_txf_offset) ||
      (sat_mask && nir_tex_instr_src_index(tex, nir_tex_src_coord) >= 0) ||
   (tex->sampler_dim == GLSL_SAMPLER_DIM_RECT &&
   options->lower_rect_offset) ||
   (options->lower_offset_filter &&
   options->lower_offset_filter(instr, options->callback_data))) {
               if ((tex->sampler_dim == GLSL_SAMPLER_DIM_RECT) && options->lower_rect &&
      tex->op != nir_texop_txf) {
   if (nir_tex_instr_is_query(tex))
         else if (compiler_options->has_texture_scaling)
                                    unsigned texture_index = tex->texture_index;
   uint32_t texture_mask = 1u << texture_index;
   int tex_index = nir_tex_instr_src_index(tex, nir_tex_src_texture_deref);
   if (tex_index >= 0) {
      nir_deref_instr *deref = nir_src_as_deref(tex->src[tex_index].src);
   nir_variable *var = nir_deref_instr_get_variable(deref);
   texture_index = var ? var->data.binding : 0;
               if (texture_mask & options->lower_y_uv_external) {
      lower_y_uv_external(b, tex, options, texture_index);
               if (texture_mask & options->lower_y_vu_external) {
      lower_y_vu_external(b, tex, options, texture_index);
               if (texture_mask & options->lower_y_u_v_external) {
      lower_y_u_v_external(b, tex, options, texture_index);
               if (texture_mask & options->lower_yx_xuxv_external) {
      lower_yx_xuxv_external(b, tex, options, texture_index);
               if (texture_mask & options->lower_yx_xvxu_external) {
      lower_yx_xvxu_external(b, tex, options, texture_index);
               if (texture_mask & options->lower_xy_uxvx_external) {
      lower_xy_uxvx_external(b, tex, options, texture_index);
               if (texture_mask & options->lower_xy_vxux_external) {
      lower_xy_vxux_external(b, tex, options, texture_index);
               if (texture_mask & options->lower_ayuv_external) {
      lower_ayuv_external(b, tex, options, texture_index);
               if (texture_mask & options->lower_xyuv_external) {
      lower_xyuv_external(b, tex, options, texture_index);
               if (texture_mask & options->lower_yuv_external) {
      lower_yuv_external(b, tex, options, texture_index);
               if ((1 << tex->texture_index) & options->lower_yu_yv_external) {
      lower_yu_yv_external(b, tex, options, texture_index);
               if ((1 << tex->texture_index) & options->lower_yv_yu_external) {
      lower_yv_yu_external(b, tex, options, texture_index);
               if ((1 << tex->texture_index) & options->lower_y41x_external) {
      lower_y41x_external(b, tex, options, texture_index);
               if (sat_mask) {
      tex = saturate_src(b, tex, sat_mask);
               if (tex->op == nir_texop_tg4 && options->lower_tg4_broadcom_swizzle) {
      swizzle_tg4_broadcom(b, tex);
               if ((texture_mask & options->swizzle_result) &&
      !nir_tex_instr_is_query(tex) &&
   !(tex->is_shadow && tex->is_new_style_shadow)) {
   swizzle_result(b, tex, options->swizzles[tex->texture_index]);
               /* should be after swizzle so we know which channels are rgb: */
   if ((texture_mask & options->lower_srgb) &&
      !nir_tex_instr_is_query(tex) && !tex->is_shadow) {
   linearize_srgb_result(b, tex);
               const bool has_min_lod =
         const bool has_offset =
            if (tex->op == nir_texop_txb && tex->is_shadow && has_min_lod &&
      options->lower_txb_shadow_clamp) {
   lower_implicit_lod(b, tex);
               if (options->lower_tex_packing_cb &&
      tex->op != nir_texop_txs &&
   tex->op != nir_texop_query_levels &&
   tex->op != nir_texop_texture_samples) {
               if (tex->op == nir_texop_txd &&
      (options->lower_txd ||
   (options->lower_txd_clamp && has_min_lod) ||
   (options->lower_txd_shadow && tex->is_shadow) ||
   (options->lower_txd_shadow_clamp && tex->is_shadow && has_min_lod) ||
   (options->lower_txd_offset_clamp && has_offset && has_min_lod) ||
   (options->lower_txd_clamp_bindless_sampler && has_min_lod &&
         (options->lower_txd_clamp_if_sampler_index_not_lt_16 &&
         (options->lower_txd_cube_map &&
         (options->lower_txd_3d &&
         (options->lower_txd_array && tex->is_array))) {
   lower_gradient(b, tex);
   progress = true;
               /* TXF, TXS and TXL require a LOD but not everything we implement using those
   * three opcodes provides one.  Provide a default LOD of 0.
   */
   if ((nir_tex_instr_src_index(tex, nir_tex_src_lod) == -1) &&
      (tex->op == nir_texop_txf || tex->op == nir_texop_txs ||
   tex->op == nir_texop_txl || tex->op == nir_texop_query_levels)) {
   b->cursor = nir_before_instr(&tex->instr);
   nir_tex_instr_add_src(tex, nir_tex_src_lod, nir_imm_int(b, 0));
   progress = true;
               /* Only fragment and compute (in some cases) support implicit
   * derivatives.  Lower those opcodes which use implicit derivatives to
   * use an explicit LOD of 0.
   * But don't touch RECT samplers because they don't have mips.
   */
   if (options->lower_invalid_implicit_lod &&
      nir_tex_instr_has_implicit_derivative(tex) &&
   tex->sampler_dim != GLSL_SAMPLER_DIM_RECT &&
   !nir_shader_supports_implicit_lod(b->shader)) {
   lower_zero_lod(b, tex);
               if (options->lower_txs_lod && tex->op == nir_texop_txs) {
      progress |= nir_lower_txs_lod(b, tex);
               if (options->lower_txs_cube_array && tex->op == nir_texop_txs &&
      tex->sampler_dim == GLSL_SAMPLER_DIM_CUBE && tex->is_array) {
   nir_lower_txs_cube_array(b, tex);
   progress = true;
               /* has to happen after all the other lowerings as the original tg4 gets
   * replaced by 4 tg4 instructions.
   */
   if (tex->op == nir_texop_tg4 &&
      nir_tex_instr_has_explicit_tg4_offsets(tex) &&
   options->lower_tg4_offsets) {
   progress |= lower_tg4_offsets(b, tex);
               if (options->lower_to_fragment_fetch_amd && tex->op == nir_texop_txf_ms) {
      nir_lower_ms_txf_to_fragment_fetch(b, tex);
   progress = true;
               if (options->lower_to_fragment_fetch_amd && tex->op == nir_texop_samples_identical) {
      nir_lower_samples_identical_to_fragment_fetch(b, tex);
   progress = true;
               if (options->lower_lod_zero_width && tex->op == nir_texop_lod) {
      nir_lower_lod_zero_width(b, tex);
   progress = true;
                     }
      static bool
   nir_lower_tex_impl(nir_function_impl *impl,
               {
      bool progress = false;
            nir_foreach_block(block, impl) {
                  nir_metadata_preserve(impl, nir_metadata_block_index |
            }
      bool
   nir_lower_tex(nir_shader *shader, const nir_lower_tex_options *options)
   {
               /* lower_tg4_offsets injects new tg4 instructions that won't be lowered
   * if lower_tg4_broadcom_swizzle is also requested so when both are set
   * we want to run lower_tg4_offsets in a separate pass first.
   */
   if (options->lower_tg4_offsets && options->lower_tg4_broadcom_swizzle) {
      nir_lower_tex_options _options = {
         };
               nir_foreach_function_impl(impl, shader) {
                     }
