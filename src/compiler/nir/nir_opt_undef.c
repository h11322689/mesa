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
      #include "nir.h"
   #include "nir_builder.h"
   #include "util/mesa-sha1.h"
   #include <math.h>
      /** @file nir_opt_undef.c
   *
   * Handles optimization of operations involving ssa_undef.
   */
      struct undef_options {
         };
      /**
   * Turn conditional selects between an undef and some other value into a move
   * of that other value (on the assumption that the condition's going to be
   * choosing the defined value).  This reduces work after if flattening when
   * each side of the if is defining a variable.
   */
   static bool
   opt_undef_csel(nir_builder *b, nir_alu_instr *instr)
   {
      if (!nir_op_is_selection(instr->op))
            for (int i = 1; i <= 2; i++) {
      nir_instr *parent = instr->src[i].src.ssa->parent_instr;
   if (parent->type != nir_instr_type_undef)
            b->cursor = nir_instr_remove(&instr->instr);
   nir_def *mov = nir_mov_alu(b, instr->src[i == 1 ? 2 : 1],
                                 }
      /**
   * Replace vecN(undef, undef, ...) with a single undef.
   */
   static bool
   opt_undef_vecN(nir_builder *b, nir_alu_instr *alu)
   {
      if (!nir_op_is_vec_or_mov(alu->op))
            for (unsigned i = 0; i < nir_op_infos[alu->op].num_inputs; i++) {
      if (alu->src[i].src.ssa->parent_instr->type != nir_instr_type_undef)
               b->cursor = nir_before_instr(&alu->instr);
   nir_def *undef = nir_undef(b, alu->def.num_components,
                     }
      static uint32_t
   nir_get_undef_mask(nir_def *def)
   {
               if (instr->type == nir_instr_type_undef)
            if (instr->type != nir_instr_type_alu)
            nir_alu_instr *alu = nir_instr_as_alu(instr);
            /* nir_op_mov of undef is handled by opt_undef_vecN() */
   if (nir_op_is_vec(alu->op)) {
      for (int i = 0; i < nir_op_infos[alu->op].num_inputs; i++) {
      if (alu->src[i].src.ssa->parent_instr->type ==
      nir_instr_type_undef) {
                        }
      /**
   * Remove any store intrinsic writemask channels whose value is undefined (the
   * existing value is a fine representation of "undefined").
   */
   static bool
   opt_undef_store(nir_intrinsic_instr *intrin)
   {
      int arg_index;
   switch (intrin->intrinsic) {
   case nir_intrinsic_store_deref:
      arg_index = 1;
      case nir_intrinsic_store_output:
   case nir_intrinsic_store_per_vertex_output:
   case nir_intrinsic_store_per_primitive_output:
   case nir_intrinsic_store_ssbo:
   case nir_intrinsic_store_shared:
   case nir_intrinsic_store_global:
   case nir_intrinsic_store_scratch:
      arg_index = 0;
      default:
                           unsigned write_mask = nir_intrinsic_write_mask(intrin);
            if (!(write_mask & undef_mask))
            write_mask &= ~undef_mask;
   if (!write_mask)
         else
               }
      struct visit_info {
      bool replace_undef_with_constant;
   bool prefer_nan;
      };
      /**
   * Analyze an undef use to see if replacing undef with a constant is
   * beneficial.
   */
   static void
   visit_undef_use(nir_src *src, struct visit_info *info)
   {
      if (nir_src_is_if(src)) {
      /* If the use is "if", keep undef because the branch will be eliminated
   * by nir_opt_dead_cf.
   */
   info->must_keep_undef = true;
                        if (instr->type == nir_instr_type_alu) {
      /* Replacing undef with a constant is only beneficial with ALU
   * instructions because it can eliminate them or simplify them.
   */
            /* Follow movs and vecs.
   *
   * Note that all vector component uses are followed and swizzles are
   * ignored.
   */
   if (alu->op == nir_op_mov || nir_op_is_vec(alu->op)) {
      nir_foreach_use_including_if(next_src, &alu->def) {
         }
                        for (unsigned i = 0; i < num_srcs; i++) {
                     if (nir_op_is_selection(alu->op) && i != 0) {
      /* nir_opt_algebraic can eliminate a select opcode only if src0 is
   * a constant. If the undef use is src1 or src2, it will be
   * handled by opt_undef_csel.
   */
               info->replace_undef_with_constant = true;
   if (nir_op_infos[alu->op].input_types[i] & nir_type_float &&
      alu->op != nir_op_fmulz &&
   (alu->op != nir_op_ffmaz || i == 2))
      } else {
      /* If the use is not ALU, don't replace undef. We need to preserve
   * undef for stores and phis because those are handled differently,
   * and replacing undef with a constant would result in worse code.
   */
   info->must_keep_undef = true;
         }
      /**
   * Replace ssa_undef used by ALU opcodes with 0 or NaN, whichever eliminates
   * more code.
   *
   * Replace it with NaN if an FP opcode uses undef, which will cause the opcode
   * to be eliminated by nir_opt_algebraic. 0 would not eliminate the FP opcode.
   */
   static bool
   replace_ssa_undef(nir_builder *b, nir_instr *instr,
         {
      nir_undef_instr *undef = nir_instr_as_undef(instr);
            nir_foreach_use_including_if(src, &undef->def) {
                  if (info.must_keep_undef || !info.replace_undef_with_constant)
            b->cursor = nir_before_instr(&undef->instr);
            /* If undef is used as float, replace it with NaN, which will
   * eliminate all FP instructions that consume it. Else, replace it
   * with 0, which is more likely to eliminate non-FP instructions.
   */
   if (info.prefer_nan && !options->disallow_undef_to_nan)
         else
            if (undef->def.num_components > 1)
            nir_def_rewrite_uses_after(&undef->def, replacement, &undef->instr);
   nir_instr_remove(&undef->instr);
      }
      static bool
   nir_opt_undef_instr(nir_builder *b, nir_instr *instr, void *data)
   {
               if (instr->type == nir_instr_type_undef) {
         } else if (instr->type == nir_instr_type_alu) {
      nir_alu_instr *alu = nir_instr_as_alu(instr);
   return opt_undef_csel(b, alu) ||
      } else if (instr->type == nir_instr_type_intrinsic) {
      nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(instr);
                  }
      bool
   nir_opt_undef(nir_shader *shader)
   {
               /* Disallow the undef->NaN transformation only for those shaders where
   * it's known to break rendering. These are shader source SHA1s printed by
   * nir_print_shader().
   */
   uint32_t shader_sha1s[][SHA1_DIGEST_LENGTH32] = {
      /* gputest/gimark */
            /* Viewperf13/CATIA_car_01 */
   {0x4746a4a4, 0xe3b27d27, 0xe6d2b0fb, 0xb7e9ceb3, 0x973e6152}, /* Taillights */
   {0xc49cc90d, 0xd7208212, 0x726502ea, 0xe1fe62c0, 0xb62fbd1f}, /* Grill */
   {0xde23f35b, 0xb6fa45ae, 0x96da7e6b, 0x5a6e4a60, 0xce0b6b31}, /* Headlights */
            /* Viewperf13/CATIA_car_04 */
   {0x631da72a, 0xc971e849, 0xd6489a15, 0xf7c8dddb, 0xe8efd982}, /* Headlights */
   {0x85984b88, 0xd16b8fee, 0x0d49d97b, 0x5f6cc66e, 0xadcafad9}, /* Rims */
   {0xad023488, 0x09930735, 0xb0567e58, 0x336dce36, 0xe3c1e448}, /* Tires */
   {0xdcc4a549, 0x587873fa, 0xeed94361, 0x9a47cbff, 0x846d0167}, /* Windows */
               for (unsigned i = 0; i < ARRAY_SIZE(shader_sha1s); i++) {
      if (_mesa_printed_sha1_equal(shader->info.source_sha1, shader_sha1s[i])) {
      options.disallow_undef_to_nan = true;
                  return nir_shader_instructions_pass(shader,
                        }
