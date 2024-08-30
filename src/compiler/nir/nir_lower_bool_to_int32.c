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
   lower_alu_instr(nir_alu_instr *alu)
   {
               switch (alu->op) {
   case nir_op_mov:
   case nir_op_vec2:
   case nir_op_vec3:
   case nir_op_vec4:
   case nir_op_vec5:
   case nir_op_vec8:
   case nir_op_vec16:
   case nir_op_inot:
   case nir_op_iand:
   case nir_op_ior:
   case nir_op_ixor:
      if (alu->def.bit_size != 1)
         /* These we expect to have booleans but the opcode doesn't change */
         case nir_op_b2b32:
   case nir_op_b2b1:
      /* We're mutating instructions in a dominance-preserving order so our
   * source boolean should be 32-bit by now.
   */
   assert(nir_src_bit_size(alu->src[0].src) == 32);
   alu->op = nir_op_mov;
         case nir_op_flt:
      alu->op = nir_op_flt32;
      case nir_op_fge:
      alu->op = nir_op_fge32;
      case nir_op_feq:
      alu->op = nir_op_feq32;
      case nir_op_fneu:
      alu->op = nir_op_fneu32;
      case nir_op_ilt:
      alu->op = nir_op_ilt32;
      case nir_op_ige:
      alu->op = nir_op_ige32;
      case nir_op_ieq:
      alu->op = nir_op_ieq32;
      case nir_op_ine:
      alu->op = nir_op_ine32;
      case nir_op_ult:
      alu->op = nir_op_ult32;
      case nir_op_uge:
      alu->op = nir_op_uge32;
         case nir_op_ball_fequal2:
      alu->op = nir_op_b32all_fequal2;
      case nir_op_ball_fequal3:
      alu->op = nir_op_b32all_fequal3;
      case nir_op_ball_fequal4:
      alu->op = nir_op_b32all_fequal4;
      case nir_op_bany_fnequal2:
      alu->op = nir_op_b32any_fnequal2;
      case nir_op_bany_fnequal3:
      alu->op = nir_op_b32any_fnequal3;
      case nir_op_bany_fnequal4:
      alu->op = nir_op_b32any_fnequal4;
      case nir_op_ball_iequal2:
      alu->op = nir_op_b32all_iequal2;
      case nir_op_ball_iequal3:
      alu->op = nir_op_b32all_iequal3;
      case nir_op_ball_iequal4:
      alu->op = nir_op_b32all_iequal4;
      case nir_op_bany_inequal2:
      alu->op = nir_op_b32any_inequal2;
      case nir_op_bany_inequal3:
      alu->op = nir_op_b32any_inequal3;
      case nir_op_bany_inequal4:
      alu->op = nir_op_b32any_inequal4;
         case nir_op_bcsel:
      alu->op = nir_op_b32csel;
         case nir_op_fisfinite:
      alu->op = nir_op_fisfinite32;
         default:
      assert(alu->def.bit_size > 1);
   for (unsigned i = 0; i < op_info->num_inputs; i++)
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
      static bool
   nir_lower_bool_to_int32_instr(UNUSED nir_builder *b,
               {
      switch (instr->type) {
   case nir_instr_type_alu:
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
   nir_lower_bool_to_int32(nir_shader *shader)
   {
      bool progress = false;
   nir_foreach_function(func, shader) {
      for (unsigned idx = 0; idx < func->num_params; idx++) {
      nir_parameter *param = &func->params[idx];
   if (param->bit_size == 1) {
      param->bit_size = 32;
                     progress |=
      nir_shader_instructions_pass(shader, nir_lower_bool_to_int32_instr,
                     }
