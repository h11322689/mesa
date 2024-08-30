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
      #include "nir.h"
   #include "nir_builder.h"
      static bool
   assert_ssa_def_is_not_1bit(nir_def *def, UNUSED void *unused)
   {
      assert(def->bit_size > 1);
      }
      static bool
   rewrite_1bit_ssa_def_to_32bit(nir_def *def, void *_progress)
   {
      bool *progress = _progress;
   if (def->bit_size == 1) {
      def->bit_size = 32;
      }
      }
      static bool
   lower_alu_instr(nir_builder *b, nir_alu_instr *alu, bool has_fcsel_ne,
         {
                        /* Replacement SSA value */
   nir_def *rep = NULL;
   switch (alu->op) {
   case nir_op_mov:
   case nir_op_vec2:
   case nir_op_vec3:
   case nir_op_vec4:
   case nir_op_vec5:
   case nir_op_vec8:
   case nir_op_vec16:
      if (alu->def.bit_size != 1)
         /* These we expect to have booleans but the opcode doesn't change */
         case nir_op_b2f32:
      alu->op = nir_op_mov;
      case nir_op_b2i32:
      alu->op = nir_op_mov;
      case nir_op_b2b1:
      alu->op = nir_op_mov;
         case nir_op_flt:
      alu->op = nir_op_slt;
      case nir_op_fge:
      alu->op = nir_op_sge;
      case nir_op_feq:
      alu->op = nir_op_seq;
      case nir_op_fneu:
      alu->op = nir_op_sne;
      case nir_op_ilt:
      alu->op = nir_op_slt;
      case nir_op_ige:
      alu->op = nir_op_sge;
      case nir_op_ieq:
      alu->op = nir_op_seq;
      case nir_op_ine:
      alu->op = nir_op_sne;
      case nir_op_ult:
      alu->op = nir_op_slt;
      case nir_op_uge:
      alu->op = nir_op_sge;
         case nir_op_ball_fequal2:
      alu->op = nir_op_fall_equal2;
      case nir_op_ball_fequal3:
      alu->op = nir_op_fall_equal3;
      case nir_op_ball_fequal4:
      alu->op = nir_op_fall_equal4;
      case nir_op_bany_fnequal2:
      alu->op = nir_op_fany_nequal2;
      case nir_op_bany_fnequal3:
      alu->op = nir_op_fany_nequal3;
      case nir_op_bany_fnequal4:
      alu->op = nir_op_fany_nequal4;
      case nir_op_ball_iequal2:
      alu->op = nir_op_fall_equal2;
      case nir_op_ball_iequal3:
      alu->op = nir_op_fall_equal3;
      case nir_op_ball_iequal4:
      alu->op = nir_op_fall_equal4;
      case nir_op_bany_inequal2:
      alu->op = nir_op_fany_nequal2;
      case nir_op_bany_inequal3:
      alu->op = nir_op_fany_nequal3;
      case nir_op_bany_inequal4:
      alu->op = nir_op_fany_nequal4;
         case nir_op_bcsel:
      if (has_fcsel_gt)
         else if (has_fcsel_ne)
         else {
      /* Only a few pre-VS 4.0 platforms (e.g., r300 vertex shaders) should
   * hit this path.
   */
   rep = nir_flrp(b,
                                 case nir_op_iand:
      alu->op = nir_op_fmul;
      case nir_op_ixor:
      alu->op = nir_op_sne;
      case nir_op_ior:
      alu->op = nir_op_fmax;
         case nir_op_inot:
      rep = nir_seq(b, nir_ssa_for_alu_src(b, alu, 0),
               default:
      assert(alu->def.bit_size > 1);
   for (unsigned i = 0; i < op_info->num_inputs; i++)
                     if (rep) {
      /* We've emitted a replacement instruction */
   nir_def_rewrite_uses(&alu->def, rep);
      } else {
      if (alu->def.bit_size == 1)
                  }
      static bool
   lower_tex_instr(nir_tex_instr *tex)
   {
      bool progress = false;
   rewrite_1bit_ssa_def_to_32bit(&tex->def, &progress);
   if (tex->dest_type == nir_type_bool1) {
      tex->dest_type = nir_type_bool32;
      }
      }
      struct lower_bool_to_float_data {
      bool has_fcsel_ne;
      };
      static bool
   nir_lower_bool_to_float_instr(nir_builder *b,
               {
               switch (instr->type) {
   case nir_instr_type_alu:
      return lower_alu_instr(b, nir_instr_as_alu(instr),
         case nir_instr_type_load_const: {
      nir_load_const_instr *load = nir_instr_as_load_const(instr);
   if (load->def.bit_size == 1) {
      nir_const_value *value = load->value;
   for (unsigned i = 0; i < load->def.num_components; i++)
         load->def.bit_size = 32;
      }
               case nir_instr_type_intrinsic:
   case nir_instr_type_undef:
   case nir_instr_type_phi: {
      bool progress = false;
   nir_foreach_def(instr, rewrite_1bit_ssa_def_to_32bit, &progress);
               case nir_instr_type_tex:
            default:
      nir_foreach_def(instr, assert_ssa_def_is_not_1bit, NULL);
         }
      bool
   nir_lower_bool_to_float(nir_shader *shader, bool has_fcsel_ne)
   {
      struct lower_bool_to_float_data data = {
      .has_fcsel_ne = has_fcsel_ne,
               return nir_shader_instructions_pass(shader, nir_lower_bool_to_float_instr,
                  }
