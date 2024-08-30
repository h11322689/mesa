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
      #include "sfn_shader_tess.h"
      #include "sfn_instr_export.h"
   #include "sfn_shader_vs.h"
      #include <sstream>
      namespace r600 {
      using std::string;
      TCSShader::TCSShader(const r600_shader_key& key):
      Shader("TCS", key.tcs.first_atomic_counter),
      {
   }
      bool
   TCSShader::do_scan_instruction(nir_instr *instr)
   {
      if (instr->type != nir_instr_type_intrinsic)
                     switch (ii->intrinsic) {
   case nir_intrinsic_load_primitive_id:
      m_sv_values.set(es_primitive_id);
      case nir_intrinsic_load_invocation_id:
      m_sv_values.set(es_invocation_id);
      case nir_intrinsic_load_tcs_rel_patch_id_r600:
      m_sv_values.set(es_rel_patch_id);
      case nir_intrinsic_load_tcs_tess_factor_base_r600:
      m_sv_values.set(es_tess_factor_base);
      default:
      return false;
      }
      }
      int
   TCSShader::do_allocate_reserved_registers()
   {
      if (m_sv_values.test(es_primitive_id)) {
                  if (m_sv_values.test(es_invocation_id)) {
                  if (m_sv_values.test(es_rel_patch_id)) {
                  if (m_sv_values.test(es_tess_factor_base)) {
                  return value_factory().next_register_index();
      }
      bool
   TCSShader::process_stage_intrinsic(nir_intrinsic_instr *instr)
   {
      switch (instr->intrinsic) {
   case nir_intrinsic_load_tcs_rel_patch_id_r600:
         case nir_intrinsic_load_invocation_id:
         case nir_intrinsic_load_primitive_id:
         case nir_intrinsic_load_tcs_tess_factor_base_r600:
         case nir_intrinsic_store_tf_r600:
         default:
            }
      bool
   TCSShader::store_tess_factor(nir_intrinsic_instr *instr)
   {
      auto value0 = value_factory().src_vec4(instr->src[0], pin_group, {0, 1, 7, 7});
   emit_instruction(new WriteTFInstr(value0));
      }
      void
   TCSShader::do_get_shader_info(r600_shader *sh_info)
   {
      sh_info->processor_type = PIPE_SHADER_TESS_CTRL;
      }
      bool
   TCSShader::read_prop(std::istream& is)
   {
      string value;
            ASSERTED auto splitpos = value.find(':');
            std::istringstream ival(value);
   string name;
                     if (name == "TCS_PRIM_MODE")
         else
            }
      void
   TCSShader::do_print_properties(std::ostream& os) const
   {
         }
      TESShader::TESShader(const pipe_stream_output_info *so_info,
                  VertexStageShader("TES", key.tes.first_atomic_counter),
   m_vs_as_gs_a(key.vs.as_gs_a),
      {
      if (key.tes.as_es)
         else
      }
      bool
   TESShader::do_scan_instruction(nir_instr *instr)
   {
      if (instr->type != nir_instr_type_intrinsic)
                     switch (intr->intrinsic) {
   case nir_intrinsic_load_tess_coord_xy:
      m_sv_values.set(es_tess_coord);
      case nir_intrinsic_load_primitive_id:
      m_sv_values.set(es_primitive_id);
      case nir_intrinsic_load_tcs_rel_patch_id_r600:
      m_sv_values.set(es_rel_patch_id);
      case nir_intrinsic_store_output: {
      int driver_location = nir_intrinsic_base(intr);
   int location = nir_intrinsic_io_semantics(intr).location;
   auto semantic = r600_get_varying_semantic(location);
   tgsi_semantic name = (tgsi_semantic)semantic.first;
   unsigned sid = semantic.second;
            if (location == VARYING_SLOT_LAYER)
            ShaderOutput output(driver_location, name, write_mask);
            switch (location) {
   case VARYING_SLOT_PSIZ:
   case VARYING_SLOT_POS:
   case VARYING_SLOT_CLIP_VERTEX:
   case VARYING_SLOT_EDGE: {
         }
   case VARYING_SLOT_CLIP_DIST0:
   case VARYING_SLOT_CLIP_DIST1:
   case VARYING_SLOT_VIEWPORT:
   case VARYING_SLOT_LAYER:
   case VARYING_SLOT_VIEW_INDEX:
   default:
         }
   add_output(output);
      }
   default:
         }
      }
      int
   TESShader::do_allocate_reserved_registers()
   {
      if (m_sv_values.test(es_tess_coord)) {
      m_tess_coord[0] = value_factory().allocate_pinned_register(0, 0);
               if (m_sv_values.test(es_rel_patch_id)) {
                  if (m_sv_values.test(es_primitive_id) || m_vs_as_gs_a) {
         }
      }
      bool
   TESShader::process_stage_intrinsic(nir_intrinsic_instr *intr)
   {
      switch (intr->intrinsic) {
   case nir_intrinsic_load_tess_coord_xy:
      return emit_simple_mov(intr->def, 0, m_tess_coord[0], pin_none) &&
      case nir_intrinsic_load_primitive_id:
         case nir_intrinsic_load_tcs_rel_patch_id_r600:
         case nir_intrinsic_store_output:
         default:
            }
      void
   TESShader::do_get_shader_info(r600_shader *sh_info)
   {
      sh_info->processor_type = PIPE_SHADER_TESS_EVAL;
      }
      void
   TESShader::do_finalize()
   {
         }
      bool
   TESShader::TESShader::read_prop(std::istream& is)
   {
      (void)is;
      }
      void
   TESShader::do_print_properties(std::ostream& os) const
   {
         }
      } // namespace r600
