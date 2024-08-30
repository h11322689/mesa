   /* -*- mesa-c++  -*-
   *
   * Copyright (c) 2022 Collabora LTD
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
      #include "sfn_shader_fs.h"
      #include "sfn_debug.h"
   #include "sfn_instr_alugroup.h"
   #include "sfn_instr_export.h"
   #include "sfn_instr_fetch.h"
   #include "sfn_instr_tex.h"
      #include <sstream>
      namespace r600 {
      using std::string;
      FragmentShader::FragmentShader(const r600_shader_key& key):
      Shader("FS", key.ps.first_atomic_counter),
   m_dual_source_blend(key.ps.dual_source_blend),
   m_max_color_exports(MAX2(key.ps.nr_cbufs, 1)),
   m_pos_input(127, false),
   m_fs_write_all(false),
   m_apply_sample_mask(key.ps.apply_sample_id_mask),
   m_rat_base(key.ps.nr_cbufs),
      {
   }
      void
   FragmentShader::do_get_shader_info(r600_shader *sh_info)
   {
               sh_info->ps_color_export_mask = m_color_export_mask;
   sh_info->ps_export_highest = m_export_highest;
                     sh_info->rat_base = m_rat_base;
   sh_info->uses_kill = m_uses_discard;
   sh_info->gs_prim_id_input = m_gs_prim_id_input;
   if (chip_class() >= ISA_CC_EVERGREEN)
         sh_info->nsys_inputs = m_nsys_inputs;
      }
      bool
   FragmentShader::load_input(nir_intrinsic_instr *intr)
   {
               auto location = nir_intrinsic_io_semantics(intr).location;
   if (location == VARYING_SLOT_POS) {
      AluInstr *ir = nullptr;
   for (unsigned i = 0; i < intr->def.num_components; ++i) {
      ir = new AluInstr(op1_mov,
                        }
   ir->set_alu_flag(alu_last_instr);
               if (location == VARYING_SLOT_FACE) {
      auto ir = new AluInstr(op2_setgt_dx10,
                           emit_instruction(ir);
                  }
      bool
   FragmentShader::store_output(nir_intrinsic_instr *intr)
   {
               if (location == FRAG_RESULT_COLOR && !m_dual_source_blend) {
                     }
      unsigned
   barycentric_ij_index(nir_intrinsic_instr *intr)
   {
      unsigned index = 0;
   switch (intr->intrinsic) {
   case nir_intrinsic_load_barycentric_sample:
      index = 0;
      case nir_intrinsic_load_barycentric_at_sample:
   case nir_intrinsic_load_barycentric_at_offset:
   case nir_intrinsic_load_barycentric_pixel:
      index = 1;
      case nir_intrinsic_load_barycentric_centroid:
      index = 2;
      default:
                  switch (nir_intrinsic_interp_mode(intr)) {
   case INTERP_MODE_NONE:
   case INTERP_MODE_SMOOTH:
   case INTERP_MODE_COLOR:
         case INTERP_MODE_NOPERSPECTIVE:
         case INTERP_MODE_FLAT:
   case INTERP_MODE_EXPLICIT:
   default:
         }
      }
      bool
   FragmentShader::process_stage_intrinsic(nir_intrinsic_instr *intr)
   {
      if (process_stage_intrinsic_hw(intr))
            switch (intr->intrinsic) {
   case nir_intrinsic_load_input:
         case nir_intrinsic_load_interpolated_input:
         case nir_intrinsic_discard_if:
      m_uses_discard = true;
   emit_instruction(new AluInstr(op2_killne_int,
                                 case nir_intrinsic_discard:
      m_uses_discard = true;
   emit_instruction(new AluInstr(op2_kille_int,
                              case nir_intrinsic_load_sample_mask_in:
      if (m_apply_sample_mask) {
         } else
      case nir_intrinsic_load_sample_id:
         case nir_intrinsic_load_helper_invocation:
         case nir_intrinsic_load_sample_pos:
         default:
            }
      bool
   FragmentShader::load_interpolated_input(nir_intrinsic_instr *intr)
   {
      auto& vf = value_factory();
   unsigned loc = nir_intrinsic_io_semantics(intr).location;
   switch (loc) {
   case VARYING_SLOT_POS:
      for (unsigned i = 0; i < intr->def.num_components; ++i)
            case VARYING_SLOT_FACE:
         default:;
               }
      int
   FragmentShader::do_allocate_reserved_registers()
   {
               if (m_sv_values.test(es_pos)) {
      set_input_gpr(m_pos_driver_loc, next_register);
               int face_reg_index = -1;
   if (m_sv_values.test(es_face)) {
      set_input_gpr(m_face_driver_loc, next_register);
   face_reg_index = next_register++;
               if (m_sv_values.test(es_sample_mask_in)) {
      if (face_reg_index < 0)
         m_sample_mask_reg = value_factory().allocate_pinned_register(face_reg_index, 2);
   sfn_log << SfnLog::io << "Set sample mask in register to " << *m_sample_mask_reg
         m_nsys_inputs = 1;
   ShaderInput input(ninputs(), TGSI_SEMANTIC_SAMPLEMASK);
   input.set_gpr(face_reg_index);
               if (m_sv_values.test(es_sample_id) || m_sv_values.test(es_sample_mask_in)) {
      int sample_id_reg = next_register++;
   m_sample_id_reg = value_factory().allocate_pinned_register(sample_id_reg, 3);
   sfn_log << SfnLog::io << "Set sample id register to " << *m_sample_id_reg << "\n";
   m_nsys_inputs++;
   ShaderInput input(ninputs(), TGSI_SEMANTIC_SAMPLEID);
   input.set_gpr(sample_id_reg);
               if (m_sv_values.test(es_helper_invocation)) {
                     }
      bool
   FragmentShader::do_scan_instruction(nir_instr *instr)
   {
      if (instr->type != nir_instr_type_intrinsic)
            auto intr = nir_instr_as_intrinsic(instr);
   switch (intr->intrinsic) {
   case nir_intrinsic_load_barycentric_pixel:
   case nir_intrinsic_load_barycentric_sample:
   case nir_intrinsic_load_barycentric_at_sample:
   case nir_intrinsic_load_barycentric_at_offset:
   case nir_intrinsic_load_barycentric_centroid:
      m_interpolators_used.set(barycentric_ij_index(intr));
      case nir_intrinsic_load_front_face:
      m_sv_values.set(es_face);
      case nir_intrinsic_load_sample_mask_in:
      m_sv_values.set(es_sample_mask_in);
      case nir_intrinsic_load_sample_pos:
      m_sv_values.set(es_sample_pos);
      case nir_intrinsic_load_sample_id:
      m_sv_values.set(es_sample_id);
      case nir_intrinsic_load_helper_invocation:
      m_sv_values.set(es_helper_invocation);
      case nir_intrinsic_load_input:
         case nir_intrinsic_load_interpolated_input:
         default:
         }
      }
      bool
   FragmentShader::emit_load_sample_mask_in(nir_intrinsic_instr *instr)
   {
      auto& vf = value_factory();
   auto dest = vf.dest(instr->def, 0, pin_free);
   auto tmp = vf.temp_register();
   assert(m_sample_id_reg);
            emit_instruction(
         emit_instruction(
            }
      bool
   FragmentShader::emit_load_helper_invocation(nir_intrinsic_instr *instr)
   {
      assert(m_helper_invocation);
   auto& vf = value_factory();
   emit_instruction(
                  auto vtx = new LoadFromBuffer(destvec,
                                 {4, 7, 7, 7},
   vtx->set_fetch_flag(FetchInstr::vpm);
   vtx->set_fetch_flag(FetchInstr::use_tc);
   vtx->set_always_keep();
   auto dst = value_factory().dest(instr->def, 0, pin_free);
   auto ir = new AluInstr(op1_mov, dst, m_helper_invocation, AluInstr::last_write);
   ir->add_required_instr(vtx);
   emit_instruction(vtx);
               }
      bool
   FragmentShader::scan_input(nir_intrinsic_instr *intr, int index_src_id)
   {
      auto index = nir_src_as_const_value(intr->src[index_src_id]);
            const unsigned location_offset = chip_class() < ISA_CC_EVERGREEN ? 32 : 0;
            unsigned location = nir_intrinsic_io_semantics(intr).location + index->u32;
   unsigned driver_location = nir_intrinsic_base(intr) + index->u32;
   auto semantic = r600_get_varying_semantic(location);
   tgsi_semantic name = (tgsi_semantic)semantic.first;
            if (location == VARYING_SLOT_POS) {
      m_sv_values.set(es_pos);
   m_pos_driver_loc = driver_location + location_offset;
   ShaderInput pos_input(m_pos_driver_loc, name);
   pos_input.set_sid(sid);
   pos_input.set_interpolator(TGSI_INTERPOLATE_LINEAR,
               add_input(pos_input);
               if (location == VARYING_SLOT_FACE) {
      m_sv_values.set(es_face);
   m_face_driver_loc = driver_location + location_offset;
   ShaderInput face_input(m_face_driver_loc, name);
   face_input.set_sid(sid);
   add_input(face_input);
               tgsi_interpolate_mode tgsi_interpolate = TGSI_INTERPOLATE_CONSTANT;
            if (index_src_id > 0) {
      glsl_interp_mode mode = INTERP_MODE_NONE;
   auto parent = nir_instr_as_intrinsic(intr->src[0].ssa->parent_instr);
   mode = (glsl_interp_mode)nir_intrinsic_interp_mode(parent);
   switch (parent->intrinsic) {
   case nir_intrinsic_load_barycentric_sample:
      tgsi_loc = TGSI_INTERPOLATE_LOC_SAMPLE;
      case nir_intrinsic_load_barycentric_at_sample:
   case nir_intrinsic_load_barycentric_at_offset:
   case nir_intrinsic_load_barycentric_pixel:
      tgsi_loc = TGSI_INTERPOLATE_LOC_CENTER;
      case nir_intrinsic_load_barycentric_centroid:
      tgsi_loc = TGSI_INTERPOLATE_LOC_CENTROID;
   uses_interpol_at_centroid = true;
      default:
      std::cerr << "Instruction " << nir_intrinsic_infos[parent->intrinsic].name
                           switch (mode) {
   case INTERP_MODE_NONE:
      if (name == TGSI_SEMANTIC_COLOR || name == TGSI_SEMANTIC_BCOLOR) {
      tgsi_interpolate = TGSI_INTERPOLATE_COLOR;
      }
      case INTERP_MODE_SMOOTH:
      tgsi_interpolate = TGSI_INTERPOLATE_PERSPECTIVE;
      case INTERP_MODE_NOPERSPECTIVE:
      tgsi_interpolate = TGSI_INTERPOLATE_LINEAR;
      case INTERP_MODE_FLAT:
         case INTERP_MODE_COLOR:
      tgsi_interpolate = TGSI_INTERPOLATE_COLOR;
      case INTERP_MODE_EXPLICIT:
   default:
                     switch (name) {
   case TGSI_SEMANTIC_PRIMID:
      m_gs_prim_id_input = true;
   m_ps_prim_id_input = ninputs();
      case TGSI_SEMANTIC_COLOR:
   case TGSI_SEMANTIC_BCOLOR:
   case TGSI_SEMANTIC_FOG:
   case TGSI_SEMANTIC_GENERIC:
   case TGSI_SEMANTIC_TEXCOORD:
   case TGSI_SEMANTIC_LAYER:
   case TGSI_SEMANTIC_PCOORD:
   case TGSI_SEMANTIC_VIEWPORT_INDEX:
   case TGSI_SEMANTIC_CLIPDIST: {
      sfn_log << SfnLog::io << " have IO at " << driver_location << "\n";
   auto iinput = find_input(driver_location);
   if (iinput == input_not_found()) {
      ShaderInput input(driver_location, name);
   input.set_sid(sid);
   input.set_need_lds_pos();
   input.set_interpolator(tgsi_interpolate, tgsi_loc, uses_interpol_at_centroid);
   sfn_log << SfnLog::io << "add IO with LDS ID at " << input.location() << "\n";
   add_input(input);
      } else {
      if (uses_interpol_at_centroid) {
            }
      }
   default:
            }
      bool
   FragmentShader::emit_export_pixel(nir_intrinsic_instr& intr)
   {
      RegisterVec4::Swizzle swizzle;
   auto semantics = nir_intrinsic_io_semantics(&intr);
   unsigned driver_location = nir_intrinsic_base(&intr);
            switch (semantics.location) {
   case FRAG_RESULT_DEPTH:
      swizzle = {0, 7, 7, 7};
      case FRAG_RESULT_STENCIL:
      swizzle = {7, 0, 7, 7};
      case FRAG_RESULT_SAMPLE_MASK:
      swizzle = {7, 7, 0, 7};
      default:
      for (int i = 0; i < 4; ++i) {
                              if (semantics.location == FRAG_RESULT_COLOR ||
      (semantics.location >= FRAG_RESULT_DATA0 &&
            ShaderOutput output(driver_location, TGSI_SEMANTIC_COLOR, write_mask);
            unsigned color_outputs =
                                                                              if (location >= m_max_color_exports) {
      sfn_log << SfnLog::io << "Pixel output loc:" << location
         << " dl:" << driver_location << " skipped  because  we have only "
                                                /* Hack: force dual source output handling if one color output has a
   * dual_source_blend_index > 0 */
                  if (m_num_color_exports > 1)
                           /* If the i-th target format is set, all previous target formats must
   * be non-zero to avoid hangs. - from radeonsi, seems to apply to eg as well.
   /*/
                                 } else if (semantics.location == FRAG_RESULT_DEPTH ||
            semantics.location == FRAG_RESULT_STENCIL ||
   emit_instruction(new ExportInstr(ExportInstr::pixel, 61, value));
   int semantic = TGSI_SEMANTIC_POSITION;
   if (semantics.location == FRAG_RESULT_STENCIL)
         else if (semantics.location == FRAG_RESULT_SAMPLE_MASK)
            ShaderOutput output(driver_location, semantic, write_mask);
         } else {
         }
      }
      bool
   FragmentShader::emit_load_sample_pos(nir_intrinsic_instr *instr)
   {
               auto fetch = new LoadFromBuffer(dest,
                                 {0, 1, 2, 3},
   fetch->set_fetch_flag(FetchInstr::srf_mode);
   emit_instruction(fetch);
      }
      void
   FragmentShader::do_finalize()
   {
      /* On pre-evergreen not emtting something to all color exports that
   * are enabled might lead to a hang.
   * see: https://gitlab.freedesktop.org/mesa/mesa/-/issues/9223
   */
   if (chip_class() < ISA_CC_EVERGREEN) {
      unsigned i = 0;
            while (i < m_max_color_exports && (mask & (1u << (4 * i)))) {
      if (!(m_color_export_written_mask & (1u << i))) {
      RegisterVec4 value(0, false, {7, 7, 7, 7});
   m_last_pixel_export = new ExportInstr(ExportInstr::pixel, i, value);
   emit_instruction(m_last_pixel_export);
   m_num_color_exports++;
   if (m_export_highest < i)
      }
                  if (!m_last_pixel_export) {
      RegisterVec4 value(0, false, {7, 7, 7, 7});
   m_last_pixel_export = new ExportInstr(ExportInstr::pixel, 0, value);
   emit_instruction(m_last_pixel_export);
   m_num_color_exports++;
      }
      }
      bool
   FragmentShader::read_prop(std::istream& is)
   {
      string value;
            ASSERTED auto splitpos = value.find(':');
            std::istringstream ival(value);
   string name;
                     if (name == "MAX_COLOR_EXPORTS")
         else if (name == "COLOR_EXPORTS")
         else if (name == "COLOR_EXPORT_MASK")
         else if (name == "WRITE_ALL_COLORS")
         else
            }
      void
   FragmentShader::do_print_properties(std::ostream& os) const
   {
      os << "PROP MAX_COLOR_EXPORTS:" << m_max_color_exports << "\n";
   os << "PROP COLOR_EXPORTS:" << m_num_color_exports << "\n";
   os << "PROP COLOR_EXPORT_MASK:" << m_color_export_mask << "\n";
      }
      int
   FragmentShaderR600::allocate_interpolators_or_inputs()
   {
      int pos = 0;
   auto& vf = value_factory();
   for (auto& [index, inp] : inputs()) {
                  RegisterVec4 input(vf.allocate_pinned_register(pos, 0),
                                                         }
      }
      bool
   FragmentShaderR600::load_input_hw(nir_intrinsic_instr *intr)
   {
      auto& vf = value_factory();
   AluInstr *ir = nullptr;
   for (unsigned i = 0; i < intr->def.num_components; ++i) {
      sfn_log << SfnLog::io << "Inject register "
         unsigned index = nir_intrinsic_component(intr) + i;
   assert(index < 4);
   vf.inject_value(intr->def,
            }
   if (ir)
            }
      bool
   FragmentShaderR600::process_stage_intrinsic_hw(nir_intrinsic_instr *intr)
   {
      switch (intr->intrinsic) {
   case nir_intrinsic_load_barycentric_centroid:
   case nir_intrinsic_load_barycentric_pixel:
   case nir_intrinsic_load_barycentric_sample:
         default:
            }
      bool
   FragmentShaderR600::load_interpolated_input_hw(nir_intrinsic_instr *intr)
   {
         }
      bool
   FragmentShaderEG::load_input_hw(nir_intrinsic_instr *intr)
   {
      auto& vf = value_factory();
   auto io = input(nir_intrinsic_base(intr));
            bool need_temp = comp > 0;
   AluInstr *ir = nullptr;
   for (unsigned i = 0; i < intr->def.num_components; ++i) {
      if (need_temp) {
      auto tmp = vf.temp_register(comp + i);
   ir =
      new AluInstr(op1_interp_load_p0,
                  emit_instruction(ir);
   emit_instruction(new AluInstr(
                  ir = new AluInstr(op1_interp_load_p0,
                           }
   ir->set_alu_flag(alu_last_instr);
      }
      int
   FragmentShaderEG::allocate_interpolators_or_inputs()
   {
      for (unsigned i = 0; i < s_max_interpolators; ++i) {
      if (interpolators_used(i)) {
      sfn_log << SfnLog::io << "Interpolator " << i << " test enabled\n";
                  int num_baryc = 0;
   for (int i = 0; i < 6; ++i) {
      if (m_interpolator[i].enabled) {
      sfn_log << SfnLog::io << "Interpolator " << i
                                             }
      }
      bool
   FragmentShaderEG::process_stage_intrinsic_hw(nir_intrinsic_instr *intr)
   {
      auto& vf = value_factory();
   switch (intr->intrinsic) {
   case nir_intrinsic_load_barycentric_centroid:
   case nir_intrinsic_load_barycentric_pixel:
   case nir_intrinsic_load_barycentric_sample: {
      unsigned ij = barycentric_ij_index(intr);
   vf.inject_value(intr->def, 0, m_interpolator[ij].i);
   vf.inject_value(intr->def, 1, m_interpolator[ij].j);
      }
   case nir_intrinsic_load_barycentric_at_offset:
         case nir_intrinsic_load_barycentric_at_sample:
         default:
            }
      bool
   FragmentShaderEG::load_interpolated_input_hw(nir_intrinsic_instr *intr)
   {
      auto& vf = value_factory();
   ASSERTED auto param = nir_src_as_const_value(intr->src[1]);
            int dest_num_comp = intr->def.num_components;
   int start_comp = nir_intrinsic_component(intr);
                              params.i = vf.src(intr->src[0], 0);
   params.j = vf.src(intr->src[0], 1);
            if (!load_interpolated(dst, params, dest_num_comp, start_comp))
            if (need_temp) {
      AluInstr *ir = nullptr;
   for (unsigned i = 0; i < intr->def.num_components; ++i) {
      auto real_dst = vf.dest(intr->def, i, pin_chan);
   ir = new AluInstr(op1_mov, real_dst, dst[i + start_comp], AluInstr::write);
      }
   assert(ir);
                  }
      bool
   FragmentShaderEG::load_interpolated(RegisterVec4& dest,
                     {
      sfn_log << SfnLog::io << "Using Interpolator (" << *params.j << ", " << *params.i
                  if (num_dest_comp == 1) {
      switch (start_comp) {
   case 0:
         case 1:
         case 2:
         case 3:
         default:
                     if (num_dest_comp == 2) {
      switch (start_comp) {
   case 0:
         case 2:
         case 1:
      return load_interpolated_one_comp(dest, params, op2_interp_z) &&
      default:
                     if (num_dest_comp == 3 && start_comp == 0)
      return load_interpolated_two_comp(dest, params, op2_interp_xy, 0x3) &&
                  bool success =
         success &=
            }
      bool
   FragmentShaderEG::load_barycentric_at_sample(nir_intrinsic_instr *instr)
   {
      auto& vf = value_factory();
   RegisterVec4 slope = vf.temp_vec4(pin_group);
   auto src = emit_load_to_register(vf.src(instr->src[0], 0));
   auto fetch = new LoadFromBuffer(slope,
                                          fetch->set_fetch_flag(FetchInstr::srf_mode);
                     auto interpolator = m_interpolator[barycentric_ij_index(instr)];
                     auto tex = new TexInstr(TexInstr::get_gradient_h, grad, {0, 1, 7, 7}, interp, 0, 0);
   tex->set_tex_flag(TexInstr::grad_fine);
   tex->set_tex_flag(TexInstr::x_unnormalized);
   tex->set_tex_flag(TexInstr::y_unnormalized);
   tex->set_tex_flag(TexInstr::z_unnormalized);
   tex->set_tex_flag(TexInstr::w_unnormalized);
            tex = new TexInstr(TexInstr::get_gradient_v, grad, {7, 7, 0, 1}, interp, 0, 0);
   tex->set_tex_flag(TexInstr::x_unnormalized);
   tex->set_tex_flag(TexInstr::y_unnormalized);
   tex->set_tex_flag(TexInstr::z_unnormalized);
   tex->set_tex_flag(TexInstr::w_unnormalized);
   tex->set_tex_flag(TexInstr::grad_fine);
            auto tmp0 = vf.temp_register();
            emit_instruction(
         emit_instruction(new AluInstr(
            emit_instruction(new AluInstr(op3_muladd,
                                 emit_instruction(new AluInstr(op3_muladd,
                                       }
      bool
   FragmentShaderEG::load_barycentric_at_offset(nir_intrinsic_instr *instr)
   {
      auto& vf = value_factory();
            auto help = vf.temp_vec4(pin_group);
            auto getgradh =
         getgradh->set_tex_flag(TexInstr::x_unnormalized);
   getgradh->set_tex_flag(TexInstr::y_unnormalized);
   getgradh->set_tex_flag(TexInstr::z_unnormalized);
   getgradh->set_tex_flag(TexInstr::w_unnormalized);
   getgradh->set_tex_flag(TexInstr::grad_fine);
            auto getgradv =
         getgradv->set_tex_flag(TexInstr::x_unnormalized);
   getgradv->set_tex_flag(TexInstr::y_unnormalized);
   getgradv->set_tex_flag(TexInstr::z_unnormalized);
   getgradv->set_tex_flag(TexInstr::w_unnormalized);
   getgradv->set_tex_flag(TexInstr::grad_fine);
            auto ofs_x = vf.src(instr->src[0], 0);
   auto ofs_y = vf.src(instr->src[0], 1);
   auto tmp0 = vf.temp_register();
   auto tmp1 = vf.temp_register();
   emit_instruction(
         emit_instruction(new AluInstr(
         emit_instruction(new AluInstr(
         emit_instruction(new AluInstr(op3_muladd,
                                       }
      bool
   FragmentShaderEG::load_interpolated_one_comp(RegisterVec4& dest,
               {
      auto group = new AluGroup();
            AluInstr *ir = nullptr;
   for (unsigned i = 0; i < 2 && success; ++i) {
      int chan = i;
   if (op == op2_interp_z)
            ir = new AluInstr(op,
                              ir->set_bank_swizzle(alu_vec_210);
      }
   ir->set_alu_flag(alu_last_instr);
   if (success)
            }
      bool
   FragmentShaderEG::load_interpolated_two_comp(RegisterVec4& dest,
                     {
      auto group = new AluGroup();
            AluInstr *ir = nullptr;
   assert(params.j);
   assert(params.i);
   for (unsigned i = 0; i < 4; ++i) {
      ir = new AluInstr(op,
                     dest[i],
   ir->set_bank_swizzle(alu_vec_210);
      }
   ir->set_alu_flag(alu_last_instr);
   if (success)
            }
      bool
   FragmentShaderEG::load_interpolated_two_comp_for_one(RegisterVec4& dest,
                     {
      auto group = new AluGroup();
   bool success = true;
            for (int i = 0; i < 4; ++i) {
      ir = new AluInstr(op,
                     dest[i],
   ir->set_bank_swizzle(alu_vec_210);
      }
   ir->set_alu_flag(alu_last_instr);
   if (success)
               }
      FragmentShaderEG::Interpolator::Interpolator():
         {
   }
      } // namespace r600
