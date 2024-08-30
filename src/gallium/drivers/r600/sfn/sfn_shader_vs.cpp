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
      #include "sfn_shader_vs.h"
      #include "sfn_debug.h"
   #include "sfn_instr_alugroup.h"
   #include "sfn_instr_export.h"
      namespace r600 {
      uint32_t
   VertexStageShader::enabled_stream_buffers_mask() const
   {
         }
      void
   VertexStageShader::combine_enabled_stream_buffers_mask(uint32_t mask)
   {
         }
      bool
   VertexExportStage::store_output(nir_intrinsic_instr& intr)
   {
      auto index = nir_src_as_const_value(intr.src[1]);
            const store_loc store_info = {nir_intrinsic_component(&intr),
                           }
      VertexExportStage::VertexExportStage(VertexStageShader *parent):
         {
   }
      VertexExportForFs::VertexExportForFs(VertexStageShader *parent,
                  VertexExportStage(parent),
   m_vs_as_gs_a(key.vs.as_gs_a),
   m_vs_prim_id_out(key.vs.prim_id_out),
      {
   }
      bool
   VertexExportForFs::do_store_output(const store_loc& store_info, nir_intrinsic_instr& intr)
   {
               case VARYING_SLOT_PSIZ:
      m_writes_point_size = true;
      case VARYING_SLOT_POS:
         case VARYING_SLOT_EDGE: {
      std::array<uint8_t, 4> swizzle_override = {7, 0, 7, 7};
      }
   case VARYING_SLOT_VIEWPORT: {
      std::array<uint8_t, 4> swizzle_override = {7, 7, 7, 0};
   return emit_varying_pos(store_info, intr, &swizzle_override) &&
      }
   case VARYING_SLOT_CLIP_VERTEX:
         case VARYING_SLOT_CLIP_DIST0:
   case VARYING_SLOT_CLIP_DIST1: {
      bool success = emit_varying_pos(store_info, intr);
   m_num_clip_dist += 4;
   if (!nir_intrinsic_io_semantics(&intr).no_varying)
            }
   case VARYING_SLOT_LAYER: {
      m_out_misc_write = 1;
   m_vs_out_layer = 1;
   std::array<uint8_t, 4> swz = {7, 7, 0, 7};
   return emit_varying_pos(store_info, intr, &swz) &&
      }
   case VARYING_SLOT_VIEW_INDEX:
            default:
      return emit_varying_param(store_info, intr);
         }
      bool
   VertexExportForFs::emit_clip_vertices(const store_loc& store_info,
         {
               m_cc_dist_mask = 0xff;
                                 }
      void
   VertexExportForFs::get_shader_info(r600_shader *sh_info) const
   {
      sh_info->cc_dist_mask = m_cc_dist_mask;
   sh_info->clip_dist_write = m_clip_dist_write;
   sh_info->vs_as_gs_a = m_vs_as_gs_a;
   sh_info->vs_out_edgeflag = m_out_edgeflag;
   sh_info->vs_out_viewport = m_out_viewport;
   sh_info->vs_out_misc_write = m_out_misc_write;
   sh_info->vs_out_point_size = m_out_point_size;
      }
      void
   VertexExportForFs::finalize()
   {
      if (m_vs_as_gs_a) {
      auto primid = m_parent->value_factory().temp_vec4(pin_group, {2, 7, 7, 7});
   m_parent->emit_instruction(new AluInstr(
                  m_last_param_export = new ExportInstr(ExportInstr::param, param, primid);
            ShaderOutput output(m_parent->noutputs(), TGSI_SEMANTIC_PRIMID, 1);
   output.set_sid(0);
   output.override_spi_sid(m_vs_prim_id_out);
               if (!m_last_pos_export) {
      RegisterVec4 value(0, false, {7, 7, 7, 7});
   m_last_pos_export = new ExportInstr(ExportInstr::pos, 0, value);
               if (!m_last_param_export) {
      RegisterVec4 value(0, false, {7, 7, 7, 7});
   m_last_param_export = new ExportInstr(ExportInstr::param, 0, value);
               m_last_pos_export->set_is_last_export(true);
            if (m_so_info && m_so_info->num_outputs)
      }
      void
   VertexShader::do_get_shader_info(r600_shader *sh_info)
   {
      sh_info->processor_type = PIPE_SHADER_VERTEX;
      }
      bool
   VertexExportForFs::emit_varying_pos(const store_loc& store_info,
               {
      RegisterVec4::Swizzle swizzle;
                     if (!swizzle_override) {
      for (int i = 0; i < 4; ++i)
      } else
                     auto in_value = m_parent->value_factory().src_vec4(intr.src[0], pin_group, swizzle);
   auto& value = in_value;
            switch (store_info.location) {
   case VARYING_SLOT_EDGE: {
      m_out_misc_write = true;
   m_out_edgeflag = true;
   auto src = m_parent->value_factory().src(intr.src[0], 0);
   auto clamped = m_parent->value_factory().temp_register();
   m_parent->emit_instruction(
         auto alu =
         if (m_parent->chip_class() < ISA_CC_EVERGREEN)
                     }
         case VARYING_SLOT_PSIZ:
      m_out_misc_write = true;
   m_out_point_size = true;
      case VARYING_SLOT_LAYER:
      export_slot = 1;
      case VARYING_SLOT_VIEWPORT:
      m_out_misc_write = true;
   m_out_viewport = true;
   export_slot = 1;
      case VARYING_SLOT_POS:
         case VARYING_SLOT_CLIP_DIST0:
   case VARYING_SLOT_CLIP_DIST1:
      m_cc_dist_mask |= write_mask
         m_clip_dist_write |= write_mask
         export_slot = m_cur_clip_pos++;
      default:
      sfn_log << SfnLog::err << __func__ << "Unsupported location " << store_info.location
                                                   }
      bool
   VertexExportForFs::emit_varying_param(const store_loc& store_info,
         {
      sfn_log << SfnLog::io << __func__ << ": emit DDL: " << store_info.driver_location
            int write_mask = nir_intrinsic_write_mask(&intr) << store_info.frac;
   RegisterVec4::Swizzle swizzle;
   for (int i = 0; i < 4; ++i)
                     int export_slot = m_parent->output(nir_intrinsic_base(&intr)).pos();
            AluInstr *alu = nullptr;
   for (int i = 0; i < 4; ++i) {
      if (swizzle[i] < 4) {
      alu = new AluInstr(op1_mov,
                           }
   if (alu)
            m_last_param_export = new ExportInstr(ExportInstr::param, export_slot, value);
                        }
      bool
   VertexExportForFs::emit_stream(int stream)
   {
      assert(m_so_info);
   if (m_so_info->num_outputs > PIPE_MAX_SO_OUTPUTS) {
      R600_ERR("Too many stream outputs: %d\n", m_so_info->num_outputs);
      }
   for (unsigned i = 0; i < m_so_info->num_outputs; i++) {
      if (m_so_info->output[i].output_buffer >= 4) {
      R600_ERR("Exceeded the max number of stream output buffers, got: %d\n",
               }
   const RegisterVec4 *so_gpr[PIPE_MAX_SHADER_OUTPUTS];
   unsigned start_comp[PIPE_MAX_SHADER_OUTPUTS];
            /* Initialize locations where the outputs are stored. */
   for (unsigned i = 0; i < m_so_info->num_outputs; i++) {
      if (stream != -1 && stream != m_so_info->output[i].stream)
            sfn_log << SfnLog::instr << "Emit stream " << i << " with register index "
                     if (!so_gpr[i]) {
      sfn_log << SfnLog::err << "\nERR: register index "
         << m_so_info->output[i].register_index
      }
   start_comp[i] = m_so_info->output[i].start_component;
   /* Lower outputs with dst_offset < start_component.
   *
   * We can only output 4D vectors with a write mask, e.g. we can
   * only output the W component at offset 3, etc. If we want
   * to store Y, Z, or W at buffer offset 0, we need to use MOV
            bool need_copy =
            int sc = m_so_info->output[i].start_component;
   for (int j = 0; j < m_so_info->output[i].num_components; j++) {
      if ((*so_gpr[i])[j + sc]->chan() != j + sc) {
      need_copy = true;
         }
   if (need_copy) {
      RegisterVec4::Swizzle swizzle = {0, 1, 2, 3};
   for (auto j = m_so_info->output[i].num_components; j < 4; ++j)
                  AluInstr *alu = nullptr;
   for (int j = 0; j < m_so_info->output[i].num_components; j++) {
      alu = new AluInstr(op1_mov, tmp[i][j], (*so_gpr[i])[j + sc], {alu_write});
      }
                  start_comp[i] = 0;
      }
               uint32_t enabled_stream_buffers_mask = 0;
   /* Write outputs to buffers. */
   for (unsigned i = 0; i < m_so_info->num_outputs; i++) {
      sfn_log << SfnLog::instr << "Write output buffer " << i << " with register index "
            auto out_stream =
      new StreamOutInstr(*so_gpr[i],
                     m_so_info->output[i].num_components,
   m_so_info->output[i].dst_offset - start_comp[i],
      m_parent->emit_instruction(out_stream);
   enabled_stream_buffers_mask |= (1 << m_so_info->output[i].output_buffer)
      }
   m_parent->combine_enabled_stream_buffers_mask(enabled_stream_buffers_mask);
      }
      const RegisterVec4 *
   VertexExportForFs::output_register(int loc) const
   {
      const RegisterVec4 *retval = nullptr;
   auto val = m_output_registers.find(loc);
   if (val != m_output_registers.end())
            }
      VertexShader::VertexShader(const pipe_stream_output_info *so_info,
                  VertexStageShader("VS", key.vs.first_atomic_counter),
      {
      if (key.vs.as_es)
         else if (key.vs.as_ls)
         else
      }
      bool
   VertexShader::do_scan_instruction(nir_instr *instr)
   {
      if (instr->type != nir_instr_type_intrinsic)
                     switch (intr->intrinsic) {
   case nir_intrinsic_load_input: {
      int vtx_register = nir_intrinsic_base(intr) + 1;
   if (m_last_vertex_attribute_register < vtx_register)
            }
   case nir_intrinsic_store_output: {
      int driver_location = nir_intrinsic_base(intr);
   int location = nir_intrinsic_io_semantics(intr).location;
   auto semantic = r600_get_varying_semantic(location);
   tgsi_semantic name = (tgsi_semantic)semantic.first;
   unsigned sid = semantic.second;
            if (location == VARYING_SLOT_LAYER)
            ShaderOutput output(driver_location, name, write_mask);
            switch (location) {
   case VARYING_SLOT_CLIP_DIST0:
   case VARYING_SLOT_CLIP_DIST1:
      if (nir_intrinsic_io_semantics(intr).no_varying)
            case VARYING_SLOT_VIEWPORT:
   case VARYING_SLOT_LAYER:
   case VARYING_SLOT_VIEW_INDEX:
   default:
      output.set_is_param(true);
      case VARYING_SLOT_PSIZ:
   case VARYING_SLOT_POS:
   case VARYING_SLOT_CLIP_VERTEX:
   case VARYING_SLOT_EDGE:
      add_output(output);
      }
      }
   case nir_intrinsic_load_vertex_id:
      m_sv_values.set(es_vertexid);
      case nir_intrinsic_load_instance_id:
      m_sv_values.set(es_instanceid);
      case nir_intrinsic_load_primitive_id:
      m_sv_values.set(es_primitive_id);
      case nir_intrinsic_load_tcs_rel_patch_id_r600:
      m_sv_values.set(es_rel_patch_id);
      default:
                     }
      bool
   VertexShader::load_input(nir_intrinsic_instr *intr)
   {
      unsigned driver_location = nir_intrinsic_base(intr);
   unsigned location = nir_intrinsic_io_semantics(intr).location;
            AluInstr *ir = nullptr;
   if (location < VERT_ATTRIB_MAX) {
      for (unsigned i = 0; i < intr->def.num_components; ++i) {
      auto src = vf.allocate_pinned_register(driver_location + 1, i);
   src->set_flag(Register::ssa);
      }
   if (ir)
            ShaderInput input(driver_location, location);
   input.set_gpr(driver_location + 1);
   add_input(input);
      }
   fprintf(stderr, "r600-NIR: Unimplemented load_deref for %d\n", location);
      }
      int
   VertexShader::do_allocate_reserved_registers()
   {
      if (m_sv_values.test(es_vertexid)) {
                  if (m_sv_values.test(es_instanceid)) {
                  if (m_sv_values.test(es_primitive_id) || m_vs_as_gs_a) {
      auto primitive_id = value_factory().allocate_pinned_register(0, 2);
               if (m_sv_values.test(es_rel_patch_id)) {
                     }
      bool
   VertexShader::store_output(nir_intrinsic_instr *intr)
   {
         }
      bool
   VertexShader::process_stage_intrinsic(nir_intrinsic_instr *intr)
   {
      switch (intr->intrinsic) {
   case nir_intrinsic_load_vertex_id:
         case nir_intrinsic_load_instance_id:
         case nir_intrinsic_load_primitive_id:
         case nir_intrinsic_load_tcs_rel_patch_id_r600:
         default:
            }
      void
   VertexShader::do_finalize()
   {
         }
      bool
   VertexShader::read_prop(std::istream& is)
   {
      (void)is;
      }
      void
   VertexShader::do_print_properties(std::ostream& os) const
   {
         }
      VertexExportForGS::VertexExportForGS(VertexStageShader *parent,
            VertexExportStage(parent),
      {
   }
      bool
   VertexExportForGS::do_store_output(const store_loc& store_info,
         {
      int ring_offset = -1;
            sfn_log << SfnLog::io << "check output " << store_info.driver_location
            for (unsigned k = 0; k < m_gs_shader->ninput; ++k) {
      auto& in_io = m_gs_shader->input[k];
   sfn_log << SfnLog::io << "  against  " << k << " name=" << in_io.name
            if (in_io.name == out_io.name() && in_io.sid == out_io.sid()) {
      ring_offset = in_io.ring_offset;
                  if (store_info.location == VARYING_SLOT_VIEWPORT) {
      m_vs_out_viewport = 1;
   m_vs_out_misc_write = 1;
               if (ring_offset == -1) {
      sfn_log << SfnLog::warn << "VS defines output at "
         << store_info.driver_location
   << "name=" << out_io.name() << " sid=" << out_io.sid()
               RegisterVec4::Swizzle src_swz = {7, 7, 7, 7};
   for (int i = 0; i < 4; ++i)
                     AluInstr *ir = nullptr;
   for (unsigned int i = 0; i < instr.num_components; ++i) {
      ir = new AluInstr(op1_mov,
                        }
   if (ir)
            m_parent->emit_instruction(new MemRingOutInstr(
            if (store_info.location == VARYING_SLOT_CLIP_DIST0 ||
      store_info.location == VARYING_SLOT_CLIP_DIST1)
            }
      void
   VertexExportForGS::finalize()
   {
   }
      void
   VertexExportForGS::get_shader_info(r600_shader *sh_info) const
   {
      sh_info->vs_out_viewport = m_vs_out_viewport;
   sh_info->vs_out_misc_write = m_vs_out_misc_write;
      }
      VertexExportForTCS::VertexExportForTCS(VertexStageShader *parent):
         {
   }
      void
   VertexExportForTCS::finalize()
   {
   }
      void
   VertexExportForTCS::get_shader_info(r600_shader *sh_info) const
   {
         }
      bool
   VertexExportForTCS::do_store_output(const store_loc& store_info,
         {
      (void)store_info;
   (void)intr;
      }
      } // namespace r600
