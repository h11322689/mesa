   /**************************************************************************
   *
   * Copyright 2009 VMware, Inc.
   * Copyright 2007-2008 VMware, Inc.
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
   * Position and shader input interpolation.
   *
   * @author Jose Fonseca <jfonseca@vmware.com>
   */
      #include "pipe/p_shader_tokens.h"
   #include "util/compiler.h"
   #include "util/u_debug.h"
   #include "util/u_memory.h"
   #include "util/u_math.h"
   #include "gallivm/lp_bld_debug.h"
   #include "gallivm/lp_bld_const.h"
   #include "gallivm/lp_bld_arit.h"
   #include "gallivm/lp_bld_swizzle.h"
   #include "gallivm/lp_bld_flow.h"
   #include "gallivm/lp_bld_logic.h"
   #include "gallivm/lp_bld_struct.h"
   #include "gallivm/lp_bld_gather.h"
   #include "lp_bld_interp.h"
         /*
   * The shader JIT function operates on blocks of quads.
   * Each block has 2x2 quads and each quad has 2x2 pixels.
   *
   * We iterate over the quads in order 0, 1, 2, 3:
   *
   * #################
   * #   |   #   |   #
   * #---0---#---1---#
   * #   |   #   |   #
   * #################
   * #   |   #   |   #
   * #---2---#---3---#
   * #   |   #   |   #
   * #################
   *
   * If we iterate over multiple quads at once, quads 01 and 23 are processed
   * together.
   *
   * Within each quad, we have four pixels which are represented in SOA
   * order:
   *
   * #########
   * # 0 | 1 #
   * #---+---#
   * # 2 | 3 #
   * #########
   *
   * So the green channel (for example) of the four pixels is stored in
   * a single vector register: {g0, g1, g2, g3}.
   * The order stays the same even with multiple quads:
   * 0 1 4 5
   * 2 3 6 7
   * is stored as g0..g7
   */
         /**
   * Do one perspective divide per quad.
   *
   * For perspective interpolation, the final attribute value is given
   *
   *  a' = a/w = a * oow
   *
   * where
   *
   *  a = a0 + dadx*x + dady*y
   *  w = w0 + dwdx*x + dwdy*y
   *  oow = 1/w = 1/(w0 + dwdx*x + dwdy*y)
   *
   * Instead of computing the division per pixel, with this macro we compute the
   * division on the upper left pixel of each quad, and use a linear
   * approximation in the remaining pixels, given by:
   *
   *  da'dx = (dadx - dwdx*a)*oow
   *  da'dy = (dady - dwdy*a)*oow
   *
   * Ironically, this actually makes things slower -- probably because the
   * divide hardware unit is rarely used, whereas the multiply unit is typically
   * already saturated.
   */
   #define PERSPECTIVE_DIVIDE_PER_QUAD 0
         static const unsigned char quad_offset_x[16] = {0, 1, 0, 1, 2, 3, 2, 3, 0, 1, 0, 1, 2, 3, 2, 3};
   static const unsigned char quad_offset_y[16] = {0, 0, 1, 1, 0, 0, 1, 1, 2, 2, 3, 3, 2, 2, 3, 3};
         static void
   attrib_name(LLVMValueRef val, unsigned attrib, unsigned chan, const char *suffix)
   {
      if (attrib == 0)
         else
      }
         static void
   calc_offsets(struct lp_build_context *coeff_bld,
               unsigned quad_start_index,
   {
      unsigned num_pix = coeff_bld->type.length;
   struct gallivm_state *gallivm = coeff_bld->gallivm;
   LLVMBuilderRef builder = coeff_bld->gallivm->builder;
            *pixoffx = coeff_bld->undef;
            for (unsigned i = 0; i < num_pix; i++) {
      nr = lp_build_const_int32(gallivm, i);
   pixxf = lp_build_const_float(gallivm, quad_offset_x[i % num_pix] +
         pixyf = lp_build_const_float(gallivm, quad_offset_y[i % num_pix] +
         *pixoffx = LLVMBuildInsertElement(builder, *pixoffx, pixxf, nr, "");
         }
         static void
   calc_centroid_offsets(struct lp_build_interp_soa_context *bld,
                        struct gallivm_state *gallivm,
   LLVMValueRef loop_iter,
      {
      struct lp_build_context *coeff_bld = &bld->coeff_bld;
   LLVMBuilderRef builder = gallivm->builder;
   LLVMValueRef s_mask_and = NULL;
   LLVMValueRef centroid_x_offset = pix_center_offset;
   LLVMValueRef centroid_y_offset = pix_center_offset;
   for (int s = bld->coverage_samples - 1; s >= 0; s--) {
      LLVMValueRef sample_cov;
            s_mask_idx = LLVMBuildAdd(builder, s_mask_idx, loop_iter, "");
   sample_cov = lp_build_pointer_get2(builder, mask_type, mask_store, s_mask_idx);
   if (s == bld->coverage_samples - 1)
         else
            LLVMValueRef x_val_idx = lp_build_const_int32(gallivm, s * 2);
            x_val_idx = lp_build_array_get2(gallivm, bld->sample_pos_array_type,
         y_val_idx = lp_build_array_get2(gallivm, bld->sample_pos_array_type,
         x_val_idx = lp_build_broadcast_scalar(coeff_bld, x_val_idx);
   y_val_idx = lp_build_broadcast_scalar(coeff_bld, y_val_idx);
   centroid_x_offset = lp_build_select(coeff_bld, sample_cov, x_val_idx, centroid_x_offset);
      }
   *centroid_x = lp_build_select(coeff_bld, s_mask_and, pix_center_offset, centroid_x_offset);
      }
         /* Note: this assumes the pointer to elem_type is in address space 0 */
   static LLVMValueRef
   load_casted(LLVMBuilderRef builder, LLVMTypeRef elem_type,
            ptr = LLVMBuildBitCast(builder, ptr, LLVMPointerType(elem_type, 0), name);
      }
         static LLVMValueRef
   indexed_load(LLVMBuilderRef builder, LLVMTypeRef gep_type,
                  ptr = LLVMBuildGEP2(builder, gep_type, ptr, &index, 1, name);
      }
         /* Much easier, and significantly less instructions in the per-stamp
   * part (less than half) but overall more instructions so a loss if
   * most quads are active. Might be a win though with larger vectors.
   * No ability to do per-quad divide (doable but not implemented)
   * Could be made to work with passed in pixel offsets (i.e. active quad
   * merging).
   */
   static void
   coeffs_init_simple(struct lp_build_interp_soa_context *bld,
                     {
      struct lp_build_context *coeff_bld = &bld->coeff_bld;
   struct lp_build_context *setup_bld = &bld->setup_bld;
   struct gallivm_state *gallivm = coeff_bld->gallivm;
            for (unsigned attrib = 0; attrib < bld->num_attribs; ++attrib) {
      /*
   * always fetch all 4 values for performance/simplicity
   * Note: we do that here because it seems to generate better
   * code. It generates a lot of moves initially but less
   * moves later. As far as I can tell this looks like a
   * llvm issue, instead of simply reloading the values from
   * the passed in pointers it if it runs out of registers
   * it spills/reloads them. Maybe some optimization passes
   * would help.
   * Might want to investigate this again later.
   */
   const enum lp_interp interp = bld->interp[attrib];
   LLVMValueRef index = lp_build_const_int32(gallivm,
         LLVMValueRef dadxaos = setup_bld->zero;
   LLVMValueRef dadyaos = setup_bld->zero;
            /* See: lp_state_fs.c / generate_fragment() / fs_elem_type */
            switch (interp) {
   case LP_INTERP_PERSPECTIVE:
            case LP_INTERP_LINEAR:
      dadxaos = indexed_load(builder, fs_elem_type,
         dadyaos = indexed_load(builder, fs_elem_type,
         attrib_name(dadxaos, attrib, 0, ".dadxaos");
               case LP_INTERP_CONSTANT:
   case LP_INTERP_FACING:
      a0aos = indexed_load(builder, fs_elem_type,
                     case LP_INTERP_POSITION:
                  default:
      assert(0);
      }
   bld->a0aos[attrib] = a0aos;
   bld->dadxaos[attrib] = dadxaos;
         }
         /**
   * Interpolate the shader input attribute values.
   * This is called for each (group of) quad(s).
   */
   static void
   attribs_update_simple(struct lp_build_interp_soa_context *bld,
                        struct gallivm_state *gallivm,
   LLVMValueRef loop_iter,
   LLVMTypeRef mask_type,
      {
      LLVMBuilderRef builder = gallivm->builder;
   struct lp_build_context *coeff_bld = &bld->coeff_bld;
   struct lp_build_context *setup_bld = &bld->setup_bld;
   LLVMValueRef oow = NULL;
   LLVMValueRef pixoffx;
   LLVMValueRef pixoffy;
   LLVMValueRef ptr;
   LLVMValueRef pix_center_offset = lp_build_const_vec(gallivm,
                     assert(loop_iter);
   ptr = LLVMBuildGEP2(builder, bld->store_elem_type, bld->xoffset_store,
         pixoffx = LLVMBuildLoad2(builder, bld->store_elem_type, ptr, "");
   ptr = LLVMBuildGEP2(builder, bld->store_elem_type, bld->yoffset_store,
                  pixoffx = LLVMBuildFAdd(builder, pixoffx,
         pixoffy = LLVMBuildFAdd(builder, pixoffy,
            for (unsigned attrib = start; attrib < end; attrib++) {
      const unsigned mask = bld->mask[attrib];
   const enum lp_interp interp = bld->interp[attrib];
            for (unsigned chan = 0; chan < TGSI_NUM_CHANNELS; chan++) {
      if (mask & (1 << chan)) {
      LLVMValueRef index = lp_build_const_int32(gallivm, chan);
   LLVMValueRef dadx = coeff_bld->zero;
   LLVMValueRef dady = coeff_bld->zero;
                  switch (interp) {
                  case LP_INTERP_LINEAR:
      if (attrib == 0 && chan == 0) {
      dadx = coeff_bld->one;
   if (sample_id) {
      LLVMValueRef x_val_idx = LLVMBuildMul(gallivm->builder, sample_id, lp_build_const_int32(gallivm, 2), "");
   x_val_idx = lp_build_array_get2(gallivm, bld->sample_pos_array_type,
            } else {
            }
   else if (attrib == 0 && chan == 1) {
      dady = coeff_bld->one;
   if (sample_id) {
      LLVMValueRef y_val_idx = LLVMBuildMul(gallivm->builder, sample_id, lp_build_const_int32(gallivm, 2), "");
   y_val_idx = LLVMBuildAdd(gallivm->builder, y_val_idx, lp_build_const_int32(gallivm, 1), "");
   y_val_idx = lp_build_array_get2(gallivm, bld->sample_pos_array_type,
            } else {
            }
   else {
      dadx = lp_build_extract_broadcast(gallivm, setup_bld->type,
               dady = lp_build_extract_broadcast(gallivm, setup_bld->type,
                                    if (bld->coverage_samples > 1) {
      LLVMValueRef xoffset = pix_center_offset;
   LLVMValueRef yoffset = pix_center_offset;
                                 x_val_idx = lp_build_array_get2(gallivm, bld->sample_pos_array_type,
         y_val_idx = lp_build_array_get2(gallivm, bld->sample_pos_array_type,
         xoffset = lp_build_broadcast_scalar(coeff_bld, x_val_idx);
      } else if (loc == TGSI_INTERPOLATE_LOC_CENTROID) {
      calc_centroid_offsets(bld, gallivm, loop_iter, mask_type, mask_store,
      }
                        /*
   * a = a0 + (x * dadx + y * dady)
                        if (interp == LP_INTERP_PERSPECTIVE) {
      if (oow == NULL) {
      LLVMValueRef w = bld->attribs[0][3];
   assert(attrib != 0);
   assert(bld->mask[0] & TGSI_WRITEMASK_W);
      }
                  case LP_INTERP_CONSTANT:
   case LP_INTERP_FACING:
      a = lp_build_extract_broadcast(gallivm, setup_bld->type,
                     case LP_INTERP_POSITION:
                        default:
                        if ((attrib == 0) && (chan == 2)) {
      /* add polygon-offset value, stored in the X component of a0 */
   LLVMValueRef offset =
      lp_build_extract_broadcast(gallivm, setup_bld->type,
                                    }
         static LLVMValueRef
   lp_build_interp_soa_indirect(struct lp_build_interp_soa_context *bld,
                                 {
      LLVMBuilderRef builder = gallivm->builder;
   struct lp_build_context *coeff_bld = &bld->coeff_bld;
   const enum lp_interp interp = bld->interp[attrib];
   LLVMValueRef dadx = coeff_bld->zero;
   LLVMValueRef dady = coeff_bld->zero;
   LLVMValueRef a = coeff_bld->zero;
   LLVMTypeRef u8ptr =
            indir_index = LLVMBuildAdd(builder, indir_index,
               LLVMValueRef index = LLVMBuildMul(builder, indir_index,
                     index = LLVMBuildAdd(builder, index,
                  /* size up to byte indices */
   index = LLVMBuildMul(builder, index,
                  struct lp_type dst_type = coeff_bld->type;
   dst_type.length = 1;
   switch (interp) {
   case LP_INTERP_PERSPECTIVE:
         case LP_INTERP_LINEAR:
      dadx = lp_build_gather(gallivm, coeff_bld->type.length,
                              dady = lp_build_gather(gallivm, coeff_bld->type.length,
                              a = lp_build_gather(gallivm, coeff_bld->type.length,
                              /*
   * a = a0 + (x * dadx + y * dady)
   */
   a = lp_build_fmuladd(builder, dadx, pixoffx, a);
            if (interp == LP_INTERP_PERSPECTIVE) {
   LLVMValueRef w = bld->attribs[0][3];
   assert(attrib != 0);
   assert(bld->mask[0] & TGSI_WRITEMASK_W);
   LLVMValueRef oow = lp_build_rcp(coeff_bld, w);
   a = lp_build_mul(coeff_bld, a, oow);
               case LP_INTERP_CONSTANT:
   case LP_INTERP_FACING:
      a = lp_build_gather(gallivm, coeff_bld->type.length,
                     coeff_bld->type.width, dst_type,
      default:
      assert(0);
      }
      }
         LLVMValueRef
   lp_build_interp_soa(struct lp_build_interp_soa_context *bld,
                     struct gallivm_state *gallivm,
   LLVMValueRef loop_iter,
   LLVMTypeRef mask_type,
   LLVMValueRef mask_store,
   unsigned attrib, unsigned chan,
   {
      LLVMBuilderRef builder = gallivm->builder;
   struct lp_build_context *coeff_bld = &bld->coeff_bld;
   struct lp_build_context *setup_bld = &bld->setup_bld;
   LLVMValueRef pixoffx;
   LLVMValueRef pixoffy;
                     assert(loop_iter);
   ptr = LLVMBuildGEP2(builder, bld->store_elem_type, bld->xoffset_store,
         pixoffx = LLVMBuildLoad2(builder, bld->store_elem_type, ptr, "");
   ptr = LLVMBuildGEP2(builder, bld->store_elem_type, bld->yoffset_store,
                  pixoffx = LLVMBuildFAdd(builder, pixoffx,
         pixoffy = LLVMBuildFAdd(builder, pixoffy,
            LLVMValueRef pix_center_offset = lp_build_const_vec(gallivm,
            if (loc == TGSI_INTERPOLATE_LOC_CENTER) {
      if (bld->coverage_samples > 1) {
      pixoffx = LLVMBuildFAdd(builder, pixoffx, pix_center_offset, "");
               if (offsets[0])
      pixoffx = LLVMBuildFAdd(builder, pixoffx,
      if (offsets[1])
      pixoffy = LLVMBuildFAdd(builder, pixoffy,
   } else if (loc == TGSI_INTERPOLATE_LOC_SAMPLE) {
      LLVMValueRef x_val_idx = LLVMBuildMul(gallivm->builder, offsets[0],
         LLVMValueRef y_val_idx = LLVMBuildAdd(gallivm->builder, x_val_idx,
            LLVMValueRef base_ptr =
      LLVMBuildBitCast(gallivm->builder,
            LLVMValueRef xoffset = lp_build_gather(gallivm,
                                       LLVMValueRef yoffset = lp_build_gather(gallivm,
                                          if (bld->coverage_samples > 1) {
      pixoffx = LLVMBuildFAdd(builder, pixoffx, xoffset, "");
         } else if (loc == TGSI_INTERPOLATE_LOC_CENTROID) {
               /* for centroid find covered samples for this quad. */
   /* if all samples are covered use pixel centers */
   if (bld->coverage_samples > 1) {
      calc_centroid_offsets(bld, gallivm, loop_iter, mask_type, mask_store,
                  pixoffx = LLVMBuildFAdd(builder, pixoffx, centroid_x_offset, "");
                  // remap attrib properly.
            if (indir_index)
   return lp_build_interp_soa_indirect(bld, gallivm, attrib, chan,
               const enum lp_interp interp = bld->interp[attrib];
   LLVMValueRef dadx = coeff_bld->zero;
   LLVMValueRef dady = coeff_bld->zero;
                     switch (interp) {
   case LP_INTERP_PERSPECTIVE:
         case LP_INTERP_LINEAR:
      dadx = lp_build_extract_broadcast(gallivm, setup_bld->type,
                  dady = lp_build_extract_broadcast(gallivm, setup_bld->type,
                  a = lp_build_extract_broadcast(gallivm, setup_bld->type,
                  /*
   * a = a0 + (x * dadx + y * dady)
   */
   a = lp_build_fmuladd(builder, dadx, pixoffx, a);
            if (interp == LP_INTERP_PERSPECTIVE) {
   LLVMValueRef w = bld->attribs[0][3];
   assert(attrib != 0);
   assert(bld->mask[0] & TGSI_WRITEMASK_W);
   LLVMValueRef oow = lp_build_rcp(coeff_bld, w);
   a = lp_build_mul(coeff_bld, a, oow);
               case LP_INTERP_CONSTANT:
   case LP_INTERP_FACING:
      a = lp_build_extract_broadcast(gallivm, setup_bld->type,
                  default:
      assert(0);
      }
      }
         /**
   * Generate the position vectors.
   *
   * Parameter x0, y0 are the integer values with upper left coordinates.
   */
   static void
   pos_init(struct lp_build_interp_soa_context *bld,
               {
      LLVMBuilderRef builder = bld->coeff_bld.gallivm->builder;
            bld->x = LLVMBuildSIToFP(builder, x0, coeff_bld->elem_type, "");
      }
         /**
   * Initialize fragment shader input attribute info.
   */
   void
   lp_build_interp_soa_init(struct lp_build_interp_soa_context *bld,
                           struct gallivm_state *gallivm,
   unsigned num_inputs,
   const struct lp_shader_input *inputs,
   bool pixel_center_integer,
   unsigned coverage_samples,
   LLVMTypeRef sample_pos_array_type,
   LLVMValueRef sample_pos_array,
   LLVMValueRef num_loop,
   LLVMBuilderRef builder,
   struct lp_type type,
   LLVMValueRef a0_ptr,
   {
      struct lp_type coeff_type;
   struct lp_type setup_type;
   unsigned attrib;
                     memset(&coeff_type, 0, sizeof coeff_type);
   coeff_type.floating = true;
   coeff_type.sign = true;
   coeff_type.width = 32;
            memset(&setup_type, 0, sizeof setup_type);
   setup_type.floating = true;
   setup_type.sign = true;
   setup_type.width = 32;
               /* XXX: we don't support interpolating into any other types */
            lp_build_context_init(&bld->coeff_bld, gallivm, coeff_type);
            /* For convenience */
   bld->pos = bld->attribs[0];
            /* Position */
   bld->mask[0] = TGSI_WRITEMASK_XYZW;
   bld->interp[0] = LP_INTERP_LINEAR;
            /* Inputs */
   for (attrib = 0; attrib < num_inputs; ++attrib) {
      bld->mask[1 + attrib] = inputs[attrib].usage_mask;
   bld->interp[1 + attrib] = inputs[attrib].interp;
      }
            /* needed for indirect */
   bld->a0_ptr = a0_ptr;
   bld->dadx_ptr = dadx_ptr;
            /* Ensure all masked out input channels have a valid value */
   for (attrib = 0; attrib < bld->num_attribs; ++attrib) {
      for (chan = 0; chan < TGSI_NUM_CHANNELS; ++chan) {
                     if (pixel_center_integer) {
         } else {
         }
   bld->coverage_samples = coverage_samples;
   bld->num_loop = num_loop;
   bld->sample_pos_array_type = sample_pos_array_type;
                     /*
   * Simple method (single step interpolation) may be slower if vector length
   * is just 4, but the results are different (generally less accurate) with
   * the other method, so always use more accurate version.
   */
   {
      /* XXX this should use a global static table */
   unsigned i;
   unsigned num_loops = 16 / type.length;
   LLVMValueRef pixoffx, pixoffy, index;
            bld->store_elem_type = lp_build_vec_type(gallivm, type);
   bld->xoffset_store =
      lp_build_array_alloca(gallivm, bld->store_elem_type,
      bld->yoffset_store =
      lp_build_array_alloca(gallivm, bld->store_elem_type,
      for (i = 0; i < num_loops; i++) {
      index = lp_build_const_int32(gallivm, i);
   calc_offsets(&bld->coeff_bld, i*type.length/4, &pixoffx, &pixoffy);
   ptr = LLVMBuildGEP2(builder, bld->store_elem_type,
         LLVMBuildStore(builder, pixoffx, ptr);
   ptr = LLVMBuildGEP2(builder, bld->store_elem_type,
               }
      }
         /*
   * Advance the position and inputs to the given quad within the block.
   */
      void
   lp_build_interp_soa_update_inputs_dyn(struct lp_build_interp_soa_context *bld,
                                 {
      attribs_update_simple(bld, gallivm, quad_start_index, mask_type,
      }
         void
   lp_build_interp_soa_update_pos_dyn(struct lp_build_interp_soa_context *bld,
                     {
      attribs_update_simple(bld, gallivm, quad_start_index,
      }
   