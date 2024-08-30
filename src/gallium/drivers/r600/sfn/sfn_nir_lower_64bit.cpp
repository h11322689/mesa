   /* -*- mesa-c++  -*-
   *
   * Copyright (c) 2020 Collabora LTD
   *
   * Author: Gert Wollny <gert.wollny@collabora.com>
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * on the rights to use, copy, modify, merge, publish, distribute, sub
   * license, and/or sell copies of the Software, and to permit persons to whom
   * the Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the next
   * paragraph) shall be included in all copies or substantial portions of the
   * Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
   * THE AUTHOR(S) AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
   * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
   * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
   * USE OR OTHER DEALINGS IN THE SOFTWARE.
   */
      #include "nir.h"
   #include "nir_builder.h"
   #include "sfn_nir.h"
      #include <iostream>
   #include <map>
   #include <vector>
      namespace r600 {
      using std::make_pair;
   using std::map;
   using std::pair;
   using std::vector;
      class LowerSplit64BitVar : public NirLowerInstruction {
   public:
      ~LowerSplit64BitVar();
   using VarSplit = pair<nir_variable *, nir_variable *>;
                           private:
                        nir_def *split_store_deref_array(nir_intrinsic_instr *intr,
                              nir_def *
                                                         nir_def *
            nir_def *
            nir_def *
                              bool filter(const nir_instr *instr) const override;
            VarMap m_varmap;
   vector<nir_variable *> m_old_vars;
      };
      class LowerLoad64Uniform : public NirLowerInstruction {
      bool filter(const nir_instr *instr) const override;
      };
      bool
   LowerLoad64Uniform::filter(const nir_instr *instr) const
   {
      if (instr->type != nir_instr_type_intrinsic)
            auto intr = nir_instr_as_intrinsic(instr);
   if (intr->intrinsic != nir_intrinsic_load_uniform &&
      intr->intrinsic != nir_intrinsic_load_ubo &&
   intr->intrinsic != nir_intrinsic_load_ubo_vec4)
            }
      nir_def *
   LowerLoad64Uniform::lower(nir_instr *instr)
   {
      auto intr = nir_instr_as_intrinsic(instr);
   int old_components = intr->def.num_components;
   assert(old_components <= 2);
   intr->def.num_components *= 2;
   intr->def.bit_size = 32;
            if (intr->intrinsic == nir_intrinsic_load_ubo ||
      intr->intrinsic == nir_intrinsic_load_ubo_vec4)
                  for (int i = 0; i < old_components; ++i) {
      result_vec[i] = nir_pack_64_2x32_split(b,
            }
   if (old_components == 1)
               }
      bool
   r600_split_64bit_uniforms_and_ubo(nir_shader *sh)
   {
         }
      class LowerSplit64op : public NirLowerInstruction {
      bool filter(const nir_instr *instr) const override
   {
      switch (instr->type) {
   case nir_instr_type_alu: {
      auto alu = nir_instr_as_alu(instr);
   switch (alu->op) {
   case nir_op_bcsel:
         case nir_op_f2i32:
   case nir_op_f2u32:
   case nir_op_f2i64:
   case nir_op_f2u64:
   case nir_op_u2f64:
   case nir_op_i2f64:
         default:
            }
   case nir_instr_type_phi: {
      auto phi = nir_instr_as_phi(instr);
      }
   default:
                     nir_def *lower(nir_instr *instr) override
               switch (instr->type) {
   case nir_instr_type_alu: {
                     case nir_op_bcsel: {
      auto lo =
      nir_bcsel(b,
            alu->src[0].src.ssa,
   auto hi =
      nir_bcsel(b,
            alu->src[0].src.ssa,
      }
   case nir_op_f2i32: {
      auto src = nir_ssa_for_alu_src(b, alu, 0);
   auto gt0 = nir_fgt_imm(b, src, 0.0);
   auto abs_src = nir_fabs(b, src);
   auto value = nir_f2u32(b, abs_src);
      }
   case nir_op_f2u32: {
      /* fp32 doesn't hold sufficient bits to represent the full range of
   * u32, therefore we have to split the values, and because f2f32
   * rounds, we have to remove the fractional part in the hi bits
   * For values > UINT_MAX the result is undefined */
   auto src = nir_ssa_for_alu_src(b, alu, 0);
   src = nir_fadd(b, src, nir_fneg(b, nir_ffract(b, src)));
   auto gt0 = nir_fgt_imm(b, src, 0.0);
   auto highval = nir_fmul_imm(b, src, 1.0 / 65536.0);
   auto fract = nir_ffract(b, highval);
   auto high = nir_f2u32(b, nir_f2f32(b, nir_fadd(b, highval, nir_fneg(b, fract))));
   auto lowval = nir_fmul_imm(b, fract, 65536.0);
   auto low = nir_f2u32(b, nir_f2f32(b, lowval));
   return nir_bcsel(b,
                  }        
   case nir_op_u2f64: {
      auto src = nir_ssa_for_alu_src(b, alu, 0);
   auto low = nir_unpack_64_2x32_split_x(b, src);
   auto high = nir_unpack_64_2x32_split_y(b, src);
   auto flow = nir_u2f64(b, low);
   auto fhigh = nir_u2f64(b, high);
      }
   case nir_op_i2f64: {
      auto src = nir_ssa_for_alu_src(b, alu, 0);
   auto low = nir_unpack_64_2x32_split_x(b, src);
   auto high = nir_unpack_64_2x32_split_y(b, src);
   auto flow = nir_u2f64(b, low);
   auto fhigh = nir_i2f64(b, high);
      }
   default:
            }
   case nir_instr_type_phi: {
      auto phi = nir_instr_as_phi(instr);
   auto phi_lo = nir_phi_instr_create(b->shader);
   auto phi_hi = nir_phi_instr_create(b->shader);
   nir_def_init(
         nir_def_init(
         nir_foreach_phi_src(s, phi)
   {
      auto lo = nir_unpack_32_2x16_split_x(b, s->src.ssa);
   auto hi = nir_unpack_32_2x16_split_x(b, s->src.ssa);
   nir_phi_instr_add_src(phi_lo, s->pred, lo);
      }
      }
   default:
               };
      bool
   r600_split_64bit_alu_and_phi(nir_shader *sh)
   {
         }
      bool
   LowerSplit64BitVar::filter(const nir_instr *instr) const
   {
      switch (instr->type) {
   case nir_instr_type_intrinsic: {
               switch (intr->intrinsic) {
   case nir_intrinsic_load_deref:
   case nir_intrinsic_load_uniform:
   case nir_intrinsic_load_input:
   case nir_intrinsic_load_ubo:
   case nir_intrinsic_load_ssbo:
      if (intr->def.bit_size != 64)
            case nir_intrinsic_store_output:
      if (nir_src_bit_size(intr->src[0]) != 64)
            case nir_intrinsic_store_deref:
      if (nir_src_bit_size(intr->src[1]) != 64)
            default:
            }
   case nir_instr_type_alu: {
      auto alu = nir_instr_as_alu(instr);
   switch (alu->op) {
   case nir_op_bcsel:
      if (alu->def.num_components < 3)
            case nir_op_bany_fnequal3:
   case nir_op_bany_fnequal4:
   case nir_op_ball_fequal3:
   case nir_op_ball_fequal4:
   case nir_op_bany_inequal3:
   case nir_op_bany_inequal4:
   case nir_op_ball_iequal3:
   case nir_op_ball_iequal4:
   case nir_op_fdot3:
   case nir_op_fdot4:
         default:
            }
   case nir_instr_type_load_const: {
      auto lc = nir_instr_as_load_const(instr);
   if (lc->def.bit_size != 64)
            }
   default:
            }
      nir_def *
   LowerSplit64BitVar::merge_64bit_loads(nir_def *load1,
               {
      if (out_is_vec3)
      return nir_vec3(b,
                  else
      return nir_vec4(b,
                     }
      LowerSplit64BitVar::~LowerSplit64BitVar()
   {
      for (auto&& v : m_old_vars)
            for (auto&& v : m_old_stores)
      }
      nir_def *
   LowerSplit64BitVar::split_double_store_deref(nir_intrinsic_instr *intr)
   {
      auto deref = nir_instr_as_deref(intr->src[0].ssa->parent_instr);
   if (deref->deref_type == nir_deref_type_var)
         else if (deref->deref_type == nir_deref_type_array)
         else {
            }
      nir_def *
   LowerSplit64BitVar::split_double_load_deref(nir_intrinsic_instr *intr)
   {
      auto deref = nir_instr_as_deref(intr->src[0].ssa->parent_instr);
   if (deref->deref_type == nir_deref_type_var)
         else if (deref->deref_type == nir_deref_type_array)
         else {
         }
      }
      nir_def *
   LowerSplit64BitVar::split_load_deref_array(nir_intrinsic_instr *intr, nir_src& index)
   {
      auto old_var = nir_intrinsic_get_var(intr, 0);
                              auto deref1 = nir_build_deref_var(b, vars.first);
   auto deref_array1 = nir_build_deref_array(b, deref1, index.ssa);
   auto load1 =
            auto deref2 = nir_build_deref_var(b, vars.second);
            auto load2 = nir_build_load_deref(
               }
      nir_def *
   LowerSplit64BitVar::split_store_deref_array(nir_intrinsic_instr *intr,
         {
      auto old_var = nir_intrinsic_get_var(intr, 0);
                                       auto deref1 = nir_build_deref_var(b, vars.first);
   auto deref_array1 =
                     auto deref2 = nir_build_deref_var(b, vars.second);
   auto deref_array2 =
            if (old_components == 3)
      nir_build_store_deref(b,
                  else
      nir_build_store_deref(b,
                        }
      nir_def *
   LowerSplit64BitVar::split_store_deref_var(nir_intrinsic_instr *intr,
         {
      auto old_var = nir_intrinsic_get_var(intr, 0);
                                       auto deref1 = nir_build_deref_var(b, vars.first);
            auto deref2 = nir_build_deref_var(b, vars.second);
   if (old_components == 3)
         else
      nir_build_store_deref(b,
                        }
      nir_def *
   LowerSplit64BitVar::split_load_deref_var(nir_intrinsic_instr *intr)
   {
      auto old_var = nir_intrinsic_get_var(intr, 0);
   auto vars = get_var_pair(old_var);
            nir_deref_instr *deref1 = nir_build_deref_var(b, vars.first);
            nir_deref_instr *deref2 = nir_build_deref_var(b, vars.second);
                        }
      LowerSplit64BitVar::VarSplit
   LowerSplit64BitVar::get_var_pair(nir_variable *old_var)
   {
                        if (split_vars == m_varmap.end()) {
      auto var1 = nir_variable_clone(old_var, b->shader);
            var1->type = glsl_dvec_type(2);
            if (old_var->type->is_array()) {
      var1->type = glsl_array_type(var1->type, old_var->type->array_size(), 0);
               if (old_var->data.mode == nir_var_shader_in ||
      old_var->data.mode == nir_var_shader_out) {
   ++var2->data.driver_location;
   ++var2->data.location;
   nir_shader_add_variable(b->shader, var1);
      } else if (old_var->data.mode == nir_var_function_temp) {
      exec_list_push_tail(&b->impl->locals, &var1->node);
                  }
      }
      nir_def *
   LowerSplit64BitVar::split_double_load(nir_intrinsic_instr *load1)
   {
      unsigned old_components = load1->def.num_components;
   auto load2 = nir_instr_as_intrinsic(nir_instr_clone(b->shader, &load1->instr));
            load1->def.num_components = 2;
   sem.num_slots = 1;
            load2->def.num_components = old_components - 2;
   sem.location += 1;
   nir_intrinsic_set_io_semantics(load2, sem);
   nir_intrinsic_set_base(load2, nir_intrinsic_base(load1) + 1);
               }
      nir_def *
   LowerSplit64BitVar::split_store_output(nir_intrinsic_instr *store1)
   {
      auto src = store1->src[0];
   unsigned old_components = nir_src_num_components(src);
            auto store2 = nir_instr_as_intrinsic(nir_instr_clone(b->shader, &store1->instr));
   auto src1 = nir_trim_vector(b, src.ssa, 2);
            nir_src_rewrite(&src, src1);
            nir_src_rewrite(&src, src2);
            sem.num_slots = 1;
            sem.location += 1;
   nir_intrinsic_set_io_semantics(store2, sem);
            nir_builder_instr_insert(b, &store2->instr);
      }
      nir_def *
   LowerSplit64BitVar::split_double_load_uniform(nir_intrinsic_instr *intr)
   {
      unsigned second_components = intr->def.num_components - 2;
   nir_intrinsic_instr *load2 =
         load2->src[0] = nir_src_for_ssa(nir_iadd_imm(b, intr->src[0].ssa, 1));
   nir_intrinsic_set_dest_type(load2, nir_intrinsic_dest_type(intr));
   nir_intrinsic_set_base(load2, nir_intrinsic_base(intr));
   nir_intrinsic_set_range(load2, nir_intrinsic_range(intr));
            nir_def_init(&load2->instr, &load2->def, second_components, 64);
                     if (second_components == 1)
      return nir_vec3(b,
                  else
      return nir_vec4(b,
                     }
      nir_def *
   LowerSplit64BitVar::split_double_load_ssbo(nir_intrinsic_instr *intr)
   {
      unsigned second_components = intr->def.num_components - 2;
   nir_intrinsic_instr *load2 =
            nir_src_rewrite(&load2->src[0], nir_iadd_imm(b, intr->src[0].ssa, 1));
   load2->num_components = second_components;
            nir_intrinsic_set_dest_type(load2, nir_intrinsic_dest_type(intr));
                        }
      nir_def *
   LowerSplit64BitVar::split_double_load_ubo(nir_intrinsic_instr *intr)
   {
      unsigned second_components = intr->def.num_components - 2;
   nir_intrinsic_instr *load2 =
         load2->src[0] = intr->src[0];
   load2->src[1] = nir_src_for_ssa(nir_iadd_imm(b, intr->src[1].ssa, 16));
   nir_intrinsic_set_range_base(load2, nir_intrinsic_range_base(intr) + 16);
   nir_intrinsic_set_range(load2, nir_intrinsic_range(intr));
   nir_intrinsic_set_access(load2, nir_intrinsic_access(intr));
   nir_intrinsic_set_align_mul(load2, nir_intrinsic_align_mul(intr));
                     nir_def_init(&load2->instr, &load2->def, second_components, 64);
                        }
      nir_def *
   LowerSplit64BitVar::split_reduction(nir_def *src[2][2],
                     {
      auto cmp0 = nir_build_alu(b, op1, src[0][0], src[0][1], nullptr, nullptr);
   auto cmp1 = nir_build_alu(b, op2, src[1][0], src[1][1], nullptr, nullptr);
      }
      nir_def *
   LowerSplit64BitVar::split_reduction3(nir_alu_instr *alu,
                     {
               src[0][0] = nir_trim_vector(b, alu->src[0].src.ssa, 2);
            src[1][0] = nir_channel(b, alu->src[0].src.ssa, 2);
               }
      nir_def *
   LowerSplit64BitVar::split_reduction4(nir_alu_instr *alu,
                     {
               src[0][0] = nir_trim_vector(b, alu->src[0].src.ssa, 2);
            src[1][0] = nir_channels(b, alu->src[0].src.ssa, 0xc);
               }
      nir_def *
   LowerSplit64BitVar::split_bcsel(nir_alu_instr *alu)
   {
      static nir_def *dest[4];
   for (unsigned i = 0; i < alu->def.num_components; ++i) {
      dest[i] = nir_bcsel(b,
                  }
      }
      nir_def *
   LowerSplit64BitVar::split_load_const(nir_load_const_instr *lc)
   {
      nir_def *ir[4];
   for (unsigned i = 0; i < lc->def.num_components; ++i)
               }
      nir_def *
   LowerSplit64BitVar::lower(nir_instr *instr)
   {
      switch (instr->type) {
   case nir_instr_type_intrinsic: {
      auto intr = nir_instr_as_intrinsic(instr);
   switch (intr->intrinsic) {
   case nir_intrinsic_load_deref:
         case nir_intrinsic_load_uniform:
         case nir_intrinsic_load_ubo:
         case nir_intrinsic_load_ssbo:
         case nir_intrinsic_load_input:
         case nir_intrinsic_store_output:
         case nir_intrinsic_store_deref:
         default:
            }
   case nir_instr_type_alu: {
      auto alu = nir_instr_as_alu(instr);
   switch (alu->op) {
   case nir_op_bany_fnequal3:
         case nir_op_ball_fequal3:
         case nir_op_bany_inequal3:
         case nir_op_ball_iequal3:
         case nir_op_fdot3:
         case nir_op_bany_fnequal4:
      return split_reduction4(alu,
                  case nir_op_ball_fequal4:
      return split_reduction4(alu,
                  case nir_op_bany_inequal4:
      return split_reduction4(alu,
                  case nir_op_ball_iequal4:
      return split_reduction4(alu,
                  case nir_op_fdot4:
         case nir_op_bcsel:
         default:
            }
   case nir_instr_type_load_const: {
      auto lc = nir_instr_as_load_const(instr);
      }
   default:
         }
      }
      /* Split 64 bit instruction so that at most two 64 bit components are
   * used in one instruction */
      bool
   r600_nir_split_64bit_io(nir_shader *sh)
   {
         }
      /* */
   class Lower64BitToVec2 : public NirLowerInstruction {
      private:
      bool filter(const nir_instr *instr) const override;
            nir_def *load_deref_64_to_vec2(nir_intrinsic_instr *intr);
   nir_def *load_uniform_64_to_vec2(nir_intrinsic_instr *intr);
   nir_def *load_ssbo_64_to_vec2(nir_intrinsic_instr *intr);
   nir_def *load_64_to_vec2(nir_intrinsic_instr *intr);
      };
      bool
   Lower64BitToVec2::filter(const nir_instr *instr) const
   {
      switch (instr->type) {
   case nir_instr_type_intrinsic: {
               switch (intr->intrinsic) {
   case nir_intrinsic_load_deref:
   case nir_intrinsic_load_input:
   case nir_intrinsic_load_uniform:
   case nir_intrinsic_load_ubo:
   case nir_intrinsic_load_global:
   case nir_intrinsic_load_global_constant:
   case nir_intrinsic_load_ubo_vec4:
   case nir_intrinsic_load_ssbo:
         case nir_intrinsic_store_deref: {
      if (nir_src_bit_size(intr->src[1]) == 64)
         auto var = nir_intrinsic_get_var(intr, 0);
   if (var->type->without_array()->bit_size() == 64)
            }
   case nir_intrinsic_store_global:
         default:
            }
   case nir_instr_type_alu: {
      auto alu = nir_instr_as_alu(instr);
      }
   case nir_instr_type_phi: {
      auto phi = nir_instr_as_phi(instr);
      }
   case nir_instr_type_load_const: {
      auto lc = nir_instr_as_load_const(instr);
      }
   case nir_instr_type_undef: {
      auto undef = nir_instr_as_undef(instr);
      }
   default:
            }
      nir_def *
   Lower64BitToVec2::lower(nir_instr *instr)
   {
      switch (instr->type) {
   case nir_instr_type_intrinsic: {
      auto intr = nir_instr_as_intrinsic(instr);
   switch (intr->intrinsic) {
   case nir_intrinsic_load_deref:
         case nir_intrinsic_load_uniform:
         case nir_intrinsic_load_ssbo:
         case nir_intrinsic_load_input:
   case nir_intrinsic_load_global:
   case nir_intrinsic_load_global_constant:
   case nir_intrinsic_load_ubo:
   case nir_intrinsic_load_ubo_vec4:
         case nir_intrinsic_store_deref:
                           }
   case nir_instr_type_alu: {
      auto alu = nir_instr_as_alu(instr);
   alu->def.bit_size = 32;
   alu->def.num_components *= 2;
   switch (alu->op) {
   case nir_op_pack_64_2x32_split:
      alu->op = nir_op_vec2;
      case nir_op_pack_64_2x32:
      alu->op = nir_op_mov;
      case nir_op_vec2:
      return nir_vec4(b,
                  nir_channel(b, alu->src[0].src.ssa, 0),
   default:
         }
      }
   case nir_instr_type_phi: {
      auto phi = nir_instr_as_phi(instr);
   phi->def.bit_size = 32;
   phi->def.num_components = 2;
      }
   case nir_instr_type_load_const: {
      auto lc = nir_instr_as_load_const(instr);
   assert(lc->def.num_components <= 2);
   nir_const_value val[4];
   for (uint i = 0; i < lc->def.num_components; ++i) {
      uint64_t v = lc->value[i].u64;
   val[i * 2 + 0] = nir_const_value_for_uint(v & 0xffffffff, 32);
                  }
   case nir_instr_type_undef: {
      auto undef = nir_instr_as_undef(instr);
   undef->def.num_components *= 2;
   undef->def.bit_size = 32;
      }
   default:
            }
      nir_def *
   Lower64BitToVec2::load_deref_64_to_vec2(nir_intrinsic_instr *intr)
   {
      auto deref = nir_instr_as_deref(intr->src[0].ssa->parent_instr);
   auto var = nir_intrinsic_get_var(intr, 0);
   unsigned components = var->type->without_array()->components();
   if (var->type->without_array()->bit_size() == 64) {
      components *= 2;
   if (deref->deref_type == nir_deref_type_var) {
                                 } else {
      nir_print_shader(b->shader, stderr);
         }
   deref->type = var->type;
   if (deref->deref_type == nir_deref_type_array) {
      auto deref_array = nir_instr_as_deref(deref->parent.ssa->parent_instr);
   deref_array->type = var->type;
               intr->num_components = components;
   intr->def.bit_size = 32;
   intr->def.num_components = components;
      }
      nir_def *
   Lower64BitToVec2::store_64_to_vec2(nir_intrinsic_instr *intr)
   {
      auto deref = nir_instr_as_deref(intr->src[0].ssa->parent_instr);
            unsigned components = var->type->without_array()->components();
   unsigned wrmask = nir_intrinsic_write_mask(intr);
   if (var->type->without_array()->bit_size() == 64) {
      components *= 2;
   if (deref->deref_type == nir_deref_type_var) {
         } else if (deref->deref_type == nir_deref_type_array) {
      var->type =
      } else {
      nir_print_shader(b->shader, stderr);
         }
   deref->type = var->type;
   if (deref->deref_type == nir_deref_type_array) {
      auto deref_array = nir_instr_as_deref(deref->parent.ssa->parent_instr);
   deref_array->type = var->type;
      }
   intr->num_components = components;
   nir_intrinsic_set_write_mask(intr, wrmask == 1 ? 3 : 0xf);
      }
      nir_def *
   Lower64BitToVec2::load_uniform_64_to_vec2(nir_intrinsic_instr *intr)
   {
      intr->num_components *= 2;
   intr->def.bit_size = 32;
   intr->def.num_components *= 2;
   nir_intrinsic_set_dest_type(intr, nir_type_float32);
      }
      nir_def *
   Lower64BitToVec2::load_64_to_vec2(nir_intrinsic_instr *intr)
   {
      intr->num_components *= 2;
   intr->def.bit_size = 32;
   intr->def.num_components *= 2;
   if (nir_intrinsic_has_component(intr))
            }
      nir_def *
   Lower64BitToVec2::load_ssbo_64_to_vec2(nir_intrinsic_instr *intr)
   {
      intr->num_components *= 2;
   intr->def.bit_size = 32;
   intr->def.num_components *= 2;
      }
      static bool
   store_64bit_intr(nir_src *src, void *state)
   {
      bool *s = (bool *)state;
   *s = nir_src_bit_size(*src) == 64;
      }
      static bool
   double2vec2(nir_src *src, UNUSED void *state)
   {
      if (nir_src_bit_size(*src) != 64)
            src->ssa->bit_size = 32;
   src->ssa->num_components *= 2;
      }
      bool
   r600_nir_64_to_vec2(nir_shader *sh)
   {
      vector<nir_instr *> intr64bit;
   nir_foreach_function_impl(impl, sh)
   {
      nir_foreach_block(block, impl)
   {
      nir_foreach_instr_safe(instr, block)
   {
      switch (instr->type) {
   case nir_instr_type_alu: {
      bool success = false;
   nir_foreach_src(instr, store_64bit_intr, &success);
   if (success)
            }
   case nir_instr_type_intrinsic: {
      auto ir = nir_instr_as_intrinsic(instr);
   switch (ir->intrinsic) {
   case nir_intrinsic_store_output:
   case nir_intrinsic_store_global:
   case nir_intrinsic_store_ssbo: {
      bool success = false;
   nir_foreach_src(instr, store_64bit_intr, &success);
   if (success) {
      auto wm = nir_intrinsic_write_mask(ir);
   nir_intrinsic_set_write_mask(ir, (wm == 1) ? 3 : 0xf);
      }
      }
   default:;
      }
   default:;
                                          for (auto&& instr : intr64bit) {
      if (instr->type == nir_instr_type_alu) {
      auto alu = nir_instr_as_alu(instr);
   auto alu_info = nir_op_infos[alu->op];
   for (unsigned i = 0; i < alu_info.num_inputs; ++i) {
      int swizzle[NIR_MAX_VEC_COMPONENTS] = {0};
   for (unsigned k = 0; k < NIR_MAX_VEC_COMPONENTS / 2; k++) {
                           switch (alu->op) {
   case nir_op_unpack_64_2x32_split_x:
      swizzle[2 * k] = alu->src[i].swizzle[k] * 2;
   alu->op = nir_op_mov;
      case nir_op_unpack_64_2x32_split_y:
      swizzle[2 * k] = alu->src[i].swizzle[k] * 2 + 1;
   alu->op = nir_op_mov;
      case nir_op_unpack_64_2x32:
      alu->op = nir_op_mov;
      case nir_op_bcsel:
      if (i == 0) {
      swizzle[2 * k] = swizzle[2 * k + 1] = alu->src[i].swizzle[k] * 2;
      }
      default:
      swizzle[2 * k] = alu->src[i].swizzle[k] * 2;
         }
   for (unsigned k = 0; k < NIR_MAX_VEC_COMPONENTS; ++k) {
               } else
      }
                  }
      using std::map;
   using std::pair;
   using std::vector;
      class StoreMerger {
   public:
      StoreMerger(nir_shader *shader);
   void collect_stores();
   bool combine();
                     StoreCombos m_stores;
      };
      StoreMerger::StoreMerger(nir_shader *shader):
         {
   }
      void
   StoreMerger::collect_stores()
   {
      unsigned vertex = 0;
   nir_foreach_function_impl(impl, sh)
   {
      nir_foreach_block(block, impl)
   {
      nir_foreach_instr_safe(instr, block)
   {
                     auto ir = nir_instr_as_intrinsic(instr);
   if (ir->intrinsic == nir_intrinsic_emit_vertex ||
      ir->intrinsic == nir_intrinsic_emit_vertex_with_counter) {
   ++vertex;
      }
                  unsigned index = nir_intrinsic_base(ir) + 64 * vertex +
                     }
      bool
   StoreMerger::combine()
   {
      bool progress = false;
   for (auto&& i : m_stores) {
      if (i.second.size() < 2)
            combine_one_slot(i.second);
      }
      }
      void
   StoreMerger::combine_one_slot(vector<nir_intrinsic_instr *>& stores)
   {
                                 unsigned comps = 0;
   unsigned writemask = 0;
   unsigned first_comp = 4;
   for (auto&& store : stores) {
      int cmp = nir_intrinsic_component(store);
   for (unsigned i = 0; i < nir_src_num_components(store->src[0]); ++i, ++comps) {
      unsigned out_comp = i + cmp;
   srcs[out_comp] = nir_channel(&b, store->src[0].ssa, i);
   writemask |= 1 << out_comp;
   if (first_comp > out_comp)
                           nir_src_rewrite(&last_store->src[0], new_src);
   last_store->num_components = comps;
   nir_intrinsic_set_component(last_store, first_comp);
            for (auto i = stores.begin(); i != stores.end() - 1; ++i)
      }
      bool
   r600_merge_vec2_stores(nir_shader *shader)
   {
      r600::StoreMerger merger(shader);
   merger.collect_stores();
      }
      static bool
   r600_lower_64bit_intrinsic(nir_builder *b, nir_intrinsic_instr *instr)
   {
               switch (instr->intrinsic) {
   case nir_intrinsic_load_ubo:
   case nir_intrinsic_load_ubo_vec4:
   case nir_intrinsic_load_uniform:
   case nir_intrinsic_load_ssbo:
   case nir_intrinsic_load_input:
   case nir_intrinsic_load_interpolated_input:
   case nir_intrinsic_load_per_vertex_input:
   case nir_intrinsic_store_output:
   case nir_intrinsic_store_per_vertex_output:
   case nir_intrinsic_store_ssbo:
         default:
                  if (instr->num_components <= 2)
            bool has_dest = nir_intrinsic_infos[instr->intrinsic].has_dest;
   if (has_dest) {
      if (instr->def.bit_size != 64)
      } else {
      if (nir_src_bit_size(instr->src[0]) != 64)
               nir_intrinsic_instr *first =
         nir_intrinsic_instr *second =
            switch (instr->intrinsic) {
   case nir_intrinsic_load_ubo:
   case nir_intrinsic_load_ubo_vec4:
   case nir_intrinsic_load_uniform:
   case nir_intrinsic_load_ssbo:
   case nir_intrinsic_store_ssbo:
            default: {
      nir_io_semantics semantics = nir_intrinsic_io_semantics(second);
   semantics.location++;
   semantics.num_slots--;
            nir_intrinsic_set_base(second, nir_intrinsic_base(second) + 1);
      }
            first->num_components = 2;
   second->num_components -= 2;
   if (has_dest) {
      first->def.num_components = 2;
               nir_builder_instr_insert(b, &first->instr);
            if (has_dest) {
      /* Merge the two loads' results back into a vector. */
   nir_scalar channels[4] = {
      nir_get_scalar(&first->def, 0),
   nir_get_scalar(&first->def, 1),
   nir_get_scalar(&second->def, 0),
      };
   nir_def *new_ir = nir_vec_scalars(b, channels, instr->num_components);
      } else {
      /* Split the src value across the two stores. */
            nir_def *src0 = instr->src[0].ssa;
   nir_scalar channels[4] = {{0}};
   for (int i = 0; i < instr->num_components; i++)
            nir_intrinsic_set_write_mask(first, nir_intrinsic_write_mask(instr) & 3);
            nir_src_rewrite(&first->src[0], nir_vec_scalars(b, channels, 2));
   nir_src_rewrite(&second->src[0],
               int offset_src = -1;
            switch (instr->intrinsic) {
   case nir_intrinsic_load_ssbo:
   case nir_intrinsic_load_ubo:
      offset_src = 1;
      case nir_intrinsic_load_ubo_vec4:
   case nir_intrinsic_load_uniform:
      offset_src = 0;
   offset_amount = 1;
      case nir_intrinsic_store_ssbo:
      offset_src = 2;
      default:
         }
   if (offset_src != -1) {
      b->cursor = nir_before_instr(&second->instr);
   nir_def *second_offset =
                     /* DCE stores we generated with no writemask (nothing else does this
   * currently).
   */
   if (!has_dest) {
      if (nir_intrinsic_write_mask(first) == 0)
         if (nir_intrinsic_write_mask(second) == 0)
                           }
      static bool
   r600_lower_64bit_load_const(nir_builder *b, nir_load_const_instr *instr)
   {
               if (instr->def.bit_size != 64 || num_components <= 2)
                     nir_load_const_instr *first = nir_load_const_instr_create(b->shader, 2, 64);
   nir_load_const_instr *second =
            first->value[0] = instr->value[0];
   first->value[1] = instr->value[1];
   second->value[0] = instr->value[2];
   if (num_components == 4)
            nir_builder_instr_insert(b, &first->instr);
            nir_def *channels[4] = {
      nir_channel(b, &first->def, 0),
   nir_channel(b, &first->def, 1),
   nir_channel(b, &second->def, 0),
      };
   nir_def *new_ir = nir_vec(b, channels, num_components);
   nir_def_rewrite_uses(&instr->def, new_ir);
               }
      static bool
   r600_lower_64bit_to_vec2_instr(nir_builder *b, nir_instr *instr, UNUSED void *data)
   {
      switch (instr->type) {
   case nir_instr_type_load_const:
            case nir_instr_type_intrinsic:
         default:
            }
      bool
   r600_lower_64bit_to_vec2(nir_shader *s)
   {
      return nir_shader_instructions_pass(s,
                  }
      } // end namespace r600
