   /*
   * Copyright © 2018 Red Hat Inc.
   * Copyright © 2015 Intel Corporation
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
      #include <math.h>
      #include "nir.h"
   #include "nir_builtin_builder.h"
      nir_def *
   nir_cross3(nir_builder *b, nir_def *x, nir_def *y)
   {
      unsigned yzx[3] = { 1, 2, 0 };
            return nir_ffma(b, nir_swizzle(b, x, yzx, 3),
                  }
      nir_def *
   nir_cross4(nir_builder *b, nir_def *x, nir_def *y)
   {
               return nir_vec4(b,
                  nir_channel(b, cross, 0),
   }
      nir_def *
   nir_fast_length(nir_builder *b, nir_def *vec)
   {
         }
      nir_def *
   nir_nextafter(nir_builder *b, nir_def *x, nir_def *y)
   {
      nir_def *zero = nir_imm_intN_t(b, 0, x->bit_size);
            nir_def *condeq = nir_feq(b, x, y);
   nir_def *conddir = nir_flt(b, x, y);
            uint64_t sign_mask = 1ull << (x->bit_size - 1);
            if (nir_is_denorm_flush_to_zero(b->shader->info.float_controls_execution_mode, x->bit_size)) {
      switch (x->bit_size) {
   case 16:
      min_abs = 1 << 10;
      case 32:
      min_abs = 1 << 23;
      case 64:
      min_abs = 1ULL << 52;
               /* Flush denorm to zero to avoid returning a denorm when condeq is true. */
               /* beware of: +/-0.0 - 1 == NaN */
   nir_def *xn =
      nir_bcsel(b,
                     /* beware of -0.0 + 1 == -0x1p-149 */
   nir_def *xp = nir_bcsel(b, condzero,
                  /* nextafter can be implemented by just +/- 1 on the int value */
   nir_def *res =
               }
      nir_def *
   nir_normalize(nir_builder *b, nir_def *vec)
   {
      if (vec->num_components == 1)
            nir_def *f0 = nir_imm_floatN_t(b, 0.0, vec->bit_size);
   nir_def *f1 = nir_imm_floatN_t(b, 1.0, vec->bit_size);
            /* scale the input to increase precision */
   nir_def *maxc = nir_fmax_abs_vec_comp(b, vec);
   nir_def *svec = nir_fdiv(b, vec, maxc);
   /* for inf */
            nir_def *temp = nir_bcsel(b, nir_feq(b, maxc, finf), finfvec, svec);
               }
      nir_def *
   nir_smoothstep(nir_builder *b, nir_def *edge0, nir_def *edge1, nir_def *x)
   {
      nir_def *f2 = nir_imm_floatN_t(b, 2.0, x->bit_size);
            /* t = clamp((x - edge0) / (edge1 - edge0), 0, 1) */
   nir_def *t =
      nir_fsat(b, nir_fdiv(b, nir_fsub(b, x, edge0),
         /* result = t * t * (3 - 2 * t) */
      }
      nir_def *
   nir_upsample(nir_builder *b, nir_def *hi, nir_def *lo)
   {
      assert(lo->num_components == hi->num_components);
            nir_def *res[NIR_MAX_VEC_COMPONENTS];
   for (unsigned i = 0; i < lo->num_components; ++i) {
      nir_def *vec = nir_vec2(b, nir_channel(b, lo, i), nir_channel(b, hi, i));
                  }
      /**
   * Compute xs[0] + xs[1] + xs[2] + ... using fadd.
   */
   static nir_def *
   build_fsum(nir_builder *b, nir_def **xs, int terms)
   {
               for (int i = 1; i < terms; i++)
               }
      nir_def *
   nir_atan(nir_builder *b, nir_def *y_over_x)
   {
               nir_def *abs_y_over_x = nir_fabs(b, y_over_x);
            /*
   * range-reduction, first step:
   *
   *      / y_over_x         if |y_over_x| <= 1.0;
   * x = <
   *      \ 1.0 / y_over_x   otherwise
   */
   nir_def *x = nir_fdiv(b, nir_fmin(b, abs_y_over_x, one),
            /*
   * approximate atan by evaluating polynomial:
   *
   * x   * 0.9999793128310355 - x^3  * 0.3326756418091246 +
   * x^5 * 0.1938924977115610 - x^7  * 0.1173503194786851 +
   * x^9 * 0.0536813784310406 - x^11 * 0.0121323213173444
   */
   nir_def *x_2 = nir_fmul(b, x, x);
   nir_def *x_3 = nir_fmul(b, x_2, x);
   nir_def *x_5 = nir_fmul(b, x_3, x_2);
   nir_def *x_7 = nir_fmul(b, x_5, x_2);
   nir_def *x_9 = nir_fmul(b, x_7, x_2);
            nir_def *polynomial_terms[] = {
      nir_fmul_imm(b, x, 0.9999793128310355f),
   nir_fmul_imm(b, x_3, -0.3326756418091246f),
   nir_fmul_imm(b, x_5, 0.1938924977115610f),
   nir_fmul_imm(b, x_7, -0.1173503194786851f),
   nir_fmul_imm(b, x_9, 0.0536813784310406f),
               nir_def *tmp =
            /* range-reduction fixup */
   tmp = nir_ffma(b,
                        /* sign fixup */
            /* The fmin and fmax above will filter out NaN values.  This leads to
   * non-NaN results for NaN inputs.  Work around this by doing
   *
   *    !isnan(y_over_x) ? ... : y_over_x;
   */
   if (b->exact ||
      nir_is_float_control_signed_zero_inf_nan_preserve(b->shader->info.float_controls_execution_mode, bit_size)) {
            b->exact = true;
   nir_def *is_not_nan = nir_feq(b, y_over_x, y_over_x);
            /* The extra 1.0*y_over_x ensures that subnormal results are flushed to
   * zero.
   */
                  }
      nir_def *
   nir_atan2(nir_builder *b, nir_def *y, nir_def *x)
   {
      assert(y->bit_size == x->bit_size);
            nir_def *zero = nir_imm_floatN_t(b, 0, bit_size);
            /* If we're on the left half-plane rotate the coordinates π/2 clock-wise
   * for the y=0 discontinuity to end up aligned with the vertical
   * discontinuity of atan(s/t) along t=0.  This also makes sure that we
   * don't attempt to divide by zero along the vertical line, which may give
   * unspecified results on non-GLSL 4.1-capable hardware.
   */
   nir_def *flip = nir_fge(b, zero, x);
   nir_def *s = nir_bcsel(b, flip, nir_fabs(b, x), y);
            /* If the magnitude of the denominator exceeds some huge value, scale down
   * the arguments in order to prevent the reciprocal operation from flushing
   * its result to zero, which would cause precision problems, and for s
   * infinite would cause us to return a NaN instead of the correct finite
   * value.
   *
   * If fmin and fmax are respectively the smallest and largest positive
   * normalized floating point values representable by the implementation,
   * the constants below should be in agreement with:
   *
   *    huge <= 1 / fmin
   *    scale <= 1 / fmin / fmax (for |t| >= huge)
   *
   * In addition scale should be a negative power of two in order to avoid
   * loss of precision.  The values chosen below should work for most usual
   * floating point representations with at least the dynamic range of ATI's
   * 24-bit representation.
   */
   const double huge_val = bit_size >= 32 ? 1e18 : 16384;
   nir_def *scale = nir_bcsel(b, nir_fge_imm(b, nir_fabs(b, t), huge_val),
         nir_def *rcp_scaled_t = nir_frcp(b, nir_fmul(b, t, scale));
            /* For |x| = |y| assume tan = 1 even if infinite (i.e. pretend momentarily
   * that ∞/∞ = 1) in order to comply with the rather artificial rules
   * inherited from IEEE 754-2008, namely:
   *
   *  "atan2(±∞, −∞) is ±3π/4
   *   atan2(±∞, +∞) is ±π/4"
   *
   * Note that this is inconsistent with the rules for the neighborhood of
   * zero that are based on iterated limits:
   *
   *  "atan2(±0, −0) is ±π
   *   atan2(±0, +0) is ±0"
   *
   * but GLSL specifically allows implementations to deviate from IEEE rules
   * at (0,0), so we take that license (i.e. pretend that 0/0 = 1 here as
   * well).
   */
   nir_def *tan = nir_bcsel(b, nir_feq(b, nir_fabs(b, x), nir_fabs(b, y)),
            /* Calculate the arctangent and fix up the result if we had flipped the
   * coordinate system.
   */
   nir_def *arc =
            /* Rather convoluted calculation of the sign of the result.  When x < 0 we
   * cannot use fsign because we need to be able to distinguish between
   * negative and positive zero.  We don't use bitwise arithmetic tricks for
   * consistency with the GLSL front-end.  When x >= 0 rcp_scaled_t will
   * always be non-negative so this won't be able to distinguish between
   * negative and positive zero, but we don't care because atan2 is
   * continuous along the whole positive y = 0 half-line, so it won't affect
   * the result significantly.
   */
   return nir_bcsel(b, nir_flt(b, nir_fmin(b, y, rcp_scaled_t), zero),
      }
      nir_def *
   nir_get_texture_size(nir_builder *b, nir_tex_instr *tex)
   {
                        unsigned num_srcs = 1; /* One for the LOD */
   for (unsigned i = 0; i < tex->num_srcs; i++) {
      if (tex->src[i].src_type == nir_tex_src_texture_deref ||
      tex->src[i].src_type == nir_tex_src_sampler_deref ||
   tex->src[i].src_type == nir_tex_src_texture_offset ||
   tex->src[i].src_type == nir_tex_src_sampler_offset ||
   tex->src[i].src_type == nir_tex_src_texture_handle ||
   tex->src[i].src_type == nir_tex_src_sampler_handle)
            txs = nir_tex_instr_create(b->shader, num_srcs);
   txs->op = nir_texop_txs;
   txs->sampler_dim = tex->sampler_dim;
   txs->is_array = tex->is_array;
   txs->is_shadow = tex->is_shadow;
   txs->is_new_style_shadow = tex->is_new_style_shadow;
   txs->texture_index = tex->texture_index;
   txs->sampler_index = tex->sampler_index;
            unsigned idx = 0;
   for (unsigned i = 0; i < tex->num_srcs; i++) {
      if (tex->src[i].src_type == nir_tex_src_texture_deref ||
      tex->src[i].src_type == nir_tex_src_sampler_deref ||
   tex->src[i].src_type == nir_tex_src_texture_offset ||
   tex->src[i].src_type == nir_tex_src_sampler_offset ||
   tex->src[i].src_type == nir_tex_src_texture_handle ||
   tex->src[i].src_type == nir_tex_src_sampler_handle) {
   txs->src[idx].src = nir_src_for_ssa(tex->src[i].src.ssa);
   txs->src[idx].src_type = tex->src[i].src_type;
         }
   /* Add in an LOD because some back-ends require it */
            nir_def_init(&txs->instr, &txs->def, nir_tex_instr_dest_size(txs), 32);
               }
      nir_def *
   nir_get_texture_lod(nir_builder *b, nir_tex_instr *tex)
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
   tql->coord_components = tex->coord_components;
   tql->sampler_dim = tex->sampler_dim;
   tql->is_array = tex->is_array;
   tql->is_shadow = tex->is_shadow;
   tql->is_new_style_shadow = tex->is_new_style_shadow;
   tql->texture_index = tex->texture_index;
   tql->sampler_index = tex->sampler_index;
            unsigned idx = 0;
   for (unsigned i = 0; i < tex->num_srcs; i++) {
      if (tex->src[i].src_type == nir_tex_src_coord ||
      tex->src[i].src_type == nir_tex_src_texture_deref ||
   tex->src[i].src_type == nir_tex_src_sampler_deref ||
   tex->src[i].src_type == nir_tex_src_texture_offset ||
   tex->src[i].src_type == nir_tex_src_sampler_offset ||
   tex->src[i].src_type == nir_tex_src_texture_handle ||
   tex->src[i].src_type == nir_tex_src_sampler_handle) {
   tql->src[idx].src = nir_src_for_ssa(tex->src[i].src.ssa);
   tql->src[idx].src_type = tex->src[i].src_type;
                  nir_def_init(&tql->instr, &tql->def, 2, 32);
            /* The LOD is the y component of the result */
      }
