   /*
   * Copyright © 2018 Intel Corporation
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
      #include "nir_builder.h"
      /**
   * Some ALU operations may not be supported in hardware in specific bit-sizes.
   * This pass allows implementations to selectively lower such operations to
   * a bit-size that is supported natively and then converts the result back to
   * the original bit-size.
   */
      static nir_def *
   convert_to_bit_size(nir_builder *bld, nir_def *src,
         {
               /* create b2i32(a) instead of i2i32(b2i8(a))/i2i32(b2i16(a)) */
   nir_alu_instr *alu = nir_src_as_alu_instr(nir_src_for_ssa(src));
   if ((type & (nir_type_uint | nir_type_int)) && bit_size == 32 &&
      alu && (alu->op == nir_op_b2i8 || alu->op == nir_op_b2i16)) {
   nir_alu_instr *instr = nir_alu_instr_create(bld->shader, nir_op_b2i32);
   nir_alu_src_copy(&instr->src[0], &alu->src[0]);
                  }
      static void
   lower_alu_instr(nir_builder *bld, nir_alu_instr *alu, unsigned bit_size)
   {
      const nir_op op = alu->op;
                     /* Convert each source to the requested bit-size */
   nir_def *srcs[NIR_MAX_VEC_COMPONENTS] = { NULL };
   for (unsigned i = 0; i < nir_op_infos[op].num_inputs; i++) {
               nir_alu_type type = nir_op_infos[op].input_types[i];
   if (nir_alu_type_get_type_size(type) == 0)
            if (i == 1 && (op == nir_op_ishl || op == nir_op_ishr || op == nir_op_ushr ||
                  op == nir_op_bitz || op == nir_op_bitz8 || op == nir_op_bitz16 ||
   assert(util_is_power_of_two_nonzero(dst_bit_size));
                           /* Emit the lowered ALU instruction */
   nir_def *lowered_dst = NULL;
   if (op == nir_op_imul_high || op == nir_op_umul_high) {
      assert(dst_bit_size * 2 <= bit_size);
   lowered_dst = nir_imul(bld, srcs[0], srcs[1]);
   if (nir_op_infos[op].output_type & nir_type_uint)
         else
      } else if (op == nir_op_iadd_sat || op == nir_op_isub_sat || op == nir_op_uadd_sat ||
            if (op == nir_op_isub_sat)
         else
            /* The add_sat and sub_sat instructions need to clamp the result to the
   * range of the original type.
   */
   if (op == nir_op_iadd_sat || op == nir_op_isub_sat) {
                     lowered_dst = nir_iclamp(bld, lowered_dst,
            } else if (op == nir_op_uadd_sat) {
               lowered_dst = nir_umin(bld, lowered_dst,
      } else {
      assert(op == nir_op_uadd_carry);
         } else {
                  /* Convert result back to the original bit-size */
   if (nir_alu_type_get_type_size(nir_op_infos[op].output_type) == 0 &&
      dst_bit_size != bit_size) {
   nir_alu_type type = nir_op_infos[op].output_type;
   nir_def *dst = nir_convert_to_bit_size(bld, lowered_dst, type, dst_bit_size);
      } else {
            }
      static void
   lower_intrinsic_instr(nir_builder *b, nir_intrinsic_instr *intrin,
         {
      switch (intrin->intrinsic) {
   case nir_intrinsic_read_invocation:
   case nir_intrinsic_read_first_invocation:
   case nir_intrinsic_vote_feq:
   case nir_intrinsic_vote_ieq:
   case nir_intrinsic_shuffle:
   case nir_intrinsic_shuffle_xor:
   case nir_intrinsic_shuffle_up:
   case nir_intrinsic_shuffle_down:
   case nir_intrinsic_quad_broadcast:
   case nir_intrinsic_quad_swap_horizontal:
   case nir_intrinsic_quad_swap_vertical:
   case nir_intrinsic_quad_swap_diagonal:
   case nir_intrinsic_reduce:
   case nir_intrinsic_inclusive_scan:
   case nir_intrinsic_exclusive_scan: {
      const unsigned old_bit_size = intrin->def.bit_size;
            nir_alu_type type = nir_type_uint;
   if (nir_intrinsic_has_reduction_op(intrin))
         else if (intrin->intrinsic == nir_intrinsic_vote_feq)
            b->cursor = nir_before_instr(&intrin->instr);
   nir_intrinsic_instr *new_intrin =
            nir_def *new_src = nir_convert_to_bit_size(b, intrin->src[0].ssa,
                  if (intrin->intrinsic == nir_intrinsic_vote_feq ||
      intrin->intrinsic == nir_intrinsic_vote_ieq) {
   /* These return a Boolean; it's always 1-bit */
      } else {
      /* These return the same bit size as the source; we need to adjust
   * the size and then we'll have to emit a down-cast.
   */
   assert(intrin->src[0].ssa->bit_size == intrin->def.bit_size);
                        nir_def *res = &new_intrin->def;
   if (intrin->intrinsic == nir_intrinsic_exclusive_scan) {
      /* For exclusive scan, we have to be careful because the identity
   * value for the higher bit size may get added into the mix by
   * disabled channels.  For some cases (imin/imax in particular),
   * this value won't convert to the right identity value when we
   * down-cast so we have to clamp it.
   */
   switch (nir_intrinsic_reduction_op(intrin)) {
   case nir_op_imin: {
      int64_t int_max = (1ull << (old_bit_size - 1)) - 1;
   res = nir_imin(b, res, nir_imm_intN_t(b, int_max, bit_size));
      }
   case nir_op_imax: {
      int64_t int_min = -(int64_t)(1ull << (old_bit_size - 1));
   res = nir_imax(b, res, nir_imm_intN_t(b, int_min, bit_size));
      }
   default:
                     if (intrin->intrinsic != nir_intrinsic_vote_feq &&
                  nir_def_rewrite_uses(&intrin->def, res);
               default:
            }
      static void
   lower_phi_instr(nir_builder *b, nir_phi_instr *phi, unsigned bit_size,
         {
      unsigned old_bit_size = phi->def.bit_size;
            nir_foreach_phi_src(src, phi) {
      b->cursor = nir_after_block_before_jump(src->pred);
                                          nir_def *new_dest = nir_u2uN(b, &phi->def, old_bit_size);
   nir_def_rewrite_uses_after(&phi->def, new_dest,
      }
      static bool
   lower_impl(nir_function_impl *impl,
               {
      nir_builder b = nir_builder_create(impl);
            nir_foreach_block(block, impl) {
      /* Stash this so we can rewrite phi destinations quickly. */
            nir_foreach_instr_safe(instr, block) {
      unsigned lower_bit_size = callback(instr, callback_data);
                  switch (instr->type) {
   case nir_instr_type_alu:
                  case nir_instr_type_intrinsic:
      lower_intrinsic_instr(&b, nir_instr_as_intrinsic(instr),
               case nir_instr_type_phi:
      lower_phi_instr(&b, nir_instr_as_phi(instr),
               default:
         }
                  if (progress) {
      nir_metadata_preserve(impl, nir_metadata_block_index |
      } else {
                     }
      bool
   nir_lower_bit_size(nir_shader *shader,
               {
               nir_foreach_function_impl(impl, shader) {
                     }
      static void
   split_phi(nir_builder *b, nir_phi_instr *phi)
   {
      nir_phi_instr *lowered[2] = {
      nir_phi_instr_create(b->shader),
      };
   int num_components = phi->def.num_components;
            nir_foreach_phi_src(src, phi) {
                        nir_def *x = nir_unpack_64_2x32_split_x(b, src->src.ssa);
            nir_phi_instr_add_src(lowered[0], src->pred, x);
               nir_def_init(&lowered[0]->instr, &lowered[0]->def, num_components, 32);
            b->cursor = nir_before_instr(&phi->instr);
   nir_builder_instr_insert(b, &lowered[0]->instr);
            b->cursor = nir_after_phis(nir_cursor_current_block(b->cursor));
   nir_def *merged = nir_pack_64_2x32_split(b, &lowered[0]->def, &lowered[1]->def);
   nir_def_rewrite_uses(&phi->def, merged);
      }
      static bool
   lower_64bit_phi_instr(nir_builder *b, nir_instr *instr, UNUSED void *cb_data)
   {
      if (instr->type != nir_instr_type_phi)
                     if (phi->def.bit_size <= 32)
            split_phi(b, phi);
      }
      bool
   nir_lower_64bit_phis(nir_shader *shader)
   {
      return nir_shader_instructions_pass(shader, lower_64bit_phi_instr,
                  }
