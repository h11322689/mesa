   /*
   * Copyright © 2018 Intel Corporation
   * Copyright © 2019 Vasily Khoruzhick <anarsoul@gmail.com>
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
      static bool
   assert_ssa_def_is_not_int(nir_def *def, void *arg)
   {
      ASSERTED BITSET_WORD *int_types = arg;
   assert(!BITSET_TEST(int_types, def->index));
      }
      static bool
   instr_has_only_trivial_swizzles(nir_alu_instr *alu)
   {
               for (unsigned i = 0; i < info->num_inputs; i++) {
      for (unsigned chan = 0; chan < alu->def.num_components; chan++) {
      if (alu->src[i].swizzle[chan] != chan)
         }
      }
      /* Recognize the y = x - ffract(x) patterns from lowered ffloor.
   * It only works for the simple case when no swizzling is involved.
   */
   static bool
   check_for_lowered_ffloor(nir_alu_instr *fadd)
   {
      if (!instr_has_only_trivial_swizzles(fadd))
            nir_alu_instr *fneg = NULL;
   nir_src x;
   for (unsigned i = 0; i < 2; i++) {
      nir_alu_instr *fadd_src_alu = nir_src_as_alu_instr(fadd->src[i].src);
   if (fadd_src_alu && fadd_src_alu->op == nir_op_fneg) {
      fneg = fadd_src_alu;
                  if (!fneg || !instr_has_only_trivial_swizzles(fneg))
            nir_alu_instr *ffract = nir_src_as_alu_instr(fneg->src[0].src);
   if (ffract && ffract->op == nir_op_ffract &&
      nir_srcs_equal(ffract->src[0].src, x) &&
   instr_has_only_trivial_swizzles(ffract))
            }
      static bool
   lower_alu_instr(nir_builder *b, nir_alu_instr *alu)
   {
               bool is_bool_only = alu->def.bit_size == 1;
   for (unsigned i = 0; i < info->num_inputs; i++) {
      if (alu->src[i].src.ssa->bit_size != 1)
               if (is_bool_only) {
      /* avoid lowering integers ops are used for booleans (ieq,ine,etc) */
                        /* Replacement SSA value */
   nir_def *rep = NULL;
   switch (alu->op) {
   case nir_op_mov:
   case nir_op_vec2:
   case nir_op_vec3:
   case nir_op_vec4:
   case nir_op_bcsel:
      /* These we expect to have integers but the opcode doesn't change */
         case nir_op_b2i32:
      alu->op = nir_op_b2f32;
      case nir_op_i2f32:
      alu->op = nir_op_mov;
      case nir_op_u2f32:
      alu->op = nir_op_mov;
         case nir_op_f2i32: {
               /* If the source was already integer, then we did't need to truncate and
   * can switch it to a mov that can be copy-propagated away.
   */
   nir_alu_instr *src_alu = nir_src_as_alu_instr(alu->src[0].src);
   if (src_alu) {
      switch (src_alu->op) {
   /* Check for the y = x - ffract(x) patterns from lowered ffloor. */
   case nir_op_fadd:
      if (check_for_lowered_ffloor(src_alu))
            case nir_op_fround_even:
   case nir_op_fceil:
   case nir_op_ftrunc:
   case nir_op_ffloor:
      alu->op = nir_op_mov;
      default:
            }
               case nir_op_f2u32:
      alu->op = nir_op_ffloor;
         case nir_op_ilt:
      alu->op = nir_op_flt;
      case nir_op_ige:
      alu->op = nir_op_fge;
      case nir_op_ieq:
      alu->op = nir_op_feq;
      case nir_op_ine:
      alu->op = nir_op_fneu;
      case nir_op_ult:
      alu->op = nir_op_flt;
      case nir_op_uge:
      alu->op = nir_op_fge;
         case nir_op_iadd:
      alu->op = nir_op_fadd;
      case nir_op_isub:
      alu->op = nir_op_fsub;
      case nir_op_imul:
      alu->op = nir_op_fmul;
         case nir_op_idiv: {
      nir_def *x = nir_ssa_for_alu_src(b, alu, 0);
            /* Hand-lower fdiv, since lower_int_to_float is after nir_opt_algebraic. */
   if (b->shader->options->lower_fdiv) {
         } else {
         }
               case nir_op_iabs:
      alu->op = nir_op_fabs;
      case nir_op_ineg:
      alu->op = nir_op_fneg;
      case nir_op_imax:
      alu->op = nir_op_fmax;
      case nir_op_imin:
      alu->op = nir_op_fmin;
      case nir_op_umax:
      alu->op = nir_op_fmax;
      case nir_op_umin:
      alu->op = nir_op_fmin;
         case nir_op_ball_iequal2:
      alu->op = nir_op_ball_fequal2;
      case nir_op_ball_iequal3:
      alu->op = nir_op_ball_fequal3;
      case nir_op_ball_iequal4:
      alu->op = nir_op_ball_fequal4;
      case nir_op_bany_inequal2:
      alu->op = nir_op_bany_fnequal2;
      case nir_op_bany_inequal3:
      alu->op = nir_op_bany_fnequal3;
      case nir_op_bany_inequal4:
      alu->op = nir_op_bany_fnequal4;
         case nir_op_i32csel_gt:
      alu->op = nir_op_fcsel_gt;
      case nir_op_i32csel_ge:
      alu->op = nir_op_fcsel_ge;
         default:
      assert(nir_alu_type_get_base_type(info->output_type) != nir_type_int &&
         for (unsigned i = 0; i < info->num_inputs; i++) {
      assert(nir_alu_type_get_base_type(info->input_types[i]) != nir_type_int &&
      }
               if (rep) {
      /* We've emitted a replacement instruction */
   nir_def_rewrite_uses(&alu->def, rep);
                  }
      static bool
   nir_lower_int_to_float_impl(nir_function_impl *impl)
   {
      bool progress = false;
                     nir_index_ssa_defs(impl);
   float_types = calloc(BITSET_WORDS(impl->ssa_alloc),
         int_types = calloc(BITSET_WORDS(impl->ssa_alloc),
                  nir_foreach_block(block, impl) {
      nir_foreach_instr_safe(instr, block) {
      switch (instr->type) {
   case nir_instr_type_alu:
                  case nir_instr_type_load_const: {
      nir_load_const_instr *load = nir_instr_as_load_const(instr);
   if (load->def.bit_size != 1 && BITSET_TEST(int_types, load->def.index)) {
      for (unsigned i = 0; i < load->def.num_components; i++)
      }
               case nir_instr_type_intrinsic:
   case nir_instr_type_undef:
   case nir_instr_type_phi:
                  default:
      nir_foreach_def(instr, assert_ssa_def_is_not_int, (void *)int_types);
                     if (progress) {
      nir_metadata_preserve(impl, nir_metadata_block_index |
      } else {
                  free(float_types);
               }
      bool
   nir_lower_int_to_float(nir_shader *shader)
   {
               nir_foreach_function_impl(impl, shader) {
      if (nir_lower_int_to_float_impl(impl))
                  }
