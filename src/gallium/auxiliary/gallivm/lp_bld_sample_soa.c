   /**************************************************************************
   *
   * Copyright 2009 VMware, Inc.
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
      /**
   * @file
   * Texture sampling -- SoA.
   *
   * @author Jose Fonseca <jfonseca@vmware.com>
   * @author Brian Paul <brianp@vmware.com>
   */
      #include "pipe/p_defines.h"
   #include "pipe/p_state.h"
   #include "pipe/p_shader_tokens.h"
   #include "util/compiler.h"
   #include "util/u_debug.h"
   #include "util/u_dump.h"
   #include "util/u_memory.h"
   #include "util/u_math.h"
   #include "util/format/u_format.h"
   #include "util/u_cpu_detect.h"
   #include "util/format_rgb9e5.h"
   #include "lp_bld_debug.h"
   #include "lp_bld_type.h"
   #include "lp_bld_const.h"
   #include "lp_bld_conv.h"
   #include "lp_bld_arit.h"
   #include "lp_bld_bitarit.h"
   #include "lp_bld_logic.h"
   #include "lp_bld_printf.h"
   #include "lp_bld_swizzle.h"
   #include "lp_bld_flow.h"
   #include "lp_bld_gather.h"
   #include "lp_bld_format.h"
   #include "lp_bld_sample.h"
   #include "lp_bld_sample_aos.h"
   #include "lp_bld_struct.h"
   #include "lp_bld_quad.h"
   #include "lp_bld_pack.h"
   #include "lp_bld_intr.h"
   #include "lp_bld_misc.h"
   #include "lp_bld_jit_types.h"
         /**
   * Generate code to fetch a texel from a texture at int coords (x, y, z).
   * The computation depends on whether the texture is 1D, 2D or 3D.
   * The result, texel, will be float vectors:
   *   texel[0] = red values
   *   texel[1] = green values
   *   texel[2] = blue values
   *   texel[3] = alpha values
   */
   static void
   lp_build_sample_texel_soa(struct lp_build_sample_context *bld,
                           LLVMValueRef width,
   LLVMValueRef height,
   LLVMValueRef depth,
   LLVMValueRef x,
   LLVMValueRef y,
   LLVMValueRef z,
   LLVMValueRef y_stride,
   {
      const struct lp_static_sampler_state *static_state = bld->static_sampler_state;
   const unsigned dims = bld->dims;
   struct lp_build_context *int_coord_bld = &bld->int_coord_bld;
   LLVMBuilderRef builder = bld->gallivm->builder;
   LLVMValueRef offset;
   LLVMValueRef i, j;
            /* use_border = x < 0 || x >= width || y < 0 || y >= height */
   if (lp_sampler_wrap_mode_uses_border_color(static_state->wrap_s,
                  LLVMValueRef b1, b2;
   b1 = lp_build_cmp(int_coord_bld, PIPE_FUNC_LESS, x, int_coord_bld->zero);
   b2 = lp_build_cmp(int_coord_bld, PIPE_FUNC_GEQUAL, x, width);
               if (dims >= 2 &&
      lp_sampler_wrap_mode_uses_border_color(static_state->wrap_t,
               LLVMValueRef b1, b2;
   b1 = lp_build_cmp(int_coord_bld, PIPE_FUNC_LESS, y, int_coord_bld->zero);
   b2 = lp_build_cmp(int_coord_bld, PIPE_FUNC_GEQUAL, y, height);
   if (use_border) {
      use_border = LLVMBuildOr(builder, use_border, b1, "ub_or_b1");
      } else {
                     if (dims == 3 &&
      lp_sampler_wrap_mode_uses_border_color(static_state->wrap_r,
               LLVMValueRef b1, b2;
   b1 = lp_build_cmp(int_coord_bld, PIPE_FUNC_LESS, z, int_coord_bld->zero);
   b2 = lp_build_cmp(int_coord_bld, PIPE_FUNC_GEQUAL, z, depth);
   if (use_border) {
      use_border = LLVMBuildOr(builder, use_border, b1, "ub_or_b1");
      } else {
                     /* convert x,y,z coords to linear offset from start of texture, in bytes */
   lp_build_sample_offset(&bld->int_coord_bld,
                     if (mipoffsets) {
                  if (use_border) {
      /* If we can sample the border color, it means that texcoords may
   * lie outside the bounds of the texture image.  We need to do
   * something to prevent reading out of bounds and causing a segfault.
   *
   * Simply AND the texture coords with !use_border.  This will cause
   * coords which are out of bounds to become zero.  Zero's guaranteed
   * to be inside the texture image.
   */
               lp_build_fetch_rgba_soa(bld->gallivm,
                           bld->format_desc,
            /*
   * Note: if we find an app which frequently samples the texture border
   * we might want to implement a true conditional here to avoid sampling
   * the texture whenever possible (since that's quite a bit of code).
   * Ex:
   *   if (use_border) {
   *      texel = border_color;
   *   } else {
   *      texel = sample_texture(coord);
   *   }
   * As it is now, we always sample the texture, then selectively replace
   * the texel color results with the border color.
            if (use_border) {
      /* select texel color or border color depending on use_border. */
   const struct util_format_description *format_desc = bld->format_desc;
   struct lp_type border_type = bld->texel_type;
   border_type.length = 4;
   /*
   * Only replace channels which are actually present. The others should
   * get optimized away eventually by sampler_view swizzle anyway but it's
   * easier too.
   */
   for (unsigned chan = 0; chan < 4; chan++) {
      unsigned chan_s;
   /* reverse-map channel... */
   if (util_format_has_stencil(format_desc)) {
      if (chan == 0)
         else
      } else {
      for (chan_s = 0; chan_s < 4; chan_s++) {
      if (chan_s == format_desc->swizzle[chan]) {
               }
   if (chan_s <= 3) {
      /* use the already clamped color */
                  border_chan = lp_build_extract_broadcast(bld->gallivm,
                           texel_out[chan] = lp_build_select(&bld->texel_bld, use_border,
               }
         /**
   * Helper to compute the mirror function for the PIPE_WRAP_MIRROR_REPEAT mode.
   * (Note that with pot sizes could do this much more easily post-scale
   * with some bit arithmetic.)
   */
   static LLVMValueRef
   lp_build_coord_mirror(struct lp_build_sample_context *bld,
         {
      struct lp_build_context *coord_bld = &bld->coord_bld;
   LLVMValueRef fract;
            /*
   * We can just use 2*(x - round(0.5*x)) to do all the mirroring,
   * it all works out. (The result is in range [-1, 1.0], negative if
   * the coord is in the "odd" section, otherwise positive.)
            coord = lp_build_mul(coord_bld, coord, half);
   fract = lp_build_round(coord_bld, coord);
   fract = lp_build_sub(coord_bld, coord, fract);
            if (posOnly) {
      /*
   * Theoretically it's not quite 100% accurate because the spec says
   * that ultimately a scaled coord of -x.0 should map to int coord
   * -x + 1 with mirroring, not -x (this does not matter for bilinear
   * filtering).
   */
   coord = lp_build_abs(coord_bld, coord);
   /* kill off NaNs */
   /* XXX: not safe without arch rounding, fract can be anything. */
   coord = lp_build_max_ext(coord_bld, coord, coord_bld->zero,
                  }
         /**
   * Helper to compute the first coord and the weight for
   * linear wrap repeat npot textures
   */
   void
   lp_build_coord_repeat_npot_linear(struct lp_build_sample_context *bld,
                                 {
      struct lp_build_context *coord_bld = &bld->coord_bld;
   struct lp_build_context *int_coord_bld = &bld->int_coord_bld;
   LLVMValueRef half = lp_build_const_vec(bld->gallivm, coord_bld->type, 0.5);
   LLVMValueRef length_minus_one = lp_build_sub(int_coord_bld, length_i,
         LLVMValueRef mask;
   /* wrap with normalized floats is just fract */
   coord_f = lp_build_fract(coord_bld, coord_f);
   /* mul by size and subtract 0.5 */
   coord_f = lp_build_mul(coord_bld, coord_f, length_f);
   coord_f = lp_build_sub(coord_bld, coord_f, half);
   /*
   * we avoided the 0.5/length division before the repeat wrap,
   * now need to fix up edge cases with selects
   */
   /*
   * Note we do a float (unordered) compare so we can eliminate NaNs.
   * (Otherwise would need fract_safe above).
   */
   mask = lp_build_compare(coord_bld->gallivm, coord_bld->type,
            /* convert to int, compute lerp weight */
   lp_build_ifloor_fract(coord_bld, coord_f, coord0_i, weight_f);
      }
         /**
   * Build LLVM code for texture wrap mode for linear filtering.
   * \param x0_out  returns first integer texcoord
   * \param x1_out  returns second integer texcoord
   * \param weight_out  returns linear interpolation weight
   */
   static void
   lp_build_sample_wrap_linear(struct lp_build_sample_context *bld,
                              bool is_gather,
   LLVMValueRef coord,
   LLVMValueRef length,
   LLVMValueRef length_f,
   LLVMValueRef offset,
      {
      struct lp_build_context *coord_bld = &bld->coord_bld;
   struct lp_build_context *int_coord_bld = &bld->int_coord_bld;
   LLVMBuilderRef builder = bld->gallivm->builder;
   LLVMValueRef half = lp_build_const_vec(bld->gallivm, coord_bld->type, 0.5);
   LLVMValueRef length_minus_one = lp_build_sub(int_coord_bld, length, int_coord_bld->one);
            switch (wrap_mode) {
   case PIPE_TEX_WRAP_REPEAT:
      if (is_pot) {
      /* mul by size and subtract 0.5 */
   coord = lp_build_mul(coord_bld, coord, length_f);
   coord = lp_build_sub(coord_bld, coord, half);
   if (offset) {
      offset = lp_build_int_to_float(coord_bld, offset);
      }
   /* convert to int, compute lerp weight */
   lp_build_ifloor_fract(coord_bld, coord, &coord0, &weight);
   coord1 = lp_build_add(int_coord_bld, coord0, int_coord_bld->one);
   /* repeat wrap */
   coord0 = LLVMBuildAnd(builder, coord0, length_minus_one, "");
      } else {
      LLVMValueRef mask;
   if (offset) {
      offset = lp_build_int_to_float(coord_bld, offset);
   offset = lp_build_div(coord_bld, offset, length_f);
      }
   lp_build_coord_repeat_npot_linear(bld, coord,
               mask = lp_build_compare(int_coord_bld->gallivm, int_coord_bld->type,
         coord1 = LLVMBuildAnd(builder,
            }
         case PIPE_TEX_WRAP_CLAMP:
      if (bld->static_sampler_state->normalized_coords) {
      /* scale coord to length */
      }
   if (offset) {
      offset = lp_build_int_to_float(coord_bld, offset);
               /*
   * clamp to [0, length]
   *
   * Unlike some other wrap modes, this should be correct for gather
   * too. GL_CLAMP explicitly does this clamp on the coord prior to
   * actual wrapping (which is per sample).
   */
                     /* convert to int, compute lerp weight */
   lp_build_ifloor_fract(coord_bld, coord, &coord0, &weight);
   coord1 = lp_build_add(int_coord_bld, coord0, int_coord_bld->one);
         case PIPE_TEX_WRAP_CLAMP_TO_EDGE:
      {
                     if (bld->static_sampler_state->normalized_coords) {
      /* mul by tex size */
      }
   if (offset) {
      offset = lp_build_int_to_float(coord_bld, offset);
               /* clamp to length max */
   coord = lp_build_min_ext(coord_bld, coord, length_f,
         if (!is_gather) {
      /* subtract 0.5 */
   coord = lp_build_sub(coord_bld, coord, half);
   /* clamp to [0, length - 0.5] */
   coord = lp_build_max(coord_bld, coord, coord_bld->zero);
   /* convert to int, compute lerp weight */
   lp_build_ifloor_fract(&abs_coord_bld, coord, &coord0, &weight);
      } else {
      /*
   * The non-gather path will end up with coords 0, 1 if coord was
   * smaller than 0.5 (with corresponding weight 0.0 so it doesn't
   * really matter what the second coord is). But for gather, we
   * really need to end up with coords 0, 0.
   */
   coord = lp_build_max(coord_bld, coord, coord_bld->zero);
   coord0 = lp_build_sub(coord_bld, coord, half);
   coord1 = lp_build_add(coord_bld, coord, half);
   /* Values range ([-0.5, length_f - 0.5], [0.5, length_f + 0.5] */
   coord0 = lp_build_itrunc(coord_bld, coord0);
   coord1 = lp_build_itrunc(coord_bld, coord1);
      }
   /* coord1 = min(coord1, length-1) */
   coord1 = lp_build_min(int_coord_bld, coord1, length_minus_one);
            case PIPE_TEX_WRAP_CLAMP_TO_BORDER:
      if (bld->static_sampler_state->normalized_coords) {
      /* scale coord to length */
      }
   if (offset) {
      offset = lp_build_int_to_float(coord_bld, offset);
      }
   /*
   * We don't need any clamp. Technically, for very large (pos or neg)
   * (or infinite) values, clamp against [-length, length] would be
   * correct, but we don't need to guarantee any specific
   * result for such coords (the ifloor will be undefined, but for modes
   * requiring border all resulting coords are safe).
   */
   coord = lp_build_sub(coord_bld, coord, half);
   /* convert to int, compute lerp weight */
   lp_build_ifloor_fract(coord_bld, coord, &coord0, &weight);
   coord1 = lp_build_add(int_coord_bld, coord0, int_coord_bld->one);
         case PIPE_TEX_WRAP_MIRROR_REPEAT:
      if (offset) {
      offset = lp_build_int_to_float(coord_bld, offset);
   offset = lp_build_div(coord_bld, offset, length_f);
      }
   if (!is_gather) {
                     /* scale coord to length */
                  /* convert to int, compute lerp weight */
                  /* coord0 = max(coord0, 0) */
   coord0 = lp_build_max(int_coord_bld, coord0, int_coord_bld->zero);
   /* coord1 = min(coord1, length-1) */
      } else {
      /*
   * This is pretty reasonable in the end,  all what the tests care
   * about is nasty edge cases (scaled coords x.5, so the individual
   * coords are actually integers, which is REALLY tricky to get right
   * due to this working differently both for negative numbers as well
   * as for even/odd cases). But with enough magic it's not too complex
   * after all.
   * Maybe should try a bit arithmetic one though for POT textures...
   */
   LLVMValueRef isNeg;
   /*
   * Wrapping just once still works, even though it means we can
   * get "wrong" sign due to performing mirror in the middle of the
   * two coords (because this can only happen very near the odd/even
   * edges, so both coords will actually end up as 0 or length - 1
   * in the end).
   * For GL4 gather with per-sample offsets we'd need to the mirroring
   * per coord too.
   */
                  /*
   * NaNs should be safe here, we'll do away with them with
   * the ones' complement plus min.
   */
   coord0 = lp_build_sub(coord_bld, coord, half);
   coord0 = lp_build_ifloor(coord_bld, coord0);
   coord1 = lp_build_add(int_coord_bld, coord0, int_coord_bld->one);
   /* ones complement for neg numbers (mirror(negX) = X - 1)  */
   isNeg = lp_build_cmp(int_coord_bld, PIPE_FUNC_LESS,
         coord0 = lp_build_xor(int_coord_bld, coord0, isNeg);
   isNeg = lp_build_cmp(int_coord_bld, PIPE_FUNC_LESS,
         coord1 = lp_build_xor(int_coord_bld, coord1, isNeg);
                     }
         case PIPE_TEX_WRAP_MIRROR_CLAMP:
      if (bld->static_sampler_state->normalized_coords) {
      /* scale coord to length */
      }
   if (offset) {
      offset = lp_build_int_to_float(coord_bld, offset);
      }
   /*
   * XXX: probably not correct for gather, albeit I'm not
   * entirely sure as it's poorly specified. The wrapping looks
   * correct according to the spec which is against gl 1.2.1,
   * however negative values will be swapped - gl re-specified
   * wrapping with newer versions (no more pre-clamp except with
   * GL_CLAMP).
   */
            /* clamp to [0, length] */
   coord = lp_build_min_ext(coord_bld, coord, length_f,
                     /* convert to int, compute lerp weight */
   lp_build_ifloor_fract(coord_bld, coord, &coord0, &weight);
   coord1 = lp_build_add(int_coord_bld, coord0, int_coord_bld->one);
         case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_EDGE:
      {
                     if (bld->static_sampler_state->normalized_coords) {
      /* scale coord to length */
      }
   if (offset) {
      offset = lp_build_int_to_float(coord_bld, offset);
      }
                     /* clamp to length max */
   coord = lp_build_min_ext(coord_bld, coord, length_f,
         /* subtract 0.5 */
   coord = lp_build_sub(coord_bld, coord, half);
                  /* convert to int, compute lerp weight */
   lp_build_ifloor_fract(&abs_coord_bld, coord, &coord0, &weight);
   coord1 = lp_build_add(int_coord_bld, coord0, int_coord_bld->one);
   /* coord1 = min(coord1, length-1) */
      } else {
      /*
   * The non-gather path will swap coord0/1 if coord was negative,
   * which is ok for filtering since the filter weight matches
   * accordingly. Also, if coord is close to zero, coord0/1 will
   * be 0 and 1, instead of 0 and 0 (again ok due to filter
   * weight being 0.0). Both issues need to be fixed for gather.
                  /*
   * Actually wanted to cheat here and use:
   * coord1 = lp_build_iround(coord_bld, coord);
   * but it's not good enough for some tests (even piglit
   * textureGather is set up in a way so the coords area always
   * .5, that is right at the crossover points).
   * So do ordinary sub/floor, then do ones' complement
   * for negative numbers.
   * (Note can't just do sub|add/abs/itrunc per coord neither -
   * because the spec demands that mirror(3.0) = 3 but
   * mirror(-3.0) = 2.)
   */
   coord = lp_build_sub(coord_bld, coord, half);
   coord0 = lp_build_ifloor(coord_bld, coord);
   coord1 = lp_build_add(int_coord_bld, coord0, int_coord_bld->one);
   isNeg = lp_build_cmp(int_coord_bld, PIPE_FUNC_LESS, coord0,
                        isNeg = lp_build_cmp(int_coord_bld, PIPE_FUNC_LESS, coord1,
                              }
         case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_BORDER:
      {
      if (bld->static_sampler_state->normalized_coords) {
      /* scale coord to length */
      }
   if (offset) {
      offset = lp_build_int_to_float(coord_bld, offset);
      }
   /*
   * XXX: probably not correct for gather due to swapped
   * order if coord is negative (same rationale as for
   * MIRROR_CLAMP).
                  /*
   * We don't need any clamp. Technically, for very large
   * (or infinite) values, clamp against length would be
   * correct, but we don't need to guarantee any specific
   * result for such coords (the ifloor will be undefined, but
   * for modes requiring border all resulting coords are safe).
                  /* convert to int, compute lerp weight */
   lp_build_ifloor_fract(coord_bld, coord, &coord0, &weight);
      }
         default:
      assert(0);
   coord0 = NULL;
   coord1 = NULL;
               *x0_out = coord0;
   *x1_out = coord1;
      }
         /**
   * Build LLVM code for texture wrap mode for nearest filtering.
   * \param coord  the incoming texcoord (nominally in [0,1])
   * \param length  the texture size along one dimension, as int vector
   * \param length_f  the texture size along one dimension, as float vector
   * \param offset  texel offset along one dimension (as int vector)
   * \param is_pot  if TRUE, length is a power of two
   * \param wrap_mode  one of PIPE_TEX_WRAP_x
   */
   static LLVMValueRef
   lp_build_sample_wrap_nearest(struct lp_build_sample_context *bld,
                              LLVMValueRef coord,
      {
      struct lp_build_context *coord_bld = &bld->coord_bld;
   struct lp_build_context *int_coord_bld = &bld->int_coord_bld;
   LLVMBuilderRef builder = bld->gallivm->builder;
   LLVMValueRef length_minus_one = lp_build_sub(int_coord_bld, length, int_coord_bld->one);
            switch (wrap_mode) {
   case PIPE_TEX_WRAP_REPEAT:
      if (is_pot) {
      coord = lp_build_mul(coord_bld, coord, length_f);
   icoord = lp_build_ifloor(coord_bld, coord);
   if (offset) {
         }
      } else {
      if (offset) {
      offset = lp_build_int_to_float(coord_bld, offset);
   offset = lp_build_div(coord_bld, offset, length_f);
      }
   /* take fraction, unnormalize */
   coord = lp_build_fract_safe(coord_bld, coord);
   coord = lp_build_mul(coord_bld, coord, length_f);
      }
         case PIPE_TEX_WRAP_CLAMP:
   case PIPE_TEX_WRAP_CLAMP_TO_EDGE:
      if (bld->static_sampler_state->normalized_coords) {
      /* scale coord to length */
               if (offset) {
      offset = lp_build_int_to_float(coord_bld, offset);
      }
   /* floor */
   /* use itrunc instead since we clamp to 0 anyway */
            /* clamp to [0, length - 1]. */
   icoord = lp_build_clamp(int_coord_bld, icoord, int_coord_bld->zero,
               case PIPE_TEX_WRAP_CLAMP_TO_BORDER:
      if (bld->static_sampler_state->normalized_coords) {
      /* scale coord to length */
      }
   /* no clamp necessary, border masking will handle this */
   icoord = lp_build_ifloor(coord_bld, coord);
   if (offset) {
         }
         case PIPE_TEX_WRAP_MIRROR_REPEAT:
      if (offset) {
      offset = lp_build_int_to_float(coord_bld, offset);
   offset = lp_build_div(coord_bld, offset, length_f);
      }
   /* compute mirror function */
            /* scale coord to length */
   assert(bld->static_sampler_state->normalized_coords);
            /* itrunc == ifloor here */
            /* clamp to [0, length - 1] */
   icoord = lp_build_min(int_coord_bld, icoord, length_minus_one);
         case PIPE_TEX_WRAP_MIRROR_CLAMP:
   case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_EDGE:
      if (bld->static_sampler_state->normalized_coords) {
      /* scale coord to length */
      }
   if (offset) {
      offset = lp_build_int_to_float(coord_bld, offset);
      }
            /* itrunc == ifloor here */
   icoord = lp_build_itrunc(coord_bld, coord);
   /*
   * Use unsigned min due to possible undef values (NaNs, overflow)
   */
   {
      struct lp_build_context abs_coord_bld = *int_coord_bld;
   abs_coord_bld.type.sign = false;
   /* clamp to [0, length - 1] */
      }
         case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_BORDER:
      if (bld->static_sampler_state->normalized_coords) {
      /* scale coord to length */
      }
   if (offset) {
      offset = lp_build_int_to_float(coord_bld, offset);
      }
            /* itrunc == ifloor here */
   icoord = lp_build_itrunc(coord_bld, coord);
         default:
      assert(0);
                  }
         /**
   * Do shadow test/comparison.
   * \param p shadow ref value
   * \param texel  the texel to compare against
   */
   static LLVMValueRef
   lp_build_sample_comparefunc(struct lp_build_sample_context *bld,
               {
      struct lp_build_context *texel_bld = &bld->texel_bld;
            if (0) {
      //lp_build_print_value(bld->gallivm, "shadow cmp coord", p);
               /* result = (p FUNC texel) ? 1 : 0 */
   /*
   * honor d3d10 floating point rules here, which state that comparisons
   * are ordered except NOT_EQUAL which is unordered.
   */
   if (bld->static_sampler_state->compare_func != PIPE_FUNC_NOTEQUAL) {
      res = lp_build_cmp_ordered(texel_bld,
            } else {
      res = lp_build_cmp(texel_bld, bld->static_sampler_state->compare_func,
      }
      }
         /**
   * Generate code to sample a mipmap level with nearest filtering.
   * If sampling a cube texture, r = cube face in [0,5].
   */
   static void
   lp_build_sample_image_nearest(struct lp_build_sample_context *bld,
                                 LLVMValueRef size,
   LLVMValueRef row_stride_vec,
   LLVMValueRef img_stride_vec,
   {
      const unsigned dims = bld->dims;
   LLVMValueRef width_vec;
   LLVMValueRef height_vec;
   LLVMValueRef depth_vec;
   LLVMValueRef flt_size;
   LLVMValueRef flt_width_vec;
   LLVMValueRef flt_height_vec;
   LLVMValueRef flt_depth_vec;
            lp_build_extract_image_sizes(bld,
                                       lp_build_extract_image_sizes(bld,
                              /*
   * Compute integer texcoords.
   */
   x = lp_build_sample_wrap_nearest(bld, coords[0], width_vec,
                              if (dims >= 2) {
      y = lp_build_sample_wrap_nearest(bld, coords[1], height_vec,
                              if (dims == 3) {
      z = lp_build_sample_wrap_nearest(bld, coords[2], depth_vec,
                           }
   if (has_layer_coord(bld->static_texture_state->target)) {
      if (bld->static_texture_state->target == PIPE_TEXTURE_CUBE_ARRAY) {
      /* add cube layer to face */
      } else {
         }
               /*
   * Get texture colors.
   */
   lp_build_sample_texel_soa(bld,
                              if (bld->static_sampler_state->compare_mode != PIPE_TEX_COMPARE_NONE) {
      LLVMValueRef cmpval;
   cmpval = lp_build_sample_comparefunc(bld, coords[4], colors_out[0]);
   /* this is really just a AND 1.0, cmpval but llvm is clever enough */
   colors_out[0] = lp_build_select(&bld->texel_bld, cmpval,
                  }
         /**
   * Like a lerp, but inputs are 0/~0 masks, so can simplify slightly.
   */
   static LLVMValueRef
   lp_build_masklerp(struct lp_build_context *bld,
                     {
      struct gallivm_state *gallivm = bld->gallivm;
   LLVMBuilderRef builder = gallivm->builder;
            weight2 = lp_build_sub(bld, bld->one, weight);
   weight = LLVMBuildBitCast(builder, weight,
         weight2 = LLVMBuildBitCast(builder, weight2,
         weight = LLVMBuildAnd(builder, weight, mask1, "");
   weight2 = LLVMBuildAnd(builder, weight2, mask0, "");
   weight = LLVMBuildBitCast(builder, weight, bld->vec_type, "");
   weight2 = LLVMBuildBitCast(builder, weight2, bld->vec_type, "");
      }
      /**
   * Like a 2d lerp, but inputs are 0/~0 masks, so can simplify slightly.
   */
   static LLVMValueRef
   lp_build_masklerp2d(struct lp_build_context *bld,
                     LLVMValueRef weight0,
   LLVMValueRef weight1,
   LLVMValueRef mask00,
   {
      LLVMValueRef val0 = lp_build_masklerp(bld, weight0, mask00, mask01);
   LLVMValueRef val1 = lp_build_masklerp(bld, weight0, mask10, mask11);
      }
      /*
   * this is a bit excessive code for something OpenGL just recommends
   * but does not require.
   */
   #define ACCURATE_CUBE_CORNERS 1
      /**
   * Generate code to sample a mipmap level with linear filtering.
   * If sampling a cube texture, r = cube face in [0,5].
   * If linear_mask is present, only pixels having their mask set
   * will receive linear filtering, the rest will use nearest.
   */
   static void
   lp_build_sample_image_linear(struct lp_build_sample_context *bld,
                              bool is_gather,
   LLVMValueRef size,
   LLVMValueRef linear_mask,
   LLVMValueRef row_stride_vec,
   LLVMValueRef img_stride_vec,
      {
      LLVMBuilderRef builder = bld->gallivm->builder;
   struct lp_build_context *ivec_bld = &bld->int_coord_bld;
   struct lp_build_context *coord_bld = &bld->coord_bld;
   struct lp_build_context *texel_bld = &bld->texel_bld;
   const unsigned dims = bld->dims;
   LLVMValueRef width_vec;
   LLVMValueRef height_vec;
   LLVMValueRef depth_vec;
   LLVMValueRef flt_size;
   LLVMValueRef flt_width_vec;
   LLVMValueRef flt_height_vec;
   LLVMValueRef flt_depth_vec;
   LLVMValueRef fall_off[4] = { 0 }, have_corners = NULL;
   LLVMValueRef z1 = NULL;
   LLVMValueRef z00 = NULL, z01 = NULL, z10 = NULL, z11 = NULL;
   LLVMValueRef x00 = NULL, x01 = NULL, x10 = NULL, x11 = NULL;
   LLVMValueRef y00 = NULL, y01 = NULL, y10 = NULL, y11 = NULL;
   LLVMValueRef s_fpart, t_fpart = NULL, r_fpart = NULL;
   LLVMValueRef xs[4], ys[4], zs[4];
   LLVMValueRef neighbors[2][2][4];
   bool seamless_cube_filter, accurate_cube_corners;
            if (is_gather) {
      switch (bld->gather_comp) {
   case 0: chan_swiz = bld->static_texture_state->swizzle_r; break;
   case 1: chan_swiz = bld->static_texture_state->swizzle_g; break;
   case 2: chan_swiz = bld->static_texture_state->swizzle_b; break;
   case 3: chan_swiz = bld->static_texture_state->swizzle_a; break;
   default:
                     seamless_cube_filter = (bld->static_texture_state->target == PIPE_TEXTURE_CUBE ||
                  /*
   * Disable accurate cube corners for integer textures, which should only
   * get here in the gather path.
   */
   accurate_cube_corners = ACCURATE_CUBE_CORNERS && seamless_cube_filter &&
            lp_build_extract_image_sizes(bld,
                                       lp_build_extract_image_sizes(bld,
                                       /*
   * Compute integer texcoords.
            if (!seamless_cube_filter) {
      lp_build_sample_wrap_linear(bld, is_gather, coords[0], width_vec,
                           lp_build_name(x00, "tex.x0.wrapped");
   lp_build_name(x01, "tex.x1.wrapped");
   x10 = x00;
            if (dims >= 2) {
      lp_build_sample_wrap_linear(bld, is_gather, coords[1], height_vec,
                           lp_build_name(y00, "tex.y0.wrapped");
   lp_build_name(y10, "tex.y1.wrapped");
                  if (dims == 3) {
      lp_build_sample_wrap_linear(bld, is_gather, coords[2], depth_vec,
                           z01 = z10 = z11 = z00;
   lp_build_name(z00, "tex.z0.wrapped");
         }
   if (has_layer_coord(bld->static_texture_state->target)) {
      if (bld->static_texture_state->target == PIPE_TEXTURE_CUBE_ARRAY) {
      /* add cube layer to face */
   z00 = z01 = z10 = z11 = z1 =
      } else {
         }
   lp_build_name(z00, "tex.z0.layer");
         } else {
      struct lp_build_if_state edge_if;
   LLVMValueRef new_faces[4], new_xcoords[4][2], new_ycoords[4][2];
   LLVMValueRef coord0, coord1, have_edge, have_corner;
   LLVMValueRef fall_off_ym_notxm, fall_off_ym_notxp, fall_off_x, fall_off_y;
   LLVMValueRef fall_off_yp_notxm, fall_off_yp_notxp;
   LLVMValueRef x0, x1, y0, y1, y0_clamped, y1_clamped;
   LLVMValueRef face = coords[2];
   LLVMValueRef half = lp_build_const_vec(bld->gallivm, coord_bld->type, 0.5f);
   LLVMValueRef length_minus_one = lp_build_sub(ivec_bld, width_vec, ivec_bld->one);
   /* XXX drop height calcs. Could (should) do this without seamless filtering too */
   height_vec = width_vec;
            /* XXX the overflow logic is actually sort of duplicated with trilinear,
   * since an overflow in one mip should also have a corresponding overflow
   * in another.
   */
   /* should always have normalized coords, and offsets are undefined */
   assert(bld->static_sampler_state->normalized_coords);
   /*
   * The coords should all be between [0,1] however we can have NaNs,
   * which will wreak havoc. In particular the y1_clamped value below
   * can be -INT_MAX (on x86) and be propagated right through (probably
   * other values might be bogus in the end too).
   * So kill off the NaNs here.
   */
   coord0 = lp_build_max_ext(coord_bld, coords[0], coord_bld->zero,
         coord0 = lp_build_mul(coord_bld, coord0, flt_width_vec);
   /* instead of clamp, build mask if overflowed */
   coord0 = lp_build_sub(coord_bld, coord0, half);
   /* convert to int, compute lerp weight */
   /* not ideal with AVX (and no AVX2) */
   lp_build_ifloor_fract(coord_bld, coord0, &x0, &s_fpart);
   x1 = lp_build_add(ivec_bld, x0, ivec_bld->one);
   coord1 = lp_build_max_ext(coord_bld, coords[1], coord_bld->zero,
         coord1 = lp_build_mul(coord_bld, coord1, flt_height_vec);
   coord1 = lp_build_sub(coord_bld, coord1, half);
   lp_build_ifloor_fract(coord_bld, coord1, &y0, &t_fpart);
            fall_off[0] = lp_build_cmp(ivec_bld, PIPE_FUNC_LESS, x0, ivec_bld->zero);
   fall_off[1] = lp_build_cmp(ivec_bld, PIPE_FUNC_GREATER, x1, length_minus_one);
   fall_off[2] = lp_build_cmp(ivec_bld, PIPE_FUNC_LESS, y0, ivec_bld->zero);
            fall_off_x = lp_build_or(ivec_bld, fall_off[0], fall_off[1]);
   fall_off_y = lp_build_or(ivec_bld, fall_off[2], fall_off[3]);
   have_edge = lp_build_or(ivec_bld, fall_off_x, fall_off_y);
            /* needed for accurate corner filtering branch later, rely on 0 init */
            for (unsigned texel_index = 0; texel_index < 4; texel_index++) {
      xs[texel_index] = lp_build_alloca(bld->gallivm, ivec_bld->vec_type, "xs");
   ys[texel_index] = lp_build_alloca(bld->gallivm, ivec_bld->vec_type, "ys");
                        have_corner = lp_build_and(ivec_bld, fall_off_x, fall_off_y);
   have_corner = lp_build_any_true_range(ivec_bld, ivec_bld->type.length, have_corner);
            /*
   * Need to feed clamped values here for cheap corner handling,
   * but only for y coord (as when falling off both edges we only
   * fall off the x one) - this should be sufficient.
   */
   y0_clamped = lp_build_max(ivec_bld, y0, ivec_bld->zero);
            /*
   * Get all possible new coords.
   */
   lp_build_cube_new_coords(ivec_bld, face,
                        /* handle fall off x-, x+ direction */
   /* determine new coords, face (not both fall_off vars can be true at same time) */
   x00 = lp_build_select(ivec_bld, fall_off[0], new_xcoords[0][0], x0);
   y00 = lp_build_select(ivec_bld, fall_off[0], new_ycoords[0][0], y0_clamped);
   x10 = lp_build_select(ivec_bld, fall_off[0], new_xcoords[0][1], x0);
   y10 = lp_build_select(ivec_bld, fall_off[0], new_ycoords[0][1], y1_clamped);
   x01 = lp_build_select(ivec_bld, fall_off[1], new_xcoords[1][0], x1);
   y01 = lp_build_select(ivec_bld, fall_off[1], new_ycoords[1][0], y0_clamped);
   x11 = lp_build_select(ivec_bld, fall_off[1], new_xcoords[1][1], x1);
            z00 = z10 = lp_build_select(ivec_bld, fall_off[0], new_faces[0], face);
            /* handle fall off y-, y+ direction */
   /*
   * Cheap corner logic: just hack up things so a texel doesn't fall
   * off both sides (which means filter weights will be wrong but we'll only
   * use valid texels in the filter).
   * This means however (y) coords must additionally be clamped (see above).
   * This corner handling should be fully OpenGL (but not d3d10) compliant.
   */
   fall_off_ym_notxm = lp_build_andnot(ivec_bld, fall_off[2], fall_off[0]);
   fall_off_ym_notxp = lp_build_andnot(ivec_bld, fall_off[2], fall_off[1]);
   fall_off_yp_notxm = lp_build_andnot(ivec_bld, fall_off[3], fall_off[0]);
            x00 = lp_build_select(ivec_bld, fall_off_ym_notxm, new_xcoords[2][0], x00);
   y00 = lp_build_select(ivec_bld, fall_off_ym_notxm, new_ycoords[2][0], y00);
   x01 = lp_build_select(ivec_bld, fall_off_ym_notxp, new_xcoords[2][1], x01);
   y01 = lp_build_select(ivec_bld, fall_off_ym_notxp, new_ycoords[2][1], y01);
   x10 = lp_build_select(ivec_bld, fall_off_yp_notxm, new_xcoords[3][0], x10);
   y10 = lp_build_select(ivec_bld, fall_off_yp_notxm, new_ycoords[3][0], y10);
   x11 = lp_build_select(ivec_bld, fall_off_yp_notxp, new_xcoords[3][1], x11);
            z00 = lp_build_select(ivec_bld, fall_off_ym_notxm, new_faces[2], z00);
   z01 = lp_build_select(ivec_bld, fall_off_ym_notxp, new_faces[2], z01);
   z10 = lp_build_select(ivec_bld, fall_off_yp_notxm, new_faces[3], z10);
            if (bld->static_texture_state->target == PIPE_TEXTURE_CUBE_ARRAY) {
      /* now can add cube layer to face (per sample) */
   z00 = lp_build_add(ivec_bld, z00, coords[3]);
   z01 = lp_build_add(ivec_bld, z01, coords[3]);
   z10 = lp_build_add(ivec_bld, z10, coords[3]);
               LLVMBuildStore(builder, x00, xs[0]);
   LLVMBuildStore(builder, x01, xs[1]);
   LLVMBuildStore(builder, x10, xs[2]);
   LLVMBuildStore(builder, x11, xs[3]);
   LLVMBuildStore(builder, y00, ys[0]);
   LLVMBuildStore(builder, y01, ys[1]);
   LLVMBuildStore(builder, y10, ys[2]);
   LLVMBuildStore(builder, y11, ys[3]);
   LLVMBuildStore(builder, z00, zs[0]);
   LLVMBuildStore(builder, z01, zs[1]);
   LLVMBuildStore(builder, z10, zs[2]);
                     LLVMBuildStore(builder, x0, xs[0]);
   LLVMBuildStore(builder, x1, xs[1]);
   LLVMBuildStore(builder, x0, xs[2]);
   LLVMBuildStore(builder, x1, xs[3]);
   LLVMBuildStore(builder, y0, ys[0]);
   LLVMBuildStore(builder, y0, ys[1]);
   LLVMBuildStore(builder, y1, ys[2]);
   LLVMBuildStore(builder, y1, ys[3]);
   if (bld->static_texture_state->target == PIPE_TEXTURE_CUBE_ARRAY) {
      LLVMValueRef cube_layer = lp_build_add(ivec_bld, face, coords[3]);
   LLVMBuildStore(builder, cube_layer, zs[0]);
   LLVMBuildStore(builder, cube_layer, zs[1]);
   LLVMBuildStore(builder, cube_layer, zs[2]);
      } else {
      LLVMBuildStore(builder, face, zs[0]);
   LLVMBuildStore(builder, face, zs[1]);
   LLVMBuildStore(builder, face, zs[2]);
                        LLVMTypeRef type = ivec_bld->vec_type;
   x00 = LLVMBuildLoad2(builder, type, xs[0], "");
   x01 = LLVMBuildLoad2(builder, type, xs[1], "");
   x10 = LLVMBuildLoad2(builder, type, xs[2], "");
   x11 = LLVMBuildLoad2(builder, type, xs[3], "");
   y00 = LLVMBuildLoad2(builder, type, ys[0], "");
   y01 = LLVMBuildLoad2(builder, type, ys[1], "");
   y10 = LLVMBuildLoad2(builder, type, ys[2], "");
   y11 = LLVMBuildLoad2(builder, type, ys[3], "");
   z00 = LLVMBuildLoad2(builder, type, zs[0], "");
   z01 = LLVMBuildLoad2(builder, type, zs[1], "");
   z10 = LLVMBuildLoad2(builder, type, zs[2], "");
               if (linear_mask) {
      /*
   * Whack filter weights into place. Whatever texel had more weight is
   * the one which should have been selected by nearest filtering hence
   * just use 100% weight for it.
   */
   struct lp_build_context *c_bld = &bld->coord_bld;
   LLVMValueRef w1_mask, w1_weight;
            w1_mask = lp_build_cmp(c_bld, PIPE_FUNC_GREATER, s_fpart, half);
   /* this select is really just a "and" */
   w1_weight = lp_build_select(c_bld, w1_mask, c_bld->one, c_bld->zero);
   s_fpart = lp_build_select(c_bld, linear_mask, s_fpart, w1_weight);
   if (dims >= 2) {
      w1_mask = lp_build_cmp(c_bld, PIPE_FUNC_GREATER, t_fpart, half);
   w1_weight = lp_build_select(c_bld, w1_mask, c_bld->one, c_bld->zero);
   t_fpart = lp_build_select(c_bld, linear_mask, t_fpart, w1_weight);
   if (dims == 3) {
      w1_mask = lp_build_cmp(c_bld, PIPE_FUNC_GREATER, r_fpart, half);
   w1_weight = lp_build_select(c_bld, w1_mask, c_bld->one, c_bld->zero);
                     /*
   * Get texture colors.
   */
   /* get x0/x1 texels */
   lp_build_sample_texel_soa(bld,
                           lp_build_sample_texel_soa(bld,
                              if (dims == 1) {
      assert(!is_gather);
   if (bld->static_sampler_state->compare_mode == PIPE_TEX_COMPARE_NONE) {
      lp_build_reduce_filter(texel_bld,
                        bld->static_sampler_state->reduction_mode,
   0,
   4,
   } else {
      LLVMValueRef cmpval0, cmpval1;
   cmpval0 = lp_build_sample_comparefunc(bld, coords[4], neighbors[0][0][0]);
   cmpval1 = lp_build_sample_comparefunc(bld, coords[4], neighbors[0][1][0]);
   /* simplified lerp, AND mask with weight and add */
   colors_out[0] = lp_build_masklerp(texel_bld, s_fpart,
               } else {
      /* 2D/3D texture */
   struct lp_build_if_state corner_if;
            /* get x0/x1 texels at y1 */
   lp_build_sample_texel_soa(bld,
                           lp_build_sample_texel_soa(bld,
                              /*
   * To avoid having to duplicate linear_mask / fetch code use
   * another branch (with corner condition though edge would work
   * as well) here.
   */
   if (have_corners && accurate_cube_corners &&
      bld->static_sampler_state->reduction_mode == PIPE_TEX_REDUCTION_WEIGHTED_AVERAGE) {
                  colorss[0] = lp_build_alloca(bld->gallivm, coord_bld->vec_type, "cs0");
   colorss[1] = lp_build_alloca(bld->gallivm, coord_bld->vec_type, "cs1");
                                                   /* find corner */
   c00 = lp_build_and(ivec_bld, fall_off[0], fall_off[2]);
   c00f = LLVMBuildBitCast(builder, c00, coord_bld->vec_type, "");
   c01 = lp_build_and(ivec_bld, fall_off[1], fall_off[2]);
   c01f = LLVMBuildBitCast(builder, c01, coord_bld->vec_type, "");
   c10 = lp_build_and(ivec_bld, fall_off[0], fall_off[3]);
   c10f = LLVMBuildBitCast(builder, c10, coord_bld->vec_type, "");
                  if (!is_gather) {
      /*
   * we can't use standard 2d lerp as we need per-element weight
   * in case of corners, so just calculate bilinear result as
   * w00*s00 + w01*s01 + w10*s10 + w11*s11.
   * (This is actually less work than using 2d lerp, 7 vs. 9
   * instructions, however calculating the weights needs another 6,
   * so actually probably not slower than 2d lerp only for 4 channels
   * as weights only need to be calculated once - of course fixing
   * the weights has additional cost.)
   */
   LLVMValueRef w00, w01, w10, w11, wx0, wy0, c_weight, tmp;
   wx0 = lp_build_sub(coord_bld, coord_bld->one, s_fpart);
   wy0 = lp_build_sub(coord_bld, coord_bld->one, t_fpart);
   w00 = lp_build_mul(coord_bld, wx0, wy0);
   w01 = lp_build_mul(coord_bld, s_fpart, wy0);
                  /* find corner weight */
   c_weight = lp_build_select(coord_bld, c00, w00, coord_bld->zero);
   c_weight = lp_build_select(coord_bld, c01, w01, c_weight);
                  /*
   * add 1/3 of the corner weight to the weight of the 3 other
   * samples and null out corner weight.
   */
   c_weight = lp_build_mul(coord_bld, c_weight, one_third);
   w00 = lp_build_add(coord_bld, w00, c_weight);
   w00 = lp_build_andnot(coord_bld, w00, c00f);
   w01 = lp_build_add(coord_bld, w01, c_weight);
   w01 = lp_build_andnot(coord_bld, w01, c01f);
   w10 = lp_build_add(coord_bld, w10, c_weight);
   w10 = lp_build_andnot(coord_bld, w10, c10f);
                  if (bld->static_sampler_state->compare_mode ==
      PIPE_TEX_COMPARE_NONE) {
   for (unsigned chan = 0; chan < 4; chan++) {
      colors0[chan] = lp_build_mul(coord_bld, w00,
         tmp = lp_build_mul(coord_bld, w01, neighbors[0][1][chan]);
   colors0[chan] = lp_build_add(coord_bld, tmp, colors0[chan]);
   tmp = lp_build_mul(coord_bld, w10, neighbors[1][0][chan]);
   colors0[chan] = lp_build_add(coord_bld, tmp, colors0[chan]);
   tmp = lp_build_mul(coord_bld, w11, neighbors[1][1][chan]);
         } else {
      LLVMValueRef cmpval00, cmpval01, cmpval10, cmpval11;
   cmpval00 = lp_build_sample_comparefunc(bld, coords[4],
         cmpval01 = lp_build_sample_comparefunc(bld, coords[4],
         cmpval10 = lp_build_sample_comparefunc(bld, coords[4],
         cmpval11 = lp_build_sample_comparefunc(bld, coords[4],
         /*
   * inputs to interpolation are just masks so just add
   * masked weights together
   */
   cmpval00 = LLVMBuildBitCast(builder, cmpval00,
         cmpval01 = LLVMBuildBitCast(builder, cmpval01,
         cmpval10 = LLVMBuildBitCast(builder, cmpval10,
         cmpval11 = LLVMBuildBitCast(builder, cmpval11,
         colors0[0] = lp_build_and(coord_bld, w00, cmpval00);
   tmp = lp_build_and(coord_bld, w01, cmpval01);
   colors0[0] = lp_build_add(coord_bld, tmp, colors0[0]);
   tmp = lp_build_and(coord_bld, w10, cmpval10);
   colors0[0] = lp_build_add(coord_bld, tmp, colors0[0]);
   tmp = lp_build_and(coord_bld, w11, cmpval11);
   colors0[0] = lp_build_add(coord_bld, tmp, colors0[0]);
         } else {
      /*
   * We don't have any weights to adjust, so instead calculate
   * the fourth texel as simply the average of the other 3.
   * (This would work for non-gather too, however we'd have
   * a boatload more of the select stuff due to there being
   * 4 times as many colors as weights.)
   */
   LLVMValueRef col00, col01, col10, col11;
   LLVMValueRef colc, colc0, colc1;
   col10 = lp_build_swizzle_soa_channel(texel_bld,
         col11 = lp_build_swizzle_soa_channel(texel_bld,
         col01 = lp_build_swizzle_soa_channel(texel_bld,
                        /*
   * The spec says for comparison filtering, the comparison
   * must happen before synthesizing the new value.
   * This means all gathered values are always 0 or 1,
   * except for the non-existing texel, which can be 0,1/3,2/3,1...
   * Seems like we'd be allowed to just return 0 or 1 too, so we
   * could simplify and pass down the compare mask values to the
   * end (using int arithmetic/compare on the mask values to
   * construct the fourth texel) and only there convert to floats
   * but it's probably not worth it (it might be easier for the cpu
   * but not for the code)...
   */
   if (bld->static_sampler_state->compare_mode !=
      PIPE_TEX_COMPARE_NONE) {
   LLVMValueRef cmpval00, cmpval01, cmpval10, cmpval11;
   cmpval00 = lp_build_sample_comparefunc(bld, coords[4], col00);
   cmpval01 = lp_build_sample_comparefunc(bld, coords[4], col01);
   cmpval10 = lp_build_sample_comparefunc(bld, coords[4], col10);
   cmpval11 = lp_build_sample_comparefunc(bld, coords[4], col11);
   col00 = lp_build_select(texel_bld, cmpval00,
         col01 = lp_build_select(texel_bld, cmpval01,
         col10 = lp_build_select(texel_bld, cmpval10,
                           /*
   * Null out corner color.
   */
   col00 = lp_build_andnot(coord_bld, col00, c00f);
   col01 = lp_build_andnot(coord_bld, col01, c01f);
                  /*
   * New corner texel color is all colors added / 3.
   */
   colc0 = lp_build_add(coord_bld, col00, col01);
   colc1 = lp_build_add(coord_bld, col10, col11);
                  /*
   * Replace the corner texel color with the new value.
   */
   col00 = lp_build_select(coord_bld, c00, colc, col00);
   col01 = lp_build_select(coord_bld, c01, colc, col01);
                  colors0[0] = col10;
   colors0[1] = col11;
   colors0[2] = col01;
               LLVMBuildStore(builder, colors0[0], colorss[0]);
   LLVMBuildStore(builder, colors0[1], colorss[1]);
                              if (bld->static_sampler_state->compare_mode == PIPE_TEX_COMPARE_NONE) {
      if (is_gather) {
      /*
   * Just assign the red channel (no component selection yet).
   * This is a bit hackish, we usually do the swizzle at the
   * end of sampling (much less values to swizzle), but this
   * obviously cannot work when using gather.
   */
   colors0[0] = lp_build_swizzle_soa_channel(texel_bld,
               colors0[1] = lp_build_swizzle_soa_channel(texel_bld,
               colors0[2] = lp_build_swizzle_soa_channel(texel_bld,
               colors0[3] = lp_build_swizzle_soa_channel(texel_bld,
            } else {
      /* Bilinear interpolate the four samples from the 2D image / 3D slice */
   lp_build_reduce_filter_2d(texel_bld,
                           bld->static_sampler_state->reduction_mode,
   0,
   4,
   s_fpart,
   t_fpart,
         } else {
      LLVMValueRef cmpval00, cmpval01, cmpval10, cmpval11;
   cmpval00 = lp_build_sample_comparefunc(bld, coords[4], neighbors[0][0][0]);
   cmpval01 = lp_build_sample_comparefunc(bld, coords[4], neighbors[0][1][0]);
                  if (is_gather) {
      /* more hacks for swizzling, should be X, ONE or ZERO... */
   colors0[0] = lp_build_select(texel_bld, cmpval10,
         colors0[1] = lp_build_select(texel_bld, cmpval11,
         colors0[2] = lp_build_select(texel_bld, cmpval01,
         colors0[3] = lp_build_select(texel_bld, cmpval00,
      } else {
      colors0[0] = lp_build_masklerp2d(texel_bld, s_fpart, t_fpart,
                        if (have_corners && accurate_cube_corners &&
      bld->static_sampler_state->reduction_mode == PIPE_TEX_REDUCTION_WEIGHTED_AVERAGE) {
   LLVMBuildStore(builder, colors0[0], colorss[0]);
   LLVMBuildStore(builder, colors0[1], colorss[1]);
                           colors0[0] = LLVMBuildLoad2(builder, coord_bld->vec_type, colorss[0], "");
   colors0[1] = LLVMBuildLoad2(builder, coord_bld->vec_type, colorss[1], "");
   colors0[2] = LLVMBuildLoad2(builder, coord_bld->vec_type, colorss[2], "");
               if (dims == 3) {
                              /* get x0/x1/y0/y1 texels at z1 */
   lp_build_sample_texel_soa(bld,
                           lp_build_sample_texel_soa(bld,
                           lp_build_sample_texel_soa(bld,
                           lp_build_sample_texel_soa(bld,
                              if (bld->static_sampler_state->compare_mode == PIPE_TEX_COMPARE_NONE) {
      /* Bilinear interpolate the four samples from the second Z slice */
   lp_build_reduce_filter_2d(texel_bld,
                           bld->static_sampler_state->reduction_mode,
   0,
   4,
   s_fpart,
                  /* Linearly interpolate the two samples from the two 3D slices */
   lp_build_reduce_filter(texel_bld,
                        bld->static_sampler_state->reduction_mode,
   0,
   4,
   } else {
      LLVMValueRef cmpval00, cmpval01, cmpval10, cmpval11;
   cmpval00 = lp_build_sample_comparefunc(bld, coords[4], neighbors[0][0][0]);
   cmpval01 = lp_build_sample_comparefunc(bld, coords[4], neighbors[0][1][0]);
   cmpval10 = lp_build_sample_comparefunc(bld, coords[4], neighbors[1][0][0]);
   cmpval11 = lp_build_sample_comparefunc(bld, coords[4], neighbors[1][1][0]);
   colors1[0] = lp_build_masklerp2d(texel_bld, s_fpart, t_fpart,
         /* Linearly interpolate the two samples from the two 3D slices */
   colors_out[0] = lp_build_lerp(texel_bld,
                           } else {
      /* 2D tex */
   for (unsigned chan = 0; chan < 4; chan++) {
               }
   if (is_gather) {
      /*
   * For gather, we can't do our usual channel swizzling done later,
   * so do it here. It only really matters for 0/1 swizzles in case
   * of comparison filtering, since in this case the results would be
   * wrong, without comparison it should all work out alright but it
   * can't hurt to do that here, since it will instantly drop all
   * calculations above, though it's a rather stupid idea to do
   * gather on a channel which will always return 0 or 1 in any case...
   */
   if (chan_swiz == PIPE_SWIZZLE_1) {
      for (unsigned chan = 0; chan < 4; chan++) {
            } else if (chan_swiz == PIPE_SWIZZLE_0) {
      for (unsigned chan = 0; chan < 4; chan++) {
                  }
         /**
   * Sample the texture/mipmap using given image filter and mip filter.
   * ilevel0 and ilevel1 indicate the two mipmap levels to sample
   * from (vectors or scalars).
   * If we're using nearest miplevel sampling the '1' values will be null/unused.
   */
   static void
   lp_build_sample_mipmap(struct lp_build_sample_context *bld,
                        unsigned img_filter,
   unsigned mip_filter,
   bool is_gather,
   const LLVMValueRef *coords,
   const LLVMValueRef *offsets,
      {
      LLVMBuilderRef builder = bld->gallivm->builder;
   LLVMValueRef size0 = NULL;
   LLVMValueRef size1 = NULL;
   LLVMValueRef row_stride0_vec = NULL;
   LLVMValueRef row_stride1_vec = NULL;
   LLVMValueRef img_stride0_vec = NULL;
   LLVMValueRef img_stride1_vec = NULL;
   LLVMValueRef data_ptr0 = NULL;
   LLVMValueRef data_ptr1 = NULL;
   LLVMValueRef mipoff0 = NULL;
   LLVMValueRef mipoff1 = NULL;
            /* sample the first mipmap level */
   lp_build_mipmap_level_sizes(bld, ilevel0,
               if (bld->num_mips == 1) {
         } else {
      /* This path should work for num_lods 1 too but slightly less efficient */
   data_ptr0 = bld->base_ptr;
               if (img_filter == PIPE_TEX_FILTER_NEAREST) {
      lp_build_sample_image_nearest(bld, size0,
                  } else {
      assert(img_filter == PIPE_TEX_FILTER_LINEAR);
   lp_build_sample_image_linear(bld, is_gather, size0, NULL,
                           /* Store the first level's colors in the output variables */
   for (unsigned chan = 0; chan < 4; chan++) {
                  if (mip_filter == PIPE_TEX_MIPFILTER_LINEAR) {
      struct lp_build_if_state if_ctx;
            /* need_lerp = lod_fpart > 0 */
   if (bld->num_lods == 1) {
      need_lerp = LLVMBuildFCmp(builder, LLVMRealUGT,
            } else {
      /*
   * We'll do mip filtering if any of the quads (or individual
   * pixel in case of per-pixel lod) need it.
   * It might be better to split the vectors here and only fetch/filter
   * quads which need it (if there's one lod per quad).
   */
   need_lerp = lp_build_compare(bld->gallivm, bld->lodf_bld.type,
               need_lerp = lp_build_any_true_range(&bld->lodi_bld, bld->num_lods, need_lerp);
               lp_build_if(&if_ctx, bld->gallivm, need_lerp);
   {
      /*
   * We unfortunately need to clamp lod_fpart here since we can get
   * negative values which would screw up filtering if not all
   * lod_fpart values have same sign.
   */
   lod_fpart = lp_build_max(&bld->lodf_bld, lod_fpart,
         /* sample the second mipmap level */
   lp_build_mipmap_level_sizes(bld, ilevel1,
               if (bld->num_mips == 1) {
         } else {
      data_ptr1 = bld->base_ptr;
      }
   if (img_filter == PIPE_TEX_FILTER_NEAREST) {
      lp_build_sample_image_nearest(bld, size1,
                  } else {
      lp_build_sample_image_linear(bld, false, size1, NULL,
                                    if (bld->num_lods != bld->coord_type.length)
      lod_fpart = lp_build_unpack_broadcast_aos_scalars(bld->gallivm,
                     for (unsigned chan = 0; chan < 4; chan++) {
      colors0[chan] = lp_build_lerp(&bld->texel_bld, lod_fpart,
                     }
         }
         /**
   * Sample the texture/mipmap using given mip filter, and using
   * both nearest and linear filtering at the same time depending
   * on linear_mask.
   * lod can be per quad but linear_mask is always per pixel.
   * ilevel0 and ilevel1 indicate the two mipmap levels to sample
   * from (vectors or scalars).
   * If we're using nearest miplevel sampling the '1' values will be null/unused.
   */
   static void
   lp_build_sample_mipmap_both(struct lp_build_sample_context *bld,
                              LLVMValueRef linear_mask,
   unsigned mip_filter,
   const LLVMValueRef *coords,
   const LLVMValueRef *offsets,
      {
      LLVMBuilderRef builder = bld->gallivm->builder;
   LLVMValueRef size0 = NULL;
   LLVMValueRef size1 = NULL;
   LLVMValueRef row_stride0_vec = NULL;
   LLVMValueRef row_stride1_vec = NULL;
   LLVMValueRef img_stride0_vec = NULL;
   LLVMValueRef img_stride1_vec = NULL;
   LLVMValueRef data_ptr0 = NULL;
   LLVMValueRef data_ptr1 = NULL;
   LLVMValueRef mipoff0 = NULL;
   LLVMValueRef mipoff1 = NULL;
            /* sample the first mipmap level */
   lp_build_mipmap_level_sizes(bld, ilevel0,
               if (bld->num_mips == 1) {
         } else {
      /* This path should work for num_lods 1 too but slightly less efficient */
   data_ptr0 = bld->base_ptr;
               lp_build_sample_image_linear(bld, false, size0, linear_mask,
                        /* Store the first level's colors in the output variables */
   for (unsigned chan = 0; chan < 4; chan++) {
                  if (mip_filter == PIPE_TEX_MIPFILTER_LINEAR) {
      struct lp_build_if_state if_ctx;
            /*
   * We'll do mip filtering if any of the quads (or individual
   * pixel in case of per-pixel lod) need it.
   * Note using lod_positive here not lod_fpart since it may be the same
   * condition as that used in the outer "if" in the caller hence llvm
   * should be able to merge the branches in this case.
   */
   need_lerp = lp_build_any_true_range(&bld->lodi_bld, bld->num_lods, lod_positive);
            lp_build_if(&if_ctx, bld->gallivm, need_lerp);
   {
      /*
   * We unfortunately need to clamp lod_fpart here since we can get
   * negative values which would screw up filtering if not all
   * lod_fpart values have same sign.
   */
   lod_fpart = lp_build_max(&bld->lodf_bld, lod_fpart,
         /* sample the second mipmap level */
   lp_build_mipmap_level_sizes(bld, ilevel1,
               if (bld->num_mips == 1) {
         } else {
      data_ptr1 = bld->base_ptr;
               lp_build_sample_image_linear(bld, false, size1, linear_mask,
                                 if (bld->num_lods != bld->coord_type.length)
      lod_fpart = lp_build_unpack_broadcast_aos_scalars(bld->gallivm,
                     for (unsigned chan = 0; chan < 4; chan++) {
      colors0[chan] = lp_build_lerp(&bld->texel_bld, lod_fpart,
                     }
         }
         /**
   * Build (per-coord) layer value.
   * Either clamp layer to valid values or fill in optional out_of_bounds
   * value and just return value unclamped.
   */
   static LLVMValueRef
   lp_build_layer_coord(struct lp_build_sample_context *bld,
                           {
      LLVMValueRef num_layers;
            num_layers = bld->dynamic_state->depth(bld->gallivm, bld->resources_type,
            if (out_of_bounds) {
      LLVMValueRef out1, out;
   assert(!is_cube_array);
   num_layers = lp_build_broadcast_scalar(int_coord_bld, num_layers);
   out = lp_build_cmp(int_coord_bld, PIPE_FUNC_LESS, layer, int_coord_bld->zero);
   out1 = lp_build_cmp(int_coord_bld, PIPE_FUNC_GEQUAL, layer, num_layers);
   *out_of_bounds = lp_build_or(int_coord_bld, out, out1);
      } else {
      LLVMValueRef maxlayer;
   LLVMValueRef s = is_cube_array ? lp_build_const_int32(bld->gallivm, 6) :
         maxlayer = lp_build_sub(&bld->int_bld, num_layers, s);
   maxlayer = lp_build_broadcast_scalar(int_coord_bld, maxlayer);
         }
      static void
   lp_build_sample_ms_offset(struct lp_build_context *int_coord_bld,
                           LLVMValueRef ms_index,
   {
      LLVMValueRef out1;
   num_samples = lp_build_broadcast_scalar(int_coord_bld, num_samples);
   sample_stride = lp_build_broadcast_scalar(int_coord_bld, sample_stride);
   out1 = lp_build_cmp(int_coord_bld, PIPE_FUNC_LESS, ms_index, int_coord_bld->zero);
   *out_of_bounds = lp_build_or(int_coord_bld, *out_of_bounds, out1);
   out1 = lp_build_cmp(int_coord_bld, PIPE_FUNC_GEQUAL, ms_index, num_samples);
   *out_of_bounds = lp_build_or(int_coord_bld, *out_of_bounds, out1);
   LLVMValueRef sample_offset = lp_build_mul(int_coord_bld,
            }
         #define WEIGHT_LUT_SIZE 1024
         static void
   lp_build_sample_aniso(struct lp_build_sample_context *bld,
                        unsigned img_filter,
   unsigned mip_filter,
   bool is_gather,
   const LLVMValueRef *coords,
   const LLVMValueRef *offsets,
      {
      struct gallivm_state *gallivm = bld->gallivm;
   LLVMBuilderRef builder = gallivm->builder;
   struct lp_build_context *coord_bld = &bld->coord_bld;
   struct lp_build_context *float_size_bld = &bld->float_size_in_bld;
   LLVMValueRef ddx_ddy = lp_build_packed_ddx_ddy_twocoord(&bld->coord_bld, coords[0], coords[1]);
   LLVMValueRef float_size;
   LLVMTypeRef i32t = LLVMInt32TypeInContext(gallivm->context);
   LLVMValueRef index0 = LLVMConstInt(i32t, 0, 0);
   LLVMValueRef index1 = LLVMConstInt(i32t, 1, 0);
   const unsigned length = bld->coord_bld.type.length;
   const unsigned num_quads = length / 4;
   LLVMValueRef filter_table = bld->aniso_filter_table;
   LLVMValueRef size0, row_stride0_vec, img_stride0_vec;
            lp_build_mipmap_level_sizes(bld, ilevel0,
               if (bld->num_mips == 1) {
         } else {
      /* This path should work for num_lods 1 too but slightly less efficient */
   data_ptr0 = bld->base_ptr;
                        LLVMValueRef float_size_lvl = lp_build_int_to_float(&bld->float_size_bld, size0);
   /* extract width and height into vectors for use later */
   static const unsigned char swizzle15[] = { /* no-op swizzle */
         };
   static const unsigned char swizzle04[] = { /* no-op swizzle */
         };
            width_dim = lp_build_swizzle_aos_n(gallivm, float_size_lvl, swizzle04,
               height_dim = lp_build_swizzle_aos_n(gallivm, float_size_lvl, swizzle15,
                     /* shuffle width/height for ddx/ddy calculations. */
            for (unsigned i = 0; i < num_quads; i++) {
      shuffles[i*4+0] = shuffles[i*4+1] = index0;
               LLVMValueRef floatdim =
      LLVMBuildShuffleVector(builder, float_size, float_size,
                  LLVMValueRef scaling =
         scaling = lp_build_int_to_float(&bld->levelf_bld, scaling);
            if (bld->levelf_bld.type.length != length) {
      if (bld->levelf_bld.type.length == 1) {
      scaling = lp_build_broadcast_scalar(coord_bld,
      } else {
      scaling = lp_build_unpack_broadcast_aos_scalars(bld->gallivm,
                                       static const unsigned char swizzle01[] = { /* no-op swizzle */
         };
   static const unsigned char swizzle23[] = {
                  LLVMValueRef ddx_ddys, ddx_ddyt;
   ddx_ddys = lp_build_swizzle_aos(coord_bld, ddx_ddy, swizzle01);
            /* compute ellipse coefficients */
   /* * A*x*x + B*x*y + C*y*y = F.*/
   /* float A = vx*vx+vy*vy+1; */
            LLVMValueRef Ay = lp_build_swizzle_aos(coord_bld, A, swizzle15);
   A = lp_build_add(coord_bld, A, Ay);
   A = lp_build_add(coord_bld, A, coord_bld->one);
            /* float B = -2*(ux*vx+uy*vy); */
   LLVMValueRef B = lp_build_mul(coord_bld, ddx_ddys, ddx_ddyt);
   LLVMValueRef By = lp_build_swizzle_aos(coord_bld, B, swizzle15);
   B = lp_build_add(coord_bld, B, By);
   B = lp_build_mul_imm(coord_bld, B, -2);
            /* float C = ux*ux+uy*uy+1; */
   LLVMValueRef C = lp_build_mul(coord_bld, ddx_ddys, ddx_ddys);
   LLVMValueRef Cy = lp_build_swizzle_aos(coord_bld, C, swizzle15);
   C = lp_build_add(coord_bld, C, Cy);
   C = lp_build_add(coord_bld, C, coord_bld->one);
            /* float F = A*C-B*B/4.0f; */
   LLVMValueRef F = lp_build_mul(coord_bld, B, B);
   F = lp_build_div(coord_bld, F, lp_build_const_vec(gallivm, coord_bld->type, 4.0));
   LLVMValueRef F_p2 = lp_build_mul(coord_bld, A, C);
            /* compute ellipse bounding box in texture space */
   /* const float d = -B*B+4.0f*C*A; */
   LLVMValueRef d = lp_build_sub(coord_bld, coord_bld->zero, lp_build_mul(coord_bld, B, B));
   LLVMValueRef d_p2 = lp_build_mul(coord_bld, A, C);
   d_p2 = lp_build_mul_imm(coord_bld, d_p2, 4);
            /* const float box_u = 2.0f / d * sqrtf(d*C*F); */
   /* box_u -> half of bbox with   */
   LLVMValueRef temp;
   temp = lp_build_mul(coord_bld, d, C);
   temp = lp_build_mul(coord_bld, temp, F);
            LLVMValueRef box_u = lp_build_div(coord_bld, lp_build_const_vec(gallivm, coord_bld->type, 2.0), d);
            /* const float box_v = 2.0f / d * sqrtf(A*d*F); */
   /* box_v -> half of bbox height */
   temp = lp_build_mul(coord_bld, A, d);
   temp = lp_build_mul(coord_bld, temp, F);
            LLVMValueRef box_v = lp_build_div(coord_bld, lp_build_const_vec(gallivm, coord_bld->type, 2.0), d);
            /* Scale ellipse formula to directly index the Filter Lookup Table.
   * i.e. scale so that F = WEIGHT_LUT_SIZE-1
   */
            A = lp_build_mul(coord_bld, A, formScale);
   B = lp_build_mul(coord_bld, B, formScale);
   C = lp_build_mul(coord_bld, C, formScale);
                     /* Heckbert MS thesis, p. 59; scan over the bounding box of the ellipse
   * and incrementally update the value of Ax^2+Bxy*Cy^2; when this
   * value, q, is less than F, we're inside the ellipse
            LLVMValueRef float_size0 = lp_build_int_to_float(float_size_bld, bld->int_size);
   LLVMValueRef width0 = lp_build_extract_broadcast(gallivm,
                     LLVMValueRef height0 = lp_build_extract_broadcast(gallivm,
                        /* texture->width0 * scaling */
   width0 = lp_build_mul(coord_bld, width0, scaling);
   /* texture->height0 * scaling */
            /* tex_u = -0.5f * s[j] * texture->width0 * scaling */
   LLVMValueRef tex_u = lp_build_mul(coord_bld, coords[0], width0);
            /* tex_v = -0.5f * t[j] * texture->height0 * scaling */
   LLVMValueRef tex_v = lp_build_mul(coord_bld, coords[1], height0);
            /* const int u0 = (int) floorf(tex_u - box_u); */
   LLVMValueRef u0 = lp_build_itrunc(coord_bld, lp_build_floor(coord_bld, lp_build_sub(coord_bld, tex_u, box_u)));
   /* const int u1 = (int) ceilf(tex_u + box_u); */
            /* const int v0 = (int) floorf(tex_v - box_v); */
   LLVMValueRef v0 = lp_build_itrunc(coord_bld, lp_build_floor(coord_bld, lp_build_sub(coord_bld, tex_v, box_v)));
   /* const int v1 = (int) ceilf(tex_v + box_v); */
            /* const float U = u0 - tex_u; */
            /* A * (2 * U + 1) */
   LLVMValueRef dq_base = lp_build_mul_imm(coord_bld, U, 2);
   dq_base = lp_build_add(coord_bld, dq_base, coord_bld->one);
            /* A * U * U */
   LLVMValueRef q_base = lp_build_mul(coord_bld, U, U);
            LLVMValueRef colors0[4];
            for (unsigned chan = 0; chan < 4; chan++)
            LLVMValueRef q_store, dq_store;
   q_store = lp_build_alloca(gallivm, bld->coord_bld.vec_type, "q");
            LLVMValueRef v_limiter = lp_build_alloca(gallivm, bld->int_coord_bld.vec_type, "v_limiter");
                     /* create an LLVM loop block for the V iterator */
            LLVMBuildBr(builder, v_loop_block);
            LLVMValueRef v_val = LLVMBuildLoad2(builder, bld->int_coord_bld.vec_type, v_limiter, "");
            /* loop over V values. */
   {
      /*  const float V = v - tex_v; */
   LLVMValueRef V =
                  /* float dq = dq_base + B * V; */
   LLVMValueRef dq = lp_build_mul(coord_bld, V, B);
            /* float q = (C * V + B * U) * V + q_base */
   LLVMValueRef q = lp_build_mul(coord_bld, C, V);
   q = lp_build_add(coord_bld, q, lp_build_mul(coord_bld, B, U));
   q = lp_build_mul(coord_bld, q, V);
            LLVMBuildStore(builder, q, q_store);
                     /* create an LLVM loop block for the V iterator */
            LLVMBuildBr(builder, u_loop_block);
            LLVMValueRef u_val = LLVMBuildLoad2(builder, bld->int_coord_bld.vec_type,
         LLVMValueRef u_mask = LLVMBuildICmp(builder,
                        /* loop over U values */
   {
      /* q = (int)q */
   q = lp_build_itrunc(coord_bld,
                  /*
   * avoid OOB access to filter table, generate a mask for q > 1024,
   * then truncate it.
   */
   LLVMValueRef q_mask = LLVMBuildICmp(builder,
                                             /* update the offsets to deal with float size. */
                  /* Lookup weights in filter table */
   LLVMValueRef weights = lp_build_gather(gallivm, coord_bld->type.length,
                        /*
   * Mask off the weights here which should ensure no-op for loops
   * where some of the u/v values are not being calculated.
   */
   weights = LLVMBuildBitCast(builder, weights, bld->int_coord_bld.vec_type, "");
   weights = lp_build_and(&bld->int_coord_bld, weights, LLVMBuildSExt(builder, u_mask, bld->int_coord_bld.vec_type, ""));
   weights = lp_build_and(&bld->int_coord_bld, weights, LLVMBuildSExt(builder, v_mask, bld->int_coord_bld.vec_type, ""));
                                 LLVMValueRef wnz = LLVMBuildFCmp(gallivm->builder, LLVMRealUNE,
         wnz = LLVMBuildSExt(builder, wnz, bld->int_coord_bld.vec_type, "");
   wnz = lp_build_any_true_range(&bld->coord_bld, bld->coord_bld.type.length, wnz);
   lp_build_if(&noloadw0, gallivm, wnz);
   LLVMValueRef new_coords[4];
   new_coords[0] = lp_build_div(coord_bld, lp_build_int_to_float(coord_bld, u_val), width_dim);
   new_coords[1] = lp_build_div(coord_bld, lp_build_int_to_float(coord_bld, v_val), height_dim);
                  /* lookup q in filter table */
   LLVMValueRef temp_colors[4];
   lp_build_sample_image_nearest(bld, size0,
                                          tcolor = lp_build_add(&bld->texel_bld, tcolor, lp_build_mul(&bld->texel_bld, temp_colors[chan], weights));
               /* multiple colors by weight and add in. */
   /* den += weight; */
   LLVMValueRef den = LLVMBuildLoad2(builder, bld->texel_bld.vec_type, den_store, "");
                  lp_build_endif(&noloadw0);
   /* q += dq; */
   /* dq += ddq; */
   q = LLVMBuildLoad2(builder, bld->texel_bld.vec_type, q_store, "");
   dq = LLVMBuildLoad2(builder, bld->texel_bld.vec_type, dq_store, "");
   q = lp_build_add(coord_bld, q, dq);
   dq = lp_build_add(coord_bld, dq, ddq);
   LLVMBuildStore(builder, q, q_store);
      }
   /* u += 1 */
   u_val = LLVMBuildLoad2(builder, bld->int_coord_bld.vec_type, u_limiter, "");
   u_val = lp_build_add(&bld->int_coord_bld, u_val, bld->int_coord_bld.one);
            u_mask = LLVMBuildICmp(builder,
                     LLVMValueRef u_end_cond = LLVMBuildSExt(builder, u_mask, bld->int_coord_bld.vec_type, "");
                     LLVMBuildCondBr(builder, u_end_cond,
                           /* v += 1 */
   v_val = LLVMBuildLoad2(builder, bld->int_coord_bld.vec_type, v_limiter, "");
   v_val = lp_build_add(&bld->int_coord_bld, v_val, bld->int_coord_bld.one);
            v_mask = LLVMBuildICmp(builder,
                     LLVMValueRef v_end_cond = LLVMBuildSExt(builder, v_mask,
         v_end_cond = lp_build_any_true_range(&bld->coord_bld,
                     LLVMBuildCondBr(builder, v_end_cond,
                              for (unsigned chan = 0; chan < 4; chan++) {
      colors0[chan] =
      lp_build_div(&bld->texel_bld,
                  LLVMValueRef den0 = lp_build_cmp(&bld->coord_bld, PIPE_FUNC_EQUAL,
            LLVMValueRef den0_any =
      lp_build_any_true_range(&bld->coord_bld,
         struct lp_build_if_state den0_fallback;
   lp_build_if(&den0_fallback, gallivm, den0_any);
   {
      LLVMValueRef colors_den0[4];
   lp_build_sample_image_linear(bld, false, size0, NULL,
                     for (unsigned chan = 0; chan < 4; chan++) {
      LLVMValueRef chan_val =
      lp_build_select(&bld->texel_bld, den0,
            }
   lp_build_else(&den0_fallback);
   {
      for (unsigned chan = 0; chan < 4; chan++) {
            }
      }
         /**
   * Calculate cube face, lod, mip levels.
   */
   static void
   lp_build_sample_common(struct lp_build_sample_context *bld,
                        bool is_lodq,
   unsigned texture_index,
   unsigned sampler_index,
   LLVMValueRef *coords,
   const struct lp_derivatives *derivs, /* optional */
   LLVMValueRef lod_bias, /* optional */
   LLVMValueRef explicit_lod, /* optional */
   LLVMValueRef *lod_pos_or_zero,
      {
      const unsigned mip_filter = bld->static_sampler_state->min_mip_filter;
   const unsigned min_filter = bld->static_sampler_state->min_img_filter;
   const unsigned mag_filter = bld->static_sampler_state->mag_img_filter;
   const unsigned target = bld->static_texture_state->target;
   const bool aniso = bld->static_sampler_state->aniso;
   LLVMValueRef first_level, last_level;
   LLVMValueRef lod_ipart = NULL;
            /*
   printf("%s mip %d  min %d  mag %d\n", __func__,
                  first_level = bld->dynamic_state->first_level(bld->gallivm,
                     last_level = bld->dynamic_state->last_level(bld->gallivm,
                        /*
   * Choose cube face, recompute texcoords for the chosen face and
   * calculate / transform derivatives.
   */
   if (target == PIPE_TEXTURE_CUBE || target == PIPE_TEXTURE_CUBE_ARRAY) {
      bool need_derivs = ((min_filter != mag_filter ||
                     lp_build_cube_lookup(bld, coords, derivs, &cube_derivs, need_derivs);
   if (need_derivs)
            if (target == PIPE_TEXTURE_CUBE_ARRAY && !is_lodq) {
      /* calculate cube layer coord now */
   LLVMValueRef layer = lp_build_iround(&bld->coord_bld, coords[3]);
   LLVMValueRef six = lp_build_const_int_vec(bld->gallivm, bld->int_coord_type, 6);
   layer = lp_build_mul(&bld->int_coord_bld, layer, six);
   coords[3] = lp_build_layer_coord(bld, texture_index, true, layer, NULL);
         } else if ((target == PIPE_TEXTURE_1D_ARRAY ||
            coords[2] = lp_build_iround(&bld->coord_bld, coords[2]);
               if (bld->static_sampler_state->compare_mode != PIPE_TEX_COMPARE_NONE) {
      /*
   * Clamp p coords to [0,1] for fixed function depth texture format here.
   * Technically this is not entirely correct for unorm depth as the ref
   * value should be converted to the depth format (quantization!) and
   * comparison then done in texture format. This would actually help
   * performance (since only need to do it once and could save the
   * per-sample conversion of texels to floats instead), but it would need
   * more messy code (would need to push at least some bits down to actual
   * fetch so conversion could be skipped, and would have ugly interaction
   * with border color, would need to convert border color to that format
   * too or do some other tricks to make it work).
   */
   const struct util_format_description *format_desc = bld->format_desc;
   /* not entirely sure we couldn't end up with non-valid swizzle here */
   const enum util_format_type chan_type =
      format_desc->swizzle[0] <= PIPE_SWIZZLE_W
   ? format_desc->channel[format_desc->swizzle[0]].type
      if (chan_type != UTIL_FORMAT_TYPE_FLOAT) {
      coords[4] = lp_build_clamp(&bld->coord_bld, coords[4],
                  /*
   * Compute the level of detail (float).
   */
   if (min_filter != mag_filter ||
      mip_filter != PIPE_TEX_MIPFILTER_NONE || is_lodq) {
            if (aniso)
      max_aniso = bld->dynamic_state->max_aniso(bld->gallivm,
                     /* Need to compute lod either to choose mipmap levels or to
   * distinguish between minification/magnification with one mipmap level.
   */
   LLVMValueRef first_level_vec =
         lp_build_lod_selector(bld, is_lodq, sampler_index,
                        first_level_vec,
      if (is_lodq) {
      last_level = lp_build_sub(&bld->int_bld, last_level, first_level);
                  switch (mip_filter) {
   case PIPE_TEX_MIPFILTER_NONE:
      *lod_fpart = bld->lodf_bld.zero;
      case PIPE_TEX_MIPFILTER_NEAREST:
      *lod_fpart = lp_build_round(&bld->lodf_bld, *lod_fpart);
      case PIPE_TEX_MIPFILTER_LINEAR:
      *lod_fpart = lp_build_clamp(&bld->lodf_bld, *lod_fpart,
            }
         } else {
      lod_ipart = bld->lodi_bld.zero;
               if ((bld->num_lods != bld->num_mips || bld->num_lods == 1) &&
      bld->lodi_bld.type.length != 1) {
   /* only makes sense if there's just a single mip level */
   assert(bld->num_mips == 1);
               first_level = lp_build_broadcast_scalar(&bld->leveli_bld, first_level);
            /*
   * Compute integer mipmap level(s) to fetch texels from: ilevel0, ilevel1
            if (aniso) {
      lp_build_nearest_mip_level(bld,
                           switch (mip_filter) {
   default:
         case PIPE_TEX_MIPFILTER_NONE:
      /* always use mip level 0 */
   *ilevel0 = first_level;
      case PIPE_TEX_MIPFILTER_NEAREST:
      assert(lod_ipart);
   lp_build_nearest_mip_level(bld,
                  case PIPE_TEX_MIPFILTER_LINEAR:
      assert(lod_ipart);
            lp_build_linear_mip_levels(bld, texture_index,
                           }
         static void
   lp_build_clamp_border_color(struct lp_build_sample_context *bld,
         {
      struct gallivm_state *gallivm = bld->gallivm;
   LLVMBuilderRef builder = gallivm->builder;
   LLVMValueRef border_color_ptr =
      bld->dynamic_state->border_color(gallivm,
            LLVMValueRef border_color;
   const struct util_format_description *format_desc = bld->format_desc;
   struct lp_type vec4_type = bld->texel_type;
   struct lp_build_context vec4_bld;
   LLVMValueRef min_clamp = NULL;
            /*
   * For normalized format need to clamp border color (technically
   * probably should also quantize the data). Really sucks doing this
   * here but can't avoid at least for now since this is part of
   * sampler state and texture format is part of sampler_view state.
   * GL expects also expects clamping for uint/sint formats too so
   * do that as well (d3d10 can't end up here with uint/sint since it
   * only supports them with ld).
   */
   vec4_type.length = 4;
            /*
   * Vectorized clamping of border color. Loading is a bit of a hack since
   * we just cast the pointer to float array to pointer to vec4
   * (int or float).
   */
   LLVMTypeRef border_color_type = LLVMArrayType(LLVMFloatTypeInContext(gallivm->context), 4);
   border_color_ptr = lp_build_array_get_ptr2(gallivm, border_color_type, border_color_ptr,
         border_color_ptr = LLVMBuildBitCast(builder, border_color_ptr,
         border_color = LLVMBuildLoad2(builder, vec4_bld.vec_type, border_color_ptr, "");
   /* we don't have aligned type in the dynamic state unfortunately */
            /*
   * Instead of having some incredibly complex logic which will try to figure
   * out clamping necessary for each channel, simply use the first channel,
   * and treat mixed signed/unsigned normalized formats specially.  (Mixed
   * non-normalized, which wouldn't work at all here, do not exist for a good
   * reason.)
   */
   if (format_desc->layout == UTIL_FORMAT_LAYOUT_PLAIN) {
      int chan;
   /* d/s needs special handling because both present means just sampling depth */
   if (util_format_is_depth_and_stencil(format_desc->format)) {
         } else {
         }
   if (chan >= 0 && chan <= PIPE_SWIZZLE_W) {
      unsigned chan_type = format_desc->channel[chan].type;
   unsigned chan_norm = format_desc->channel[chan].normalized;
   unsigned chan_pure = format_desc->channel[chan].pure_integer;
   if (chan_type == UTIL_FORMAT_TYPE_SIGNED) {
      if (chan_norm) {
      min_clamp = lp_build_const_vec(gallivm, vec4_type, -1.0F);
      } else if (chan_pure) {
      /*
   * Border color was stored as int, hence need min/max clamp
   * only if chan has less than 32 bits..
   */
   unsigned chan_size = format_desc->channel[chan].size;
   if (chan_size < 32) {
      min_clamp = lp_build_const_int_vec(gallivm, vec4_type,
         max_clamp = lp_build_const_int_vec(gallivm, vec4_type,
         }
      } else if (chan_type == UTIL_FORMAT_TYPE_UNSIGNED) {
      if (chan_norm) {
      min_clamp = vec4_bld.zero;
      } else if (chan_pure) {
      /*
   * Need a ugly hack here, because we don't have Z32_FLOAT_X8X24
   * we use Z32_FLOAT_S8X24 to imply sampling depth component and
   * ignoring stencil, which will blow up here if we try to do a
   * uint clamp in a float texel build...  And even if we had
                        /*
   * Border color was stored as uint, hence never need min clamp,
   * and only need max clamp if chan has less than 32 bits.
   */
   unsigned chan_size = format_desc->channel[chan].size;
   if (chan_size < 32) {
      max_clamp = lp_build_const_int_vec(gallivm, vec4_type,
      }
         } else if (chan_type == UTIL_FORMAT_TYPE_FIXED) {
            }
   /* mixed plain formats (or different pure size) */
   switch (format_desc->format) {
   case PIPE_FORMAT_B10G10R10A2_UINT:
   case PIPE_FORMAT_R10G10B10A2_UINT:
      {
      unsigned max10 = (1 << 10) - 1;
   max_clamp = lp_build_const_aos(gallivm, vec4_type, max10, max10,
      }
      case PIPE_FORMAT_R10SG10SB10SA2U_NORM:
      min_clamp = lp_build_const_aos(gallivm, vec4_type, -1.0F, -1.0F,
         max_clamp = vec4_bld.one;
      case PIPE_FORMAT_R8SG8SB8UX8U_NORM:
   case PIPE_FORMAT_R5SG5SB6U_NORM:
      min_clamp = lp_build_const_aos(gallivm, vec4_type, -1.0F, -1.0F,
         max_clamp = vec4_bld.one;
      default:
            } else {
      /* cannot figure this out from format description */
   if (format_desc->layout == UTIL_FORMAT_LAYOUT_S3TC) {
      /* s3tc formats are always unorm */
   min_clamp = vec4_bld.zero;
      } else if (format_desc->layout == UTIL_FORMAT_LAYOUT_RGTC ||
            format_desc->layout == UTIL_FORMAT_LAYOUT_ETC ||
   switch (format_desc->format) {
   case PIPE_FORMAT_RGTC1_UNORM:
   case PIPE_FORMAT_RGTC2_UNORM:
   case PIPE_FORMAT_LATC1_UNORM:
   case PIPE_FORMAT_LATC2_UNORM:
   case PIPE_FORMAT_ETC1_RGB8:
   case PIPE_FORMAT_BPTC_RGBA_UNORM:
   case PIPE_FORMAT_BPTC_SRGBA:
      min_clamp = vec4_bld.zero;
   max_clamp = vec4_bld.one;
      case PIPE_FORMAT_RGTC1_SNORM:
   case PIPE_FORMAT_RGTC2_SNORM:
   case PIPE_FORMAT_LATC1_SNORM:
   case PIPE_FORMAT_LATC2_SNORM:
      min_clamp = lp_build_const_vec(gallivm, vec4_type, -1.0F);
   max_clamp = vec4_bld.one;
      case PIPE_FORMAT_BPTC_RGB_FLOAT:
      /* not sure if we should clamp to max half float? */
      case PIPE_FORMAT_BPTC_RGB_UFLOAT:
      min_clamp = vec4_bld.zero;
      default:
      assert(0);
         } else if (format_desc->colorspace != UTIL_FORMAT_COLORSPACE_YUV){
      /*
   * all others from subsampled/other group, though we don't care
   * about yuv (and should not have any from zs here)
   */
   switch (format_desc->format) {
   case PIPE_FORMAT_R8G8_B8G8_UNORM:
   case PIPE_FORMAT_G8R8_G8B8_UNORM:
   case PIPE_FORMAT_G8R8_B8R8_UNORM:
   case PIPE_FORMAT_R8G8_R8B8_UNORM:
   case PIPE_FORMAT_G8B8_G8R8_UNORM:
   case PIPE_FORMAT_B8G8_R8G8_UNORM:
   case PIPE_FORMAT_R1_UNORM: /* doesn't make sense but ah well */
      min_clamp = vec4_bld.zero;
   max_clamp = vec4_bld.one;
      case PIPE_FORMAT_R8G8Bx_SNORM:
      min_clamp = lp_build_const_vec(gallivm, vec4_type, -1.0F);
   max_clamp = vec4_bld.one;
   break;
   /*
   * Note smallfloat formats usually don't need clamping
   * (they still have infinite range) however this is not
   * true for r11g11b10 and r9g9b9e5, which can't represent
   * negative numbers (and additionally r9g9b9e5 can't represent
   * very large numbers). d3d10 seems happy without clamping in
   * this case, but gl spec is pretty clear: "for floating
   * point and integer formats, border values are clamped to
   * the representable range of the format" so do that here.
      case PIPE_FORMAT_R11G11B10_FLOAT:
      min_clamp = vec4_bld.zero;
      case PIPE_FORMAT_R9G9B9E5_FLOAT:
      min_clamp = vec4_bld.zero;
   max_clamp = lp_build_const_vec(gallivm, vec4_type, MAX_RGB9E5);
      default:
      assert(0);
                     if (min_clamp) {
         }
   if (max_clamp) {
                     }
         /**
   * General texture sampling codegen.
   * This function handles texture sampling for all texture targets (1D,
   * 2D, 3D, cube) and all filtering modes.
   */
   static void
   lp_build_sample_general(struct lp_build_sample_context *bld,
                           unsigned sampler_unit,
   bool is_gather,
   const LLVMValueRef *coords,
   const LLVMValueRef *offsets,
   LLVMValueRef lod_positive,
   {
      LLVMBuilderRef builder = bld->gallivm->builder;
   const struct lp_static_sampler_state *sampler_state = bld->static_sampler_state;
   const unsigned mip_filter = sampler_state->min_mip_filter;
   const unsigned min_filter = sampler_state->min_img_filter;
   const unsigned mag_filter = sampler_state->mag_img_filter;
   LLVMValueRef texels[4];
            /* if we need border color, (potentially) clamp it now */
   if (lp_sampler_wrap_mode_uses_border_color(sampler_state->wrap_s,
                  (bld->dims > 1 &&
      lp_sampler_wrap_mode_uses_border_color(sampler_state->wrap_t,
            (bld->dims > 2 &&
      lp_sampler_wrap_mode_uses_border_color(sampler_state->wrap_r,
                           /*
   * Get/interpolate texture colors.
            for (chan = 0; chan < 4; ++chan) {
   texels[chan] = lp_build_alloca(bld->gallivm, bld->texel_bld.vec_type, "");
   lp_build_name(texels[chan], "sampler%u_texel_%c_var", sampler_unit, "xyzw"[chan]);
            if (sampler_state->aniso) {
      lp_build_sample_aniso(bld, PIPE_TEX_FILTER_NEAREST, mip_filter,
            } else if (min_filter == mag_filter) {
      /* no need to distinguish between minification and magnification */
   lp_build_sample_mipmap(bld, min_filter, mip_filter,
                        } else {
      /*
   * Could also get rid of the if-logic and always use mipmap_both, both
   * for the single lod and multi-lod case if nothing really uses this.
   */
   if (bld->num_lods == 1) {
      /* Emit conditional to choose min image filter or mag image filter
   * depending on the lod being > 0 or <= 0, respectively.
                  lod_positive = LLVMBuildTrunc(builder, lod_positive,
                  lp_build_if(&if_ctx, bld->gallivm, lod_positive);
   {
      /* Use the minification filter */
   lp_build_sample_mipmap(bld, min_filter, mip_filter, false,
                  }
   lp_build_else(&if_ctx);
   {
      /* Use the magnification filter */
   lp_build_sample_mipmap(bld, mag_filter, PIPE_TEX_MIPFILTER_NONE,
                        }
      } else {
      LLVMValueRef need_linear, linear_mask;
                  if (min_filter == PIPE_TEX_FILTER_LINEAR) {
      linear_mask = lod_positive;
      } else {
      linear_mask = lp_build_not(&bld->lodi_bld, lod_positive);
      }
   need_linear = lp_build_any_true_range(&bld->lodi_bld, bld->num_lods,
                  if (bld->num_lods != bld->coord_type.length) {
      linear_mask = lp_build_unpack_broadcast_aos_scalars(bld->gallivm,
                           lp_build_if(&if_ctx, bld->gallivm, need_linear);
   {
      /*
   * Do sampling with both filters simultaneously. This means using
   * a linear filter and doing some tricks (with weights) for the
   * pixels which need nearest filter.
   * Note that it's probably rare some pixels need nearest and some
   * linear filter but the fixups required for the nearest pixels
   * aren't all that complicated so just always run a combined path
   * if at least some pixels require linear.
   */
   lp_build_sample_mipmap_both(bld, linear_mask, mip_filter,
                        }
   lp_build_else(&if_ctx);
   {
      /*
   * All pixels require just nearest filtering, which is way
   * cheaper than linear, hence do a separate path for that.
   */
   lp_build_sample_mipmap(bld, PIPE_TEX_FILTER_NEAREST,
                        }
                  for (chan = 0; chan < 4; ++chan) {
   colors_out[chan] = LLVMBuildLoad2(builder, bld->texel_bld.vec_type, texels[chan], "");
   lp_build_name(colors_out[chan], "sampler%u_texel_%c", sampler_unit, "xyzw"[chan]);
      }
         /**
   * Texel fetch function.  In contrast to general sampling there is no
   * filtering, no coord minification, lod (if any) is always explicit uint,
   * coords are uints (in terms of texel units) directly to be applied to the
   * selected mip level (after adding texel offsets).  This function handles
   * texel fetch for all targets where texel fetch is supported (no cube maps,
   * but 1d, 2d, 3d are supported, arrays and buffers should be too).
   */
   static void
   lp_build_fetch_texel(struct lp_build_sample_context *bld,
                        unsigned texture_unit,
   LLVMValueRef ms_index,
      {
      struct lp_build_context *perquadi_bld = &bld->lodi_bld;
   struct lp_build_context *int_coord_bld = &bld->int_coord_bld;
   unsigned dims = bld->dims, chan;
   unsigned target = bld->static_texture_state->target;
   bool out_of_bound_ret_zero = true;
   LLVMValueRef size, ilevel;
   LLVMValueRef row_stride_vec = NULL, img_stride_vec = NULL;
   LLVMValueRef x = coords[0], y = coords[1], z = coords[2];
   LLVMValueRef width, height, depth, i, j;
   LLVMValueRef offset, out_of_bounds, out1;
   LLVMValueRef first_level = bld->dynamic_state->first_level(bld->gallivm,
                           if (explicit_lod && bld->static_texture_state->target != PIPE_BUFFER) {
      if (bld->num_mips != int_coord_bld->type.length) {
      ilevel = lp_build_pack_aos_scalars(bld->gallivm, int_coord_bld->type,
      } else {
         }
   LLVMValueRef last_level = bld->dynamic_state->last_level(bld->gallivm,
               first_level = lp_build_broadcast_scalar(&bld->leveli_bld, first_level);
   last_level = lp_build_broadcast_scalar(&bld->leveli_bld, last_level);
   lp_build_nearest_mip_level(bld,
                  } else {
      assert(bld->num_mips == 1);
   if (bld->static_texture_state->target != PIPE_BUFFER) {
         } else {
            }
   lp_build_mipmap_level_sizes(bld, ilevel,
               lp_build_extract_image_sizes(bld, &bld->int_size_bld, int_coord_bld->type,
            if (target == PIPE_TEXTURE_1D_ARRAY ||
      target == PIPE_TEXTURE_2D_ARRAY) {
   if (out_of_bound_ret_zero) {
      z = lp_build_layer_coord(bld, texture_unit, false, z, &out1);
      } else {
                     /* This is a lot like border sampling */
   if (offsets[0]) {
      /*
   * coords are really unsigned, offsets are signed, but I don't think
   * exceeding 31 bits is possible
   */
      }
   out1 = lp_build_cmp(int_coord_bld, PIPE_FUNC_LESS, x, int_coord_bld->zero);
   out_of_bounds = lp_build_or(int_coord_bld, out_of_bounds, out1);
   out1 = lp_build_cmp(int_coord_bld, PIPE_FUNC_GEQUAL, x, width);
            if (dims >= 2) {
      if (offsets[1]) {
         }
   out1 = lp_build_cmp(int_coord_bld, PIPE_FUNC_LESS, y, int_coord_bld->zero);
   out_of_bounds = lp_build_or(int_coord_bld, out_of_bounds, out1);
   out1 = lp_build_cmp(int_coord_bld, PIPE_FUNC_GEQUAL, y, height);
            if (dims >= 3) {
      if (offsets[2]) {
         }
   out1 = lp_build_cmp(int_coord_bld, PIPE_FUNC_LESS, z, int_coord_bld->zero);
   out_of_bounds = lp_build_or(int_coord_bld, out_of_bounds, out1);
   out1 = lp_build_cmp(int_coord_bld, PIPE_FUNC_GEQUAL, z, depth);
                  lp_build_sample_offset(int_coord_bld,
                        if (bld->static_texture_state->target != PIPE_BUFFER) {
      offset = lp_build_add(int_coord_bld, offset,
               if (bld->fetch_ms) {
      LLVMValueRef num_samples = bld->dynamic_state->num_samples(bld->gallivm,
                     LLVMValueRef sample_stride = bld->dynamic_state->sample_stride(bld->gallivm,
                           lp_build_sample_ms_offset(int_coord_bld, ms_index, num_samples, sample_stride,
                        lp_build_fetch_rgba_soa(bld->gallivm,
                           bld->format_desc,
            if (out_of_bound_ret_zero) {
      /*
   * Only needed for ARB_robust_buffer_access_behavior and d3d10.
   * Could use min/max above instead of out-of-bounds comparisons
   * if we don't care about the result returned for out-of-bounds.
   */
   LLVMValueRef oob[4] = {
      bld->texel_bld.zero,
   bld->texel_bld.zero,
   bld->texel_bld.zero,
      };
   lp_build_format_swizzle_soa(bld->format_desc, &bld->texel_bld, oob, oob);
   for (chan = 0; chan < 4; chan++) {
      colors_out[chan] = lp_build_select(&bld->texel_bld, out_of_bounds,
            }
         /**
   * Just set texels to white instead of actually sampling the texture.
   * For debugging.
   */
   void
   lp_build_sample_nop(struct gallivm_state *gallivm,
                     {
      LLVMValueRef one = lp_build_one(gallivm, type);
   for (unsigned chan = 0; chan < 4; chan++) {
            }
         struct lp_type
   lp_build_texel_type(struct lp_type texel_type,
         {
      /* always using the first channel hopefully should be safe,
   * if not things WILL break in other places anyway.
   */
   if (format_desc->colorspace == UTIL_FORMAT_COLORSPACE_RGB &&
      format_desc->channel[0].pure_integer) {
   if (format_desc->channel[0].type == UTIL_FORMAT_TYPE_SIGNED) {
         } else if (format_desc->channel[0].type == UTIL_FORMAT_TYPE_UNSIGNED) {
            } else if (util_format_has_stencil(format_desc) &&
      !util_format_has_depth(format_desc)) {
   /* for stencil only formats, sample stencil (uint) */
      }
      }
         /**
   * Build the actual texture sampling code.
   * 'texel' will return a vector of four LLVMValueRefs corresponding to
   * R, G, B, A.
   * \param type  vector float type to use for coords, etc.
   * \param sample_key
   * \param derivs  partial derivatives of (s,t,r,q) with respect to x and y
   */
   void
   lp_build_sample_soa_code(struct gallivm_state *gallivm,
                           const struct lp_static_texture_state *static_texture_state,
   const struct lp_static_sampler_state *static_sampler_state,
   struct lp_sampler_dynamic_state *dynamic_state,
   struct lp_type type,
   unsigned sample_key,
   unsigned texture_index,
   unsigned sampler_index,
   LLVMTypeRef resources_type,
   LLVMValueRef resources_ptr,
   LLVMTypeRef thread_data_type,
   LLVMValueRef thread_data_ptr,
   const LLVMValueRef *coords,
   const LLVMValueRef *offsets,
   const struct lp_derivatives *derivs, /* optional */
   {
      assert(static_texture_state);
   assert(static_texture_state->format < PIPE_FORMAT_COUNT);
            const enum pipe_texture_target target = static_texture_state->target;
   const unsigned dims = texture_dims(target);
   const unsigned num_quads = type.length / 4;
   struct lp_build_sample_context bld;
   struct lp_static_sampler_state derived_sampler_state = *static_sampler_state;
   LLVMTypeRef i32t = LLVMInt32TypeInContext(gallivm->context);
   LLVMBuilderRef builder = gallivm->builder;
            if (0) {
      enum pipe_format fmt = static_texture_state->format;
               const enum lp_sampler_lod_property lod_property =
      (sample_key & LP_SAMPLER_LOD_PROPERTY_MASK) >>
      const enum lp_sampler_lod_control lod_control =
      (sample_key & LP_SAMPLER_LOD_CONTROL_MASK) >>
      const enum lp_sampler_op_type op_type =
      (sample_key & LP_SAMPLER_OP_TYPE_MASK) >>
         const bool fetch_ms = !!(sample_key & LP_SAMPLER_FETCH_MS);
   const bool op_is_tex = op_type == LP_SAMPLER_OP_TEXTURE;
   const bool op_is_lodq = op_type == LP_SAMPLER_OP_LODQ;
            LLVMValueRef lod_bias = NULL;
   LLVMValueRef explicit_lod = NULL;
   if (lod_control == LP_SAMPLER_LOD_BIAS) {
      lod_bias = lod;
   assert(lod);
      } else if (lod_control == LP_SAMPLER_LOD_EXPLICIT) {
      explicit_lod = lod;
   assert(lod);
      } else if (lod_control == LP_SAMPLER_LOD_DERIVATIVES) {
      assert(derivs);
      } else {
      assert(derivs == NULL);
               if (static_texture_state->format == PIPE_FORMAT_NONE) {
      /*
   * If there's nothing bound, format is NONE, and we must return
   * all zero as mandated by d3d10 in this case.
   */
   LLVMValueRef zero = lp_build_zero(gallivm, type);
   for (unsigned chan = 0; chan < 4; chan++) {
         }
                        /* Setup our build context */
   memset(&bld, 0, sizeof bld);
   bld.gallivm = gallivm;
   bld.resources_type = resources_type;
   bld.resources_ptr = resources_ptr;
   bld.aniso_filter_table = aniso_filter_table;
   bld.static_sampler_state = &derived_sampler_state;
   bld.static_texture_state = static_texture_state;
   bld.dynamic_state = dynamic_state;
   bld.format_desc = util_format_description(static_texture_state->format);
                     if (gallivm_perf & GALLIVM_PERF_NO_QUAD_LOD || op_is_lodq) {
         }
   if (!(gallivm_perf & GALLIVM_PERF_RHO_APPROX) || op_is_lodq) {
         }
   if (!(gallivm_perf & GALLIVM_PERF_BRILINEAR) || op_is_lodq || lod_bias || explicit_lod) {
                           bld.float_type = lp_type_float(32);
   bld.int_type = lp_type_int(32);
   bld.coord_type = type;
   bld.int_coord_type = lp_int_type(type);
   bld.float_size_in_type = lp_type_float(32);
   bld.float_size_in_type.length = dims > 1 ? 4 : 1;
                     if (!static_texture_state->level_zero_only ||
      !static_sampler_state->max_lod_pos || op_is_lodq) {
      } else {
                  if (op_is_gather) {
      /*
   * gather4 is exactly like GL_LINEAR filtering but in the end skipping
   * the actual filtering. Using mostly the same paths, so cube face
   * selection, coord wrapping etc. all naturally uses the same code.
   */
   derived_sampler_state.min_mip_filter = PIPE_TEX_MIPFILTER_NONE;
   derived_sampler_state.min_img_filter = PIPE_TEX_FILTER_LINEAR;
               const enum pipe_tex_mipfilter mip_filter =
            if (static_texture_state->target == PIPE_TEXTURE_CUBE ||
      static_texture_state->target == PIPE_TEXTURE_CUBE_ARRAY) {
   /*
   * Seamless filtering ignores wrap modes.
   * Setting to CLAMP_TO_EDGE is correct for nearest filtering, for
   * bilinear it's not correct but way better than using for instance
   * repeat.  Note we even set this for non-seamless. Technically GL
   * allows any wrap mode, which made sense when supporting true borders
   * (can get seamless effect with border and CLAMP_TO_BORDER), but
   * gallium doesn't support borders and d3d9 requires wrap modes to be
   * ignored and it's a pain to fix up the sampler state (as it makes it
   * texture dependent).
   */
   derived_sampler_state.wrap_s = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
               /*
   * We could force CLAMP to CLAMP_TO_EDGE here if min/mag filter is nearest,
   * so AoS path could be used. Not sure it's worth the trouble...
   */
   const enum pipe_tex_filter min_img_filter =
         const enum pipe_tex_filter mag_img_filter =
            /*
   * This is all a bit complicated different paths are chosen for performance
   * reasons.
   * Essentially, there can be 1 lod per element, 1 lod per quad or 1 lod for
   * everything (the last two options are equivalent for 4-wide case).
   * If there's per-quad lod but we split to 4-wide so we can use AoS, per-quad
   * lod is calculated then the lod value extracted afterwards so making this
   * case basically the same as far as lod handling is concerned for the
   * further sample/filter code as the 1 lod for everything case.
   * Different lod handling mostly shows up when building mipmap sizes
   * (lp_build_mipmap_level_sizes() and friends) and also in filtering
   * (getting the fractional part of the lod to the right texels).
            /*
   * There are other situations where at least the multiple int lods could be
   * avoided like min and max lod being equal.
   */
            if ((mip_filter != PIPE_TEX_MIPFILTER_NONE && op_is_tex &&
         (static_texture_state->target == PIPE_TEXTURE_CUBE ||
      op_is_lodq) {
   /*
   * special case for using per-pixel lod even for implicit lod,
   * which is generally never required (ok by APIs) except to please
   * some (somewhat broken imho) tests (because per-pixel face selection
   * can cause derivatives to be different for pixels outside the primitive
   * due to the major axis division even if pre-project derivatives are
   * looking normal).
   * For lodq, we do it to simply avoid scalar pack / unpack (albeit for
   * cube maps we do indeed get per-pixel lod values).
   */
   bld.num_mips = type.length;
      } else if (lod_property == LP_SAMPLER_LOD_PER_ELEMENT ||
      (explicit_lod || lod_bias || derivs)) {
   if ((!op_is_tex && target != PIPE_BUFFER) ||
      (op_is_tex && mip_filter != PIPE_TEX_MIPFILTER_NONE)) {
   bld.num_mips = type.length;
      } else if (op_is_tex && min_img_filter != mag_img_filter) {
      bld.num_mips = 1;
         }
   /* TODO: for true scalar_lod should only use 1 lod value */
   else if ((!op_is_tex && explicit_lod && target != PIPE_BUFFER) ||
            bld.num_mips = num_quads;
      } else if (op_is_tex && min_img_filter != mag_img_filter) {
      bld.num_mips = 1;
               bld.fetch_ms = fetch_ms;
   if (op_is_gather)
         bld.lodf_type = type;
   /* we want native vector size to be able to use our intrinsics */
   if (bld.num_lods != type.length) {
      /* TODO: this currently always has to be per-quad or per-element */
      }
   bld.lodi_type = lp_int_type(bld.lodf_type);
   bld.levelf_type = bld.lodf_type;
   if (bld.num_mips == 1) {
         }
   bld.leveli_type = lp_int_type(bld.levelf_type);
            /* Note: size vectors may not be native. They contain minified w/h/d/_
   * values, with per-element lod that is w0/h0/d0/_/w1/h1/d1_/... so up to
   * 8x4f32
   */
   if (bld.num_mips > 1) {
      bld.float_size_type.length = bld.num_mips == type.length ?
            }
            lp_build_context_init(&bld.float_bld, gallivm, bld.float_type);
   lp_build_context_init(&bld.float_vec_bld, gallivm, type);
   lp_build_context_init(&bld.int_bld, gallivm, bld.int_type);
   lp_build_context_init(&bld.coord_bld, gallivm, bld.coord_type);
   lp_build_context_init(&bld.int_coord_bld, gallivm, bld.int_coord_type);
   lp_build_context_init(&bld.int_size_in_bld, gallivm, bld.int_size_in_type);
   lp_build_context_init(&bld.float_size_in_bld, gallivm, bld.float_size_in_type);
   lp_build_context_init(&bld.int_size_bld, gallivm, bld.int_size_type);
   lp_build_context_init(&bld.float_size_bld, gallivm, bld.float_size_type);
   lp_build_context_init(&bld.texel_bld, gallivm, bld.texel_type);
   lp_build_context_init(&bld.levelf_bld, gallivm, bld.levelf_type);
   lp_build_context_init(&bld.leveli_bld, gallivm, bld.leveli_type);
   lp_build_context_init(&bld.lodf_bld, gallivm, bld.lodf_type);
            /* Get the dynamic state */
   LLVMValueRef tex_width = dynamic_state->width(gallivm, resources_type,
               bld.row_stride_array = dynamic_state->row_stride(gallivm, resources_type,
               bld.img_stride_array = dynamic_state->img_stride(gallivm, resources_type,
               bld.base_ptr = dynamic_state->base_ptr(gallivm, resources_type,
         bld.mip_offsets = dynamic_state->mip_offsets(gallivm, resources_type,
                           if (dynamic_state->cache_ptr && thread_data_ptr) {
      bld.cache = dynamic_state->cache_ptr(gallivm, thread_data_type,
               uint32_t res_bw = res_format_desc->block.width;
   uint32_t res_bh = res_format_desc->block.height;
   uint32_t bw = bld.format_desc->block.width;
            /* only scale if the blocksizes are different. */
   if (res_bw == bw)
         if (res_bh == bh)
            /* width, height, depth as single int vector */
   if (dims <= 1) {
      bld.int_size = tex_width;
   bld.int_tex_blocksize = LLVMConstInt(i32t, res_bw, 0);
   bld.int_tex_blocksize_log2 = LLVMConstInt(i32t, util_logbase2(res_bw), 0);
      } else {
      bld.int_size = LLVMBuildInsertElement(builder, bld.int_size_in_bld.undef,
               bld.int_tex_blocksize = LLVMBuildInsertElement(builder, bld.int_size_in_bld.undef,
               bld.int_tex_blocksize_log2 = LLVMBuildInsertElement(builder, bld.int_size_in_bld.undef,
               bld.int_view_blocksize = LLVMBuildInsertElement(builder, bld.int_size_in_bld.undef,
               if (dims >= 2) {
      LLVMValueRef tex_height =
      dynamic_state->height(gallivm, resources_type,
      bld.int_size = LLVMBuildInsertElement(builder, bld.int_size,
               bld.int_tex_blocksize = LLVMBuildInsertElement(builder, bld.int_tex_blocksize,
               bld.int_tex_blocksize_log2 = LLVMBuildInsertElement(builder, bld.int_tex_blocksize_log2,
               bld.int_view_blocksize = LLVMBuildInsertElement(builder, bld.int_view_blocksize,
               if (dims >= 3) {
      LLVMValueRef tex_depth =
      dynamic_state->depth(gallivm, resources_type, resources_ptr,
      bld.int_size = LLVMBuildInsertElement(builder, bld.int_size,
               bld.int_tex_blocksize = LLVMBuildInsertElement(builder, bld.int_tex_blocksize,
               bld.int_tex_blocksize_log2 = LLVMBuildInsertElement(builder, bld.int_tex_blocksize_log2,
               bld.int_view_blocksize = LLVMBuildInsertElement(builder, bld.int_view_blocksize,
                           LLVMValueRef newcoords[5];
   for (unsigned i = 0; i < 5; i++) {
                  if (util_format_is_pure_integer(static_texture_state->format) &&
      !util_format_has_depth(bld.format_desc) && op_is_tex &&
   (static_sampler_state->min_mip_filter == PIPE_TEX_MIPFILTER_LINEAR ||
   static_sampler_state->min_img_filter == PIPE_TEX_FILTER_LINEAR ||
   static_sampler_state->mag_img_filter == PIPE_TEX_FILTER_LINEAR)) {
   /*
   * Bail if impossible filtering is specified (the awkard additional
   * depth check is because it is legal in gallium to have things like
   * S8Z24 here which would say it's pure int despite such formats should
   * sample the depth component).
   * In GL such filters make the texture incomplete, this makes it robust
   * against gallium frontends which set this up regardless (we'd crash in
   * the lerp later otherwise).
   * At least in some apis it may be legal to use such filters with lod
   * queries and/or gather (at least for gather d3d10 says only the wrap
   * bits are really used hence filter bits are likely simply ignored).
   * For fetch, we don't get valid samplers either way here.
   */
   LLVMValueRef zero = lp_build_zero(gallivm, type);
   for (unsigned chan = 0; chan < 4; chan++) {
         }
               if (0) {
      /* For debug: no-op texture sampling */
   lp_build_sample_nop(gallivm,
                  } else if (op_type == LP_SAMPLER_OP_FETCH) {
      lp_build_fetch_texel(&bld, texture_index, ms_index, newcoords,
      } else {
      LLVMValueRef lod_fpart = NULL, lod_positive = NULL;
   LLVMValueRef ilevel0 = NULL, ilevel1 = NULL, lod = NULL;
   bool use_aos = util_format_fits_8unorm(bld.format_desc) &&
            op_is_tex &&
   /* not sure this is strictly needed or simply impossible */
               use_aos &= bld.num_lods <= num_quads ||
                  if (gallivm_perf & GALLIVM_PERF_NO_AOS_SAMPLING) {
                  if (dims > 1) {
      use_aos &= lp_is_simple_wrap_mode(derived_sampler_state.wrap_t);
   if (dims > 2) {
            }
   if ((static_texture_state->target == PIPE_TEXTURE_CUBE ||
      static_texture_state->target == PIPE_TEXTURE_CUBE_ARRAY) &&
   derived_sampler_state.seamless_cube_map &&
   (derived_sampler_state.min_img_filter == PIPE_TEX_FILTER_LINEAR ||
   derived_sampler_state.mag_img_filter == PIPE_TEX_FILTER_LINEAR)) {
   /* theoretically possible with AoS filtering but not implemented (complex!) */
               if ((gallivm_debug & GALLIVM_DEBUG_PERF) &&
      !use_aos && util_format_fits_8unorm(bld.format_desc)) {
   debug_printf("%s: using floating point linear filtering for %s\n",
         debug_printf("  min_img %d  mag_img %d  mip %d  target %d  seamless %d"
               "  wraps %d  wrapt %d  wrapr %d\n",
   derived_sampler_state.min_img_filter,
   derived_sampler_state.mag_img_filter,
   derived_sampler_state.min_mip_filter,
   static_texture_state->target,
   derived_sampler_state.seamless_cube_map,
               lp_build_sample_common(&bld, op_is_lodq, texture_index, sampler_index,
                        if (op_is_lodq) {
      texel_out[0] = lod_fpart;
   texel_out[1] = lod;
   texel_out[2] = texel_out[3] = bld.coord_bld.zero;
               if (use_aos && static_texture_state->target == PIPE_TEXTURE_CUBE_ARRAY) {
      /* The aos path doesn't do seamless filtering so simply add cube layer
   * to face now.
   */
               /*
   * we only try 8-wide sampling with soa or if we have AVX2
   * as it appears to be a loss with just AVX)
   */
   if (num_quads == 1 || !use_aos ||
      (util_get_cpu_caps()->has_avx2 &&
   (bld.num_lods == 1 ||
         if (use_aos) {
      /* do sampling/filtering with fixed pt arithmetic */
   lp_build_sample_aos(&bld,
                     newcoords[0], newcoords[1],
      } else {
      lp_build_sample_general(&bld, sampler_index,
                                 } else {
      struct lp_build_sample_context bld4;
   struct lp_type type4 = type;
                           /* Setup our build context */
   memset(&bld4, 0, sizeof bld4);
   bld4.no_quad_lod = bld.no_quad_lod;
   bld4.no_rho_approx = bld.no_rho_approx;
   bld4.no_brilinear = bld.no_brilinear;
   bld4.gallivm = bld.gallivm;
   bld4.resources_type = bld.resources_type;
   bld4.resources_ptr = bld.resources_ptr;
   bld4.aniso_filter_table = aniso_filter_table;
   bld4.static_texture_state = bld.static_texture_state;
   bld4.static_sampler_state = bld.static_sampler_state;
   bld4.dynamic_state = bld.dynamic_state;
   bld4.format_desc = bld.format_desc;
   bld4.dims = bld.dims;
   bld4.row_stride_type = bld.row_stride_type;
   bld4.row_stride_array = bld.row_stride_array;
   bld4.img_stride_type = bld.img_stride_type;
   bld4.img_stride_array = bld.img_stride_array;
   bld4.base_ptr = bld.base_ptr;
   bld4.mip_offsets_type = bld.mip_offsets_type;
   bld4.mip_offsets = bld.mip_offsets;
   bld4.int_size = bld.int_size;
   bld4.int_tex_blocksize = bld.int_tex_blocksize;
   bld4.int_tex_blocksize_log2 = bld.int_tex_blocksize_log2;
                           bld4.float_type = lp_type_float(32);
   bld4.int_type = lp_type_int(32);
   bld4.coord_type = type4;
   bld4.int_coord_type = lp_int_type(type4);
   bld4.float_size_in_type = lp_type_float(32);
   bld4.float_size_in_type.length = dims > 1 ? 4 : 1;
   bld4.int_size_in_type = lp_int_type(bld4.float_size_in_type);
                  bld4.num_mips = bld4.num_lods = 1;
   if (bld4.no_quad_lod && bld4.no_rho_approx &&
      (static_texture_state->target == PIPE_TEXTURE_CUBE ||
   static_texture_state->target == PIPE_TEXTURE_CUBE_ARRAY) &&
   (op_is_tex && mip_filter != PIPE_TEX_MIPFILTER_NONE)) {
   bld4.num_mips = type4.length;
      }
   if (lod_property == LP_SAMPLER_LOD_PER_ELEMENT &&
      (explicit_lod || lod_bias || derivs)) {
   if ((!op_is_tex && target != PIPE_BUFFER) ||
      (op_is_tex && mip_filter != PIPE_TEX_MIPFILTER_NONE)) {
   bld4.num_mips = type4.length;
      } else if (op_is_tex && min_img_filter != mag_img_filter) {
      bld4.num_mips = 1;
                  /* we want native vector size to be able to use our intrinsics */
   bld4.lodf_type = type4;
   if (bld4.num_lods != type4.length) {
         }
   bld4.lodi_type = lp_int_type(bld4.lodf_type);
   bld4.levelf_type = type4;
   if (bld4.num_mips != type4.length) {
         }
   bld4.leveli_type = lp_int_type(bld4.levelf_type);
   bld4.float_size_type = bld4.float_size_in_type;
   if (bld4.num_mips > 1) {
      bld4.float_size_type.length = bld4.num_mips == type4.length ?
                           lp_build_context_init(&bld4.float_bld, gallivm, bld4.float_type);
   lp_build_context_init(&bld4.float_vec_bld, gallivm, type4);
   lp_build_context_init(&bld4.int_bld, gallivm, bld4.int_type);
   lp_build_context_init(&bld4.coord_bld, gallivm, bld4.coord_type);
   lp_build_context_init(&bld4.int_coord_bld, gallivm, bld4.int_coord_type);
   lp_build_context_init(&bld4.int_size_in_bld, gallivm, bld4.int_size_in_type);
   lp_build_context_init(&bld4.float_size_in_bld, gallivm, bld4.float_size_in_type);
   lp_build_context_init(&bld4.int_size_bld, gallivm, bld4.int_size_type);
   lp_build_context_init(&bld4.float_size_bld, gallivm, bld4.float_size_type);
   lp_build_context_init(&bld4.texel_bld, gallivm, bld4.texel_type);
   lp_build_context_init(&bld4.levelf_bld, gallivm, bld4.levelf_type);
   lp_build_context_init(&bld4.leveli_bld, gallivm, bld4.leveli_type);
                  for (unsigned i = 0; i < num_quads; i++) {
      LLVMValueRef s4, t4, r4;
   LLVMValueRef lod_positive4, lod_fpart4 = NULL;
   LLVMValueRef ilevel04, ilevel14 = NULL;
                  s4 = lp_build_extract_range(gallivm, newcoords[0], 4*i, 4);
                  if (offsets[0]) {
      offsets4[0] = lp_build_extract_range(gallivm, offsets[0], 4*i, 4);
   if (dims > 1) {
      offsets4[1] = lp_build_extract_range(gallivm, offsets[1], 4*i, 4);
   if (dims > 2) {
               }
   lod_positive4 = lp_build_extract_range(gallivm, lod_positive, num_lods * i, num_lods);
   ilevel04 = bld.num_mips == 1 ? ilevel0 :
         if (mip_filter == PIPE_TEX_MIPFILTER_LINEAR) {
                        if (use_aos) {
      /* do sampling/filtering with fixed pt arithmetic */
   lp_build_sample_aos(&bld4,
                        } else {
      /* this path is currently unreachable and hence might break easily... */
   LLVMValueRef newcoords4[5];
   newcoords4[0] = s4;
   newcoords4[1] = t4;
                        lp_build_sample_general(&bld4, sampler_index,
                              }
   for (unsigned j = 0; j < 4; j++) {
                     for (unsigned j = 0; j < 4; j++) {
                        if (target != PIPE_BUFFER && op_type != LP_SAMPLER_OP_GATHER) {
                  /*
   * texel type can be a (32bit) int/uint (for pure int formats only),
   * however we are expected to always return floats (storage is untyped).
   */
   if (!bld.texel_type.floating) {
      unsigned chan;
   for (chan = 0; chan < 4; chan++) {
      texel_out[chan] = LLVMBuildBitCast(builder, texel_out[chan],
            }
         #define USE_TEX_FUNC_CALL 1
      static inline void
   get_target_info(enum pipe_texture_target target,
               {
      unsigned dims = texture_dims(target);
   *num_coords = dims;
   *num_offsets = dims;
   *num_derivs = (target == PIPE_TEXTURE_CUBE ||
         *layer = has_layer_coord(target) ? 2: 0;
   if (target == PIPE_TEXTURE_CUBE_ARRAY) {
      /*
   * dims doesn't include r coord for cubes - this is handled
   * by layer instead, but need to fix up for cube arrays...
   */
   *layer = 3;
         }
         /**
   * Generate the function body for a texture sampling function.
   */
   static void
   lp_build_sample_gen_func(struct gallivm_state *gallivm,
                           const struct lp_static_texture_state *static_texture_state,
   const struct lp_static_sampler_state *static_sampler_state,
   struct lp_sampler_dynamic_state *dynamic_state,
   struct lp_type type,
   LLVMTypeRef resources_type,
   LLVMTypeRef thread_data_type,
   unsigned texture_index,
   unsigned sampler_index,
   {
      LLVMBuilderRef old_builder;
   LLVMBasicBlockRef block;
   LLVMValueRef coords[5];
   LLVMValueRef offsets[3] = { NULL };
   LLVMValueRef lod = NULL;
   LLVMValueRef ms_index = NULL;
   LLVMValueRef resources_ptr;
   LLVMValueRef thread_data_ptr = NULL;
   LLVMValueRef aniso_filter_table = NULL;
   LLVMValueRef texel_out[4];
   struct lp_derivatives derivs;
   struct lp_derivatives *deriv_ptr = NULL;
   unsigned num_param = 0;
   unsigned num_coords, num_derivs, num_offsets, layer;
            const enum lp_sampler_lod_control lod_control =
      (sample_key & LP_SAMPLER_LOD_CONTROL_MASK)
         const enum lp_sampler_op_type op_type =
            get_target_info(static_texture_state->target,
            /* lod query doesn't take a layer */
   if (layer && op_type == LP_SAMPLER_OP_LODQ)
            if (dynamic_state->cache_ptr) {
      const struct util_format_description *format_desc;
   format_desc = util_format_description(static_texture_state->format);
   if (format_desc->layout == UTIL_FORMAT_LAYOUT_S3TC) {
                     /* "unpack" arguments */
   resources_ptr = LLVMGetParam(function, num_param++);
   if (has_aniso_filter_table)
         if (need_cache) {
         }
   for (unsigned i = 0; i < num_coords; i++) {
         }
   for (unsigned i = num_coords; i < 5; i++) {
      /* This is rather unfortunate... */
      }
   if (layer) {
         }
   if (sample_key & LP_SAMPLER_SHADOW) {
         }
   if (sample_key & LP_SAMPLER_FETCH_MS) {
         }
   if (sample_key & LP_SAMPLER_OFFSETS) {
      for (unsigned i = 0; i < num_offsets; i++) {
            }
   if (lod_control == LP_SAMPLER_LOD_BIAS ||
      lod_control == LP_SAMPLER_LOD_EXPLICIT) {
      } else if (lod_control == LP_SAMPLER_LOD_DERIVATIVES) {
      for (unsigned i = 0; i < num_derivs; i++) {
      derivs.ddx[i] = LLVMGetParam(function, num_param++);
      }
                        /*
   * Function body
            old_builder = gallivm->builder;
   block = LLVMAppendBasicBlockInContext(gallivm->context, function, "entry");
   gallivm->builder = LLVMCreateBuilderInContext(gallivm->context);
            lp_build_sample_soa_code(gallivm,
                           static_texture_state,
   static_sampler_state,
   dynamic_state,
   type,
   sample_key,
   texture_index,
   sampler_index,
   resources_type,
   resources_ptr,
   thread_data_type,
   thread_data_ptr,
   coords,
   offsets,
                     LLVMDisposeBuilder(gallivm->builder);
               }
         /**
   * Call the matching function for texture sampling.
   * If there's no match, generate a new one.
   */
   static void
   lp_build_sample_soa_func(struct gallivm_state *gallivm,
                           const struct lp_static_texture_state *static_texture_state,
   const struct lp_static_sampler_state *static_sampler_state,
   {
      LLVMBuilderRef builder = gallivm->builder;
   LLVMModuleRef module = LLVMGetGlobalParent(LLVMGetBasicBlockParent(
         LLVMValueRef args[LP_MAX_TEX_FUNC_ARGS];
   unsigned sample_key = params->sample_key;
   const LLVMValueRef *coords = params->coords;
   const LLVMValueRef *offsets = params->offsets;
            const enum lp_sampler_lod_control lod_control =
      (sample_key & LP_SAMPLER_LOD_CONTROL_MASK) >>
         const enum lp_sampler_op_type op_type =
            unsigned num_coords, num_derivs, num_offsets, layer;
   get_target_info(static_texture_state->target,
            /* lod query doesn't take a layer */
   if (layer && op_type == LP_SAMPLER_OP_LODQ)
            bool need_cache = false;
   if (dynamic_state->cache_ptr) {
      const struct util_format_description *format_desc;
   format_desc = util_format_description(static_texture_state->format);
   if (format_desc->layout == UTIL_FORMAT_LAYOUT_S3TC) {
                     /*
   * texture function matches are found by name.
   * Thus the name has to include both the texture and sampler unit
   * (which covers all static state) plus the actual texture function
   * (including things like offsets, shadow coord, lod control).
   * Additionally lod_property has to be included too.
   */
   char func_name[64];
   snprintf(func_name, sizeof(func_name), "texfunc_res_%d_sam_%d_%x",
            LLVMValueRef function = LLVMGetNamedFunction(module, func_name);
   LLVMTypeRef arg_types[LP_MAX_TEX_FUNC_ARGS];
   LLVMTypeRef ret_type;
   LLVMTypeRef val_type[4];
            /*
   * Generate the function prototype.
            arg_types[num_param++] = LLVMTypeOf(params->resources_ptr);
   if (params->aniso_filter_table)
         if (need_cache) {
         }
   for (unsigned i = 0; i < num_coords; i++) {
      arg_types[num_param++] = LLVMTypeOf(coords[0]);
      }
   if (layer) {
      arg_types[num_param++] = LLVMTypeOf(coords[layer]);
      }
   if (sample_key & LP_SAMPLER_SHADOW) {
         }
   if (sample_key & LP_SAMPLER_FETCH_MS) {
         }
   if (sample_key & LP_SAMPLER_OFFSETS) {
      for (unsigned i = 0; i < num_offsets; i++) {
      arg_types[num_param++] = LLVMTypeOf(offsets[0]);
         }
   if (lod_control == LP_SAMPLER_LOD_BIAS ||
      lod_control == LP_SAMPLER_LOD_EXPLICIT) {
      } else if (lod_control == LP_SAMPLER_LOD_DERIVATIVES) {
      for (unsigned i = 0; i < num_derivs; i++) {
      arg_types[num_param++] = LLVMTypeOf(derivs->ddx[i]);
   arg_types[num_param++] = LLVMTypeOf(derivs->ddy[i]);
   assert(LLVMTypeOf(derivs->ddx[0]) == LLVMTypeOf(derivs->ddx[i]));
                  val_type[0] = val_type[1] = val_type[2] = val_type[3] =
         ret_type = LLVMStructTypeInContext(gallivm->context, val_type, 4, 0);
            if (!function) {
               for (unsigned i = 0; i < num_param; ++i) {
                                 LLVMSetFunctionCallConv(function, LLVMFastCallConv);
            lp_build_sample_gen_func(gallivm,
                           static_texture_state,
   static_sampler_state,
   dynamic_state,
   params->type,
   params->resources_type,
   params->thread_data_type,
   texture_index,
               unsigned num_args = 0;
   args[num_args++] = params->resources_ptr;
   if (params->aniso_filter_table)
         if (need_cache) {
         }
   for (unsigned i = 0; i < num_coords; i++) {
         }
   if (layer) {
         }
   if (sample_key & LP_SAMPLER_SHADOW) {
         }
   if (sample_key & LP_SAMPLER_FETCH_MS) {
         }
   if (sample_key & LP_SAMPLER_OFFSETS) {
      for (unsigned i = 0; i < num_offsets; i++) {
            }
   if (lod_control == LP_SAMPLER_LOD_BIAS ||
      lod_control == LP_SAMPLER_LOD_EXPLICIT) {
      } else if (lod_control == LP_SAMPLER_LOD_DERIVATIVES) {
      for (unsigned i = 0; i < num_derivs; i++) {
      args[num_args++] = derivs->ddx[i];
                           *tex_ret = LLVMBuildCall2(builder, function_type, function, args, num_args, "");
   LLVMBasicBlockRef bb = LLVMGetInsertBlock(builder);
   LLVMValueRef inst = LLVMGetLastInstruction(bb);
      }
         /**
   * Build texture sampling code.
   * Either via a function call or inline it directly.
   */
   void
   lp_build_sample_soa(const struct lp_static_texture_state *static_texture_state,
                     const struct lp_static_sampler_state *static_sampler_state,
   {
               /*
   * Do not use a function call if the sampling is "simple enough".
   * We define this by
   * a) format
   * b) no mips (either one level only or no mip filter)
   * No mips will definitely make the code smaller, though
   * the format requirement is a bit iffy - there's some (SoA) formats
   * which definitely generate less code. This does happen to catch
   * some important cases though which are hurt quite a bit by using
   * a call (though not really because of the call overhead but because
   * they are reusing the same texture unit with some of the same
   * parameters).
   * Ideally we'd let llvm recognize this stuff by doing IPO passes.
            if (USE_TEX_FUNC_CALL) {
      const struct util_format_description *format_desc =
         const bool simple_format =
      (util_format_is_rgba8_variant(format_desc) &&
      const enum lp_sampler_op_type op_type =
      (params->sample_key & LP_SAMPLER_OP_TYPE_MASK) >>
      const bool simple_tex =
      op_type != LP_SAMPLER_OP_TEXTURE ||
   ((static_sampler_state->min_mip_filter == PIPE_TEX_MIPFILTER_NONE ||
                           if (use_tex_func) {
      LLVMValueRef tex_ret;
   lp_build_sample_soa_func(gallivm,
                                    for (unsigned i = 0; i < 4; i++) {
      params->texel[i] =
         } else {
      lp_build_sample_soa_code(gallivm,
                           static_texture_state,
   static_sampler_state,
   dynamic_state,
   params->type,
   params->sample_key,
   params->texture_index,
   params->sampler_index,
   params->resources_type,
   params->resources_ptr,
   params->thread_data_type,
   params->thread_data_ptr,
   params->coords,
   params->offsets,
         }
         void
   lp_build_size_query_soa(struct gallivm_state *gallivm,
                     {
      LLVMValueRef first_level = NULL;
   const unsigned num_lods = 1;
   LLVMTypeRef resources_type = params->resources_type;
   LLVMValueRef resources_ptr = params->resources_ptr;
   const unsigned texture_unit = params->texture_unit;
   const enum pipe_texture_target target = params->target;
   LLVMValueRef texture_unit_offset = params->texture_unit_offset;
   const struct util_format_description *format_desc =
         const struct util_format_description *res_format_desc =
            if (static_state->format == PIPE_FORMAT_NONE) {
      /*
   * If there's nothing bound, format is NONE, and we must return
   * all zero as mandated by d3d10 in this case.
   */
   LLVMValueRef zero = lp_build_const_vec(gallivm, params->int_type, 0.0F);
   for (unsigned chan = 0; chan < 4; chan++) {
         }
               /*
   * Do some sanity verification about bound texture and shader dcl target.
   * Not entirely sure what's possible but assume array/non-array
   * always compatible (probably not ok for OpenGL but d3d10 has no
   * distinction of arrays at the resource level).
   * Everything else looks bogus (though not entirely sure about rect/2d).
   * Currently disabled because it causes assertion failures if there's
   * nothing bound (or rather a dummy texture, not that this case would
   * return the right values).
   */
   if (0 && static_state->target != target) {
      if (static_state->target == PIPE_TEXTURE_1D)
         else if (static_state->target == PIPE_TEXTURE_1D_ARRAY)
         else if (static_state->target == PIPE_TEXTURE_2D)
         else if (static_state->target == PIPE_TEXTURE_2D_ARRAY)
         else if (static_state->target == PIPE_TEXTURE_CUBE)
         else if (static_state->target == PIPE_TEXTURE_CUBE_ARRAY)
         else
                                          struct lp_build_context bld_int_vec4;
            if (params->samples_only) {
      params->sizes_out[0] =
      lp_build_broadcast(gallivm,
                     lp_build_vec_type(gallivm, params->int_type),
   dynamic_state->num_samples(gallivm,
                  LLVMValueRef lod;
   LLVMValueRef level = 0;
   if (params->explicit_lod) {
      /* FIXME: this needs to honor per-element lod */
   lod = LLVMBuildExtractElement(gallivm->builder, params->explicit_lod,
         first_level = dynamic_state->first_level(gallivm, resources_type,
               level = LLVMBuildAdd(gallivm->builder, lod, first_level, "level");
      } else {
                  LLVMValueRef size = bld_int_vec4.undef;
   LLVMValueRef tex_blocksize = bld_int_vec4.undef;
   LLVMValueRef tex_blocksize_log2 = bld_int_vec4.undef;
            uint32_t res_bw = res_format_desc->block.width;
   uint32_t res_bh = res_format_desc->block.height;
   uint32_t bw = format_desc->block.width;
            /* only scale if the blocksizes are different. */
   if (res_bw == bw)
         if (res_bh == bh)
         size = LLVMBuildInsertElement(gallivm->builder, size,
                                 dynamic_state->width(gallivm,
   tex_blocksize = LLVMBuildInsertElement(gallivm->builder, tex_blocksize,
               tex_blocksize_log2 = LLVMBuildInsertElement(gallivm->builder, tex_blocksize_log2,
               view_blocksize = LLVMBuildInsertElement(gallivm->builder, view_blocksize,
               if (dims >= 2) {
      size = LLVMBuildInsertElement(gallivm->builder, size,
                                 dynamic_state->height(gallivm,
   tex_blocksize = LLVMBuildInsertElement(gallivm->builder, tex_blocksize,
               tex_blocksize_log2 = LLVMBuildInsertElement(gallivm->builder, tex_blocksize_log2,
               view_blocksize = LLVMBuildInsertElement(gallivm->builder, view_blocksize,
                     if (dims >= 3) {
      size = LLVMBuildInsertElement(gallivm->builder, size,
                                 dynamic_state->depth(gallivm,
   tex_blocksize = LLVMBuildInsertElement(gallivm->builder, tex_blocksize,
               tex_blocksize_log2 = LLVMBuildInsertElement(gallivm->builder, tex_blocksize_log2,
               view_blocksize = LLVMBuildInsertElement(gallivm->builder, view_blocksize,
                     size = lp_build_minify(&bld_int_vec4, size, lod, true);
   size = lp_build_scale_view_dims(&bld_int_vec4, size, tex_blocksize,
            if (has_array) {
      LLVMValueRef layers = dynamic_state->depth(gallivm, resources_type,
               if (target == PIPE_TEXTURE_CUBE_ARRAY) {
      /*
   * It looks like GL wants number of cubes, d3d10.1 has it undefined?
   * Could avoid this by passing in number of cubes instead of total
   * number of layers (might make things easier elsewhere too).
   */
   LLVMValueRef six = lp_build_const_int32(gallivm, 6);
      }
   size = LLVMBuildInsertElement(gallivm->builder, size, layers,
               /*
   * d3d10 requires zero for x/y/z values (but not w, i.e. mip levels)
   * if level is out of bounds (note this can't cover unbound texture
   * here, which also requires returning zero).
   */
   if (params->explicit_lod && params->is_sviewinfo) {
      LLVMValueRef last_level, out, out1;
            /* everything is scalar for now */
   lp_build_context_init(&leveli_bld, gallivm, lp_type_int_vec(32, 32));
   last_level = dynamic_state->last_level(gallivm, resources_type,
                  out = lp_build_cmp(&leveli_bld, PIPE_FUNC_LESS, level, first_level);
   out1 = lp_build_cmp(&leveli_bld, PIPE_FUNC_GREATER, level, last_level);
   out = lp_build_or(&leveli_bld, out, out1);
   if (num_lods == 1) {
         } else {
      /* TODO */
      }
               unsigned i;
   for (i = 0; i < dims + (has_array ? 1 : 0); i++) {
      params->sizes_out[i] =
      lp_build_extract_broadcast(gallivm, bld_int_vec4.type,
               }
   if (params->is_sviewinfo) {
      for (; i < 4; i++) {
      params->sizes_out[i] = lp_build_const_vec(gallivm,
                  /*
   * if there's no explicit_lod (buffers, rects) queries requiring nr of
   * mips would be illegal.
   */
   if (params->is_sviewinfo && params->explicit_lod) {
      struct lp_build_context bld_int_scalar;
            LLVMValueRef num_levels;
   if (static_state->level_zero_only) {
         } else {
               last_level = dynamic_state->last_level(gallivm, resources_type,
               num_levels = lp_build_sub(&bld_int_scalar, last_level, first_level);
   num_levels = lp_build_add(&bld_int_scalar, num_levels,
      }
   params->sizes_out[3] =
      lp_build_broadcast(gallivm,
                  if (target == PIPE_BUFFER) {
      struct lp_build_context bld_int;
            params->sizes_out[0] = lp_build_min(&bld_int, params->sizes_out[0],
         }
         static void
   lp_build_do_atomic_soa(struct gallivm_state *gallivm,
                        const struct util_format_description *format_desc,
   struct lp_type type,
   LLVMValueRef exec_mask,
   LLVMValueRef base_ptr,
   LLVMValueRef offset,
   LLVMValueRef out_of_bounds,
   unsigned img_op,
      {
               bool valid = format == PIPE_FORMAT_R32_UINT ||
                  bool integer = format != PIPE_FORMAT_R32_FLOAT;
   if (img_op == LP_IMG_ATOMIC) {
      switch (op) {
   case LLVMAtomicRMWBinOpAdd:
   case LLVMAtomicRMWBinOpSub:
   case LLVMAtomicRMWBinOpAnd:
   case LLVMAtomicRMWBinOpNand:
   case LLVMAtomicRMWBinOpOr:
   case LLVMAtomicRMWBinOpXor:
   case LLVMAtomicRMWBinOpMax:
   case LLVMAtomicRMWBinOpMin:
   case LLVMAtomicRMWBinOpUMax:
   case LLVMAtomicRMWBinOpUMin:
      valid &= integer;
      case LLVMAtomicRMWBinOpFAdd:
   #if LLVM_VERSION_MAJOR >= 15
               #endif
            valid &= !integer;
      default:
            } else {
                  if (!valid) {
      atomic_result[0] = lp_build_zero(gallivm, type);
               LLVMTypeRef ref_type = (format == PIPE_FORMAT_R32_FLOAT) ?
      LLVMFloatTypeInContext(gallivm->context) :
         LLVMTypeRef atom_res_elem_type =
                  offset = LLVMBuildGEP2(gallivm->builder,
                  struct lp_build_loop_state loop_state;
   lp_build_loop_begin(&loop_state, gallivm, lp_build_const_int32(gallivm, 0));
   struct lp_build_if_state ifthen;
   LLVMValueRef cond;
            LLVMValueRef should_store_mask =
      LLVMBuildAnd(gallivm->builder, exec_mask,
                     cond = LLVMBuildICmp(gallivm->builder, LLVMIntNE, should_store_mask,
         cond = LLVMBuildExtractElement(gallivm->builder, cond,
                  LLVMValueRef data =
         LLVMValueRef cast_base_ptr =
         cast_base_ptr = LLVMBuildBitCast(gallivm->builder, cast_base_ptr,
         data = LLVMBuildBitCast(gallivm->builder, data,
            if (img_op == LP_IMG_ATOMIC_CAS) {
      LLVMValueRef cas_src_ptr =
      LLVMBuildExtractElement(gallivm->builder, packed2,
      LLVMValueRef cas_src =
      LLVMBuildBitCast(gallivm->builder, cas_src_ptr,
      data = LLVMBuildAtomicCmpXchg(gallivm->builder, cast_base_ptr, data,
                              } else {
      data = LLVMBuildAtomicRMW(gallivm->builder, op,
                           LLVMValueRef temp_res =
         temp_res = LLVMBuildInsertElement(gallivm->builder, temp_res, data,
                  lp_build_endif(&ifthen);
   lp_build_loop_end_cond(&loop_state,
               atomic_result[0] = LLVMBuildLoad2(gallivm->builder, atom_res_elem_type,
      }
         static void
   lp_build_img_op_no_format(struct gallivm_state *gallivm,
               {
      /*
   * If there's nothing bound, format is NONE, and we must return
   * all zero as mandated by d3d10 in this case.
   */
   if (params->img_op != LP_IMG_STORE) {
      LLVMValueRef zero = lp_build_zero(gallivm, params->type);
   for (unsigned chan = 0; chan < (params->img_op == LP_IMG_LOAD ? 4 : 1);
      chan++) {
            }
         void
   lp_build_img_op_soa(const struct lp_static_texture_state *static_texture_state,
                     struct lp_sampler_dynamic_state *dynamic_state,
   {
      const enum pipe_texture_target target = params->target;
   const unsigned dims = texture_dims(target);
   const struct util_format_description *format_desc =
         const struct util_format_description *res_format_desc =
         LLVMValueRef x = params->coords[0], y = params->coords[1],
                  /** regular scalar int type */
   struct lp_type int_coord_type = lp_uint_type(params->type);
   struct lp_build_context int_coord_bld;
            if (static_texture_state->format == PIPE_FORMAT_NONE) {
      lp_build_img_op_no_format(gallivm, params, outdata);
                  LLVMValueRef row_stride = dynamic_state->row_stride(gallivm,
                     LLVMValueRef img_stride = dynamic_state->img_stride(gallivm,
                     LLVMValueRef base_ptr = dynamic_state->base_ptr(gallivm,
                     LLVMValueRef width = dynamic_state->width(gallivm,
                     LLVMValueRef height = dynamic_state->height(gallivm,
                     LLVMValueRef depth = dynamic_state->depth(gallivm,
                              width = lp_build_scale_view_dim(gallivm, width, res_format_desc->block.width,
         width = lp_build_broadcast_scalar(&int_coord_bld, width);
   if (dims >= 2) {
      height = lp_build_scale_view_dim(gallivm, height, res_format_desc->block.height,
         height = lp_build_broadcast_scalar(&int_coord_bld, height);
      }
   if (dims >= 3 || layer_coord) {
      depth = lp_build_broadcast_scalar(&int_coord_bld, depth);
               LLVMValueRef out_of_bounds = int_coord_bld.zero;
   LLVMValueRef out1 = lp_build_cmp(&int_coord_bld, PIPE_FUNC_GEQUAL, x, width);
            if (dims >= 2) {
      out1 = lp_build_cmp(&int_coord_bld, PIPE_FUNC_GEQUAL, y, height);
      }
   if (dims >= 3 || layer_coord) {
      out1 = lp_build_cmp(&int_coord_bld, PIPE_FUNC_GEQUAL, z, depth);
               LLVMValueRef offset, i, j;
   lp_build_sample_offset(&int_coord_bld,
                        if (params->ms_index) {
      LLVMValueRef num_samples = dynamic_state->num_samples(gallivm,
                     LLVMValueRef sample_stride = dynamic_state->sample_stride(gallivm,
                     lp_build_sample_ms_offset(&int_coord_bld,
                  }
   if (params->img_op == LP_IMG_LOAD) {
               offset = lp_build_andnot(&int_coord_bld, offset, out_of_bounds);
   struct lp_build_context texel_bld;
   lp_build_context_init(&texel_bld, gallivm, texel_type);
   lp_build_fetch_rgba_soa(gallivm,
                           format_desc,
            for (unsigned chan = 0; chan < 3; chan++) {
      outdata[chan] = lp_build_select(&texel_bld, out_of_bounds,
      }
   if (format_desc->swizzle[3] == PIPE_SWIZZLE_1) {
      outdata[3] = lp_build_select(&texel_bld, out_of_bounds,
      } else {
      outdata[3] = lp_build_select(&texel_bld, out_of_bounds,
         } else if (params->img_op == LP_IMG_STORE) {
      lp_build_store_rgba_soa(gallivm, format_desc, params->type,
            } else {
      lp_build_do_atomic_soa(gallivm, format_desc, params->type,
                     }
         /*
   * These functions are for indirect texture access suppoort.
   *
   * Indirect textures are implemented using a switch statement, that
   * takes the texture index and jumps to the sampler functions for
   * that texture unit.
   */
      /*
   * Initialise an indexed sampler switch block.
   *
   * This sets up the switch_info state and adds the LLVM flow control pieces.
   */
   void
   lp_build_sample_array_init_soa(struct lp_build_sample_array_switch *switch_info,
                           {
      switch_info->gallivm = gallivm;
   switch_info->params = *params;
   switch_info->base = base;
            /* for generating the switch functions we don't want the texture index
   * offset
   */
            LLVMBasicBlockRef initial_block = LLVMGetInsertBlock(gallivm->builder);
            switch_info->switch_ref = LLVMBuildSwitch(gallivm->builder, idx,
                  LLVMTypeRef val_type[4];
   val_type[0] = val_type[1] = val_type[2] = val_type[3] =
            LLVMTypeRef ret_type =
                              switch_info->phi = LLVMBuildPhi(gallivm->builder, ret_type, "");
      }
         /*
   * Add an individual entry to the indirect texture switch.
   *
   * This builds the sample function and links a case for it into the switch
   * statement.
   */
   void
   lp_build_sample_array_case_soa(struct lp_build_sample_array_switch *switch_info,
                           {
      struct gallivm_state *gallivm = switch_info->gallivm;
            LLVMAddCase(switch_info->switch_ref,
                        LLVMValueRef tex_ret;
   lp_build_sample_soa_func(gallivm, static_texture_state,
                  LLVMAddIncoming(switch_info->phi, &tex_ret, &this_block, 1);
      }
         /*
   * Finish a switch statement.
   *
   * This handles extract the results from the switch.
   */
   void
   lp_build_sample_array_fini_soa(struct lp_build_sample_array_switch *switch_info)
   {
               LLVMPositionBuilderAtEnd(gallivm->builder, switch_info->merge_ref);
   for (unsigned i = 0; i < 4; i++) {
      switch_info->params.texel[i] =
         }
         void
   lp_build_image_op_switch_soa(struct lp_build_img_op_array_switch *switch_info,
                           {
      switch_info->gallivm = gallivm;
   switch_info->params = *params;
   switch_info->base = base;
            /* for generating the switch functions we don't want the texture index
   * offset
   */
            LLVMBasicBlockRef initial_block = LLVMGetInsertBlock(gallivm->builder);
            switch_info->switch_ref =
      LLVMBuildSwitch(gallivm->builder, idx,
         if (params->img_op != LP_IMG_STORE) {
      LLVMTypeRef ret_type = lp_build_vec_type(gallivm, params->type);
                     for (unsigned i = 0; i < ((params->img_op == LP_IMG_LOAD) ? 4 : 1); i++) {
      switch_info->phi[i] = LLVMBuildPhi(gallivm->builder, ret_type, "");
            }
         void
   lp_build_image_op_array_case(struct lp_build_img_op_array_switch *switch_info,
                     {
      struct gallivm_state *gallivm = switch_info->gallivm;
   LLVMBasicBlockRef this_block = lp_build_insert_new_block(gallivm, "img");
            LLVMAddCase(switch_info->switch_ref,
                           lp_build_img_op_soa(static_texture_state, dynamic_state,
            if (switch_info->params.img_op != LP_IMG_STORE) {
      for (unsigned i = 0;
      i < ((switch_info->params.img_op == LP_IMG_LOAD) ? 4 : 1); i++) {
   tex_ret[i] =
      LLVMBuildBitCast(gallivm->builder, tex_ret[i],
                  this_block = LLVMGetInsertBlock(gallivm->builder);
   for (unsigned i = 0;
      i < ((switch_info->params.img_op == LP_IMG_LOAD) ? 4 : 1); i++) {
         }
      }
         void
   lp_build_image_op_array_fini_soa(struct lp_build_img_op_array_switch *switch_info)
   {
                        if (switch_info->params.img_op != LP_IMG_STORE) {
      for (unsigned i = 0;
      i < ((switch_info->params.img_op == LP_IMG_LOAD) ? 4 : 1); i++) {
            }
