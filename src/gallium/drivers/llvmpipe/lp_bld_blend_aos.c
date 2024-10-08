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
   * Blend LLVM IR generation -- AoS layout.
   *
   * AoS blending is in general much slower than SoA, but there are some cases
   * where it might be faster. In particular, if a pixel is rendered only once
   * then the overhead of tiling and untiling will dominate over the speedup that
   * SoA gives. So we might want to detect such cases and fallback to AoS in the
   * future, but for now this function is here for historical/benchmarking
   * purposes.
   *
   * Run lp_blend_test after any change to this file.
   *
   * @author Jose Fonseca <jfonseca@vmware.com>
   */
         #include "pipe/p_state.h"
   #include "util/u_debug.h"
   #include "util/format/u_format.h"
      #include "gallivm/lp_bld_type.h"
   #include "gallivm/lp_bld_const.h"
   #include "gallivm/lp_bld_arit.h"
   #include "gallivm/lp_bld_logic.h"
   #include "gallivm/lp_bld_swizzle.h"
   #include "gallivm/lp_bld_bitarit.h"
   #include "gallivm/lp_bld_debug.h"
      #include "lp_bld_blend.h"
         /**
   * We may the same values several times, so we keep them here to avoid
   * recomputing them. Also reusing the values allows us to do simplifications
   * that LLVM optimization passes wouldn't normally be able to do.
   */
   struct lp_build_blend_aos_context
   {
               LLVMValueRef src;
   LLVMValueRef src_alpha;
   LLVMValueRef src1;
   LLVMValueRef src1_alpha;
   LLVMValueRef dst;
   LLVMValueRef const_;
   LLVMValueRef const_alpha;
            LLVMValueRef inv_src;
   LLVMValueRef inv_src_alpha;
   LLVMValueRef inv_dst;
   LLVMValueRef inv_const;
   LLVMValueRef inv_const_alpha;
            LLVMValueRef rgb_src_factor;
   LLVMValueRef alpha_src_factor;
   LLVMValueRef rgb_dst_factor;
      };
         static LLVMValueRef
   lp_build_blend_factor_unswizzled(struct lp_build_blend_aos_context *bld,
               {
      LLVMValueRef src_alpha = bld->src_alpha ? bld->src_alpha : bld->src;
   LLVMValueRef src1_alpha = bld->src1_alpha ? bld->src1_alpha : bld->src1;
            switch (factor) {
   case PIPE_BLENDFACTOR_ZERO:
         case PIPE_BLENDFACTOR_ONE:
         case PIPE_BLENDFACTOR_SRC_COLOR:
         case PIPE_BLENDFACTOR_SRC_ALPHA:
         case PIPE_BLENDFACTOR_DST_COLOR:
   case PIPE_BLENDFACTOR_DST_ALPHA:
         case PIPE_BLENDFACTOR_SRC_ALPHA_SATURATE:
      if (alpha) {
         } else {
      /*
   * If there's no dst alpha the complement is zero but for unclamped
   * float inputs (or snorm inputs) min can be non-zero (negative).
   */
   if (!bld->saturate) {
      if (!bld->has_dst_alpha) {
         }
   else if (bld->base.type.norm && bld->base.type.sign) {
      /*
   * The complement/min totally doesn't work, since
   * the complement is in range [0,2] but the other
   * min input is [-1,1]. However, we can just clamp to 0
   * before doing the complement...
   */
   LLVMValueRef inv_dst;
   inv_dst = lp_build_max(&bld->base, bld->base.zero, bld->dst);
   inv_dst = lp_build_comp(&bld->base, inv_dst);
      } else {
      if (!bld->inv_dst) {
         }
         }
         case PIPE_BLENDFACTOR_CONST_COLOR:
         case PIPE_BLENDFACTOR_CONST_ALPHA:
         case PIPE_BLENDFACTOR_SRC1_COLOR:
         case PIPE_BLENDFACTOR_SRC1_ALPHA:
         case PIPE_BLENDFACTOR_INV_SRC_COLOR:
      if (!bld->inv_src)
            case PIPE_BLENDFACTOR_INV_SRC_ALPHA:
      if (!bld->inv_src_alpha)
            case PIPE_BLENDFACTOR_INV_DST_COLOR:
   case PIPE_BLENDFACTOR_INV_DST_ALPHA:
      if (!bld->inv_dst)
            case PIPE_BLENDFACTOR_INV_CONST_COLOR:
      if (!bld->inv_const)
            case PIPE_BLENDFACTOR_INV_CONST_ALPHA:
      if (!bld->inv_const_alpha)
            case PIPE_BLENDFACTOR_INV_SRC1_COLOR:
         case PIPE_BLENDFACTOR_INV_SRC1_ALPHA:
         default:
      assert(0);
         }
         enum lp_build_blend_swizzle {
      LP_BUILD_BLEND_SWIZZLE_RGBA = 0,
      };
         /**
   * How should we shuffle the base factor.
   */
   static enum lp_build_blend_swizzle
   lp_build_blend_factor_swizzle(unsigned factor)
   {
      switch (factor) {
   case PIPE_BLENDFACTOR_ONE:
   case PIPE_BLENDFACTOR_ZERO:
   case PIPE_BLENDFACTOR_SRC_COLOR:
   case PIPE_BLENDFACTOR_DST_COLOR:
   case PIPE_BLENDFACTOR_CONST_COLOR:
   case PIPE_BLENDFACTOR_SRC1_COLOR:
   case PIPE_BLENDFACTOR_INV_SRC_COLOR:
   case PIPE_BLENDFACTOR_INV_DST_COLOR:
   case PIPE_BLENDFACTOR_INV_CONST_COLOR:
   case PIPE_BLENDFACTOR_INV_SRC1_COLOR:
         case PIPE_BLENDFACTOR_SRC_ALPHA:
   case PIPE_BLENDFACTOR_DST_ALPHA:
   case PIPE_BLENDFACTOR_SRC_ALPHA_SATURATE:
   case PIPE_BLENDFACTOR_SRC1_ALPHA:
   case PIPE_BLENDFACTOR_CONST_ALPHA:
   case PIPE_BLENDFACTOR_INV_SRC_ALPHA:
   case PIPE_BLENDFACTOR_INV_DST_ALPHA:
   case PIPE_BLENDFACTOR_INV_CONST_ALPHA:
   case PIPE_BLENDFACTOR_INV_SRC1_ALPHA:
         default:
      assert(0);
         }
         static LLVMValueRef
   lp_build_blend_swizzle(struct lp_build_blend_aos_context *bld,
                        LLVMValueRef rgb,
      {
               switch (rgb_swizzle) {
   case LP_BUILD_BLEND_SWIZZLE_RGBA:
      swizzled_rgb = rgb;
      case LP_BUILD_BLEND_SWIZZLE_AAAA:
      swizzled_rgb = lp_build_swizzle_scalar_aos(&bld->base, rgb,
            default:
      assert(0);
               if (rgb != alpha) {
      swizzled_rgb = lp_build_select_aos(&bld->base, 1 << alpha_swizzle,
                        }
         /**
   * @sa http://www.opengl.org/sdk/docs/man/xhtml/glBlendFuncSeparate.xml
   */
   static LLVMValueRef
   lp_build_blend_factor(struct lp_build_blend_aos_context *bld,
                           {
      LLVMValueRef rgb_factor_, alpha_factor_;
            if (alpha_swizzle == PIPE_SWIZZLE_X && num_channels == 1) {
                           if (alpha_swizzle != PIPE_SWIZZLE_NONE) {
      rgb_swizzle   = lp_build_blend_factor_swizzle(rgb_factor);
   alpha_factor_ = lp_build_blend_factor_unswizzled(bld, alpha_factor, true);
   return lp_build_blend_swizzle(bld, rgb_factor_, alpha_factor_,
      } else {
            }
         /**
   * Performs blending of src and dst pixels
   *
   * @param blend         the blend state of the shader variant
   * @param cbuf_format   format of the colour buffer
   * @param type          data type of the pixel vector
   * @param rt            render target index
   * @param src           blend src
   * @param src_alpha     blend src alpha (if not included in src)
   * @param src1          second blend src (for dual source blend)
   * @param src1_alpha    second blend src alpha (if not included in src1)
   * @param dst           blend dst
   * @param mask          optional mask to apply to the blending result
   * @param const_        const blend color
   * @param const_alpha   const blend color alpha (if not included in const_)
   * @param swizzle       swizzle values for RGBA
   *
   * @return the result of blending src and dst
   */
   LLVMValueRef
   lp_build_blend_aos(struct gallivm_state *gallivm,
                     const struct pipe_blend_state *blend,
   enum pipe_format cbuf_format,
   struct lp_type type,
   unsigned rt,
   LLVMValueRef src,
   LLVMValueRef src_alpha,
   LLVMValueRef src1,
   LLVMValueRef src1_alpha,
   LLVMValueRef dst,
   LLVMValueRef mask,
   LLVMValueRef const_,
   {
      const struct pipe_rt_blend_state *state = &blend->rt[rt];
   const struct util_format_description *desc =
         struct lp_build_blend_aos_context bld;
            /* Setup build context */
   memset(&bld, 0, sizeof bld);
   lp_build_context_init(&bld.base, gallivm, type);
   bld.src = src;
   bld.src1 = src1;
   bld.dst = dst;
   bld.const_ = const_;
   bld.src_alpha = src_alpha;
   bld.src1_alpha = src1_alpha;
   bld.const_alpha = const_alpha;
            /* Find the alpha channel if not provided separately */
   unsigned alpha_swizzle = PIPE_SWIZZLE_NONE;
   if (!src_alpha) {
      for (unsigned i = 0; i < 4; ++i) {
      if (swizzle[i] == 3) {
            }
   /*
   * Note that we may get src_alpha included from source (and 4 channels)
   * even if the destination doesn't have an alpha channel (for rgbx
   * formats). Generally this shouldn't make much of a difference (we're
   * relying on blend factors being sanitized already if there's no
   * dst alpha).
   */
               if (blend->logicop_enable) {
      if (!type.floating) {
      result = lp_build_logicop(gallivm->builder, blend->logicop_func,
      } else {
            } else if (!state->blend_enable) {
         } else {
      bool rgb_alpha_same =
      (state->rgb_src_factor == state->rgb_dst_factor &&
   state->alpha_src_factor == state->alpha_dst_factor) ||
      bool alpha_only = nr_channels == 1 && alpha_swizzle == PIPE_SWIZZLE_X;
            src_factor = lp_build_blend_factor(&bld, state->rgb_src_factor,
                        dst_factor = lp_build_blend_factor(&bld, state->rgb_dst_factor,
                        result = lp_build_blend(&bld.base,
                           state->rgb_func,
   alpha_only ? state->alpha_src_factor : state->rgb_src_factor,
   alpha_only ? state->alpha_dst_factor : state->rgb_dst_factor,
   src,
            if (state->rgb_func != state->alpha_func && nr_channels > 1 &&
                     alpha = lp_build_blend(&bld.base,
                        state->alpha_func,
   state->alpha_src_factor,
   state->alpha_dst_factor,
   src,
               result = lp_build_blend_swizzle(&bld,
                                          /* Check if color mask is necessary */
   if (!util_format_colormask_full(desc, state->colormask)) {
      LLVMValueRef color_mask =
      lp_build_const_mask_aos_swizzled(gallivm, bld.base.type,
                     /* Combine with input mask if necessary */
   if (mask) {
      /* We can be blending floating values but masks are always integer... */
                              } else {
                     /* Apply mask, if one exists */
   if (mask) {
                     }
