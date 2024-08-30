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
      #include "sfn_shader_gs.h"
      #include "sfn_debug.h"
   #include "sfn_instr_fetch.h"
      namespace r600 {
      GeometryShader::GeometryShader(const r600_shader_key& key):
      Shader("GS", key.gs.first_atomic_counter),
      {
   }
      bool
   GeometryShader::do_scan_instruction(nir_instr *instr)
   {
      if (instr->type != nir_instr_type_intrinsic)
                     switch (ii->intrinsic) {
   case nir_intrinsic_store_output:
         case nir_intrinsic_load_per_vertex_input:
         default:
            }
      bool
   GeometryShader::process_store_output(nir_intrinsic_instr *instr)
   {
      auto location = nir_intrinsic_io_semantics(instr).location;
   auto index = nir_src_as_const_value(instr->src[1]);
                     if (location == VARYING_SLOT_COL0 || location == VARYING_SLOT_COL1 ||
      (location >= VARYING_SLOT_VAR0 && location <= VARYING_SLOT_VAR31) ||
   (location >= VARYING_SLOT_TEX0 && location <= VARYING_SLOT_TEX7) ||
   location == VARYING_SLOT_BFC0 || location == VARYING_SLOT_BFC1 ||
   location == VARYING_SLOT_PNTC || location == VARYING_SLOT_CLIP_VERTEX ||
   location == VARYING_SLOT_CLIP_DIST0 || location == VARYING_SLOT_CLIP_DIST1 ||
   location == VARYING_SLOT_PRIMITIVE_ID || location == VARYING_SLOT_POS ||
   location == VARYING_SLOT_PSIZ || location == VARYING_SLOT_LAYER ||
            auto semantic = r600_get_varying_semantic(location);
   tgsi_semantic name = (tgsi_semantic)semantic.first;
   auto write_mask = nir_intrinsic_write_mask(instr);
            if (!nir_intrinsic_io_semantics(instr).no_varying)
         if (nir_intrinsic_io_semantics(instr).location != VARYING_SLOT_CLIP_VERTEX)
            if (location == VARYING_SLOT_VIEWPORT) {
      m_out_viewport = true;
               if (location == VARYING_SLOT_CLIP_DIST0 || location == VARYING_SLOT_CLIP_DIST1) {
      auto write_mask = nir_intrinsic_write_mask(instr);
   m_cc_dist_mask |= write_mask << (4 * (location - VARYING_SLOT_CLIP_DIST0));
               if (m_noutputs <= driver_location &&
                     }
      }
      bool
   GeometryShader::process_load_input(nir_intrinsic_instr *instr)
   {
      auto location = nir_intrinsic_io_semantics(instr).location;
   auto index = nir_src_as_const_value(instr->src[1]);
                     if (location == VARYING_SLOT_POS || location == VARYING_SLOT_PSIZ ||
      location == VARYING_SLOT_FOGC || location == VARYING_SLOT_CLIP_VERTEX ||
   location == VARYING_SLOT_CLIP_DIST0 || location == VARYING_SLOT_CLIP_DIST1 ||
   location == VARYING_SLOT_COL0 || location == VARYING_SLOT_COL1 ||
   location == VARYING_SLOT_BFC0 || location == VARYING_SLOT_BFC1 ||
   location == VARYING_SLOT_PNTC ||
   (location >= VARYING_SLOT_VAR0 && location <= VARYING_SLOT_VAR31) ||
            uint64_t bit = 1ull << location;
   if (!(bit & m_input_mask)) {
      auto semantic = r600_get_varying_semantic(location);
   ShaderInput input(driver_location, semantic.first);
   input.set_sid(semantic.second);
   input.set_ring_offset(16 * driver_location);
   add_input(input);
   m_next_input_ring_offset += 16;
      }
      }
      }
      int
   GeometryShader::do_allocate_reserved_registers()
   {
      const int sel[6] = {0, 0, 0, 1, 1, 1};
            /* Reserve registers used by the shaders (should check how many
   * components are actually used */
   for (int i = 0; i < 6; ++i) {
                  m_primitive_id = value_factory().allocate_pinned_register(0, 2);
                              for (int i = 0; i < 4; ++i) {
      m_export_base[i] = value_factory().temp_register(0, false);
   emit_instruction(
                        /* GS thread with no output workaround - emit a cut at start of GS */
   if (chip_class() == ISA_CC_R600) {
      emit_instruction(new EmitVertexInstr(0, true));
               if (m_tri_strip_adj_fix)
               }
      bool
   GeometryShader::process_stage_intrinsic(nir_intrinsic_instr *intr)
   {
      switch (intr->intrinsic) {
   case nir_intrinsic_emit_vertex:
         case nir_intrinsic_end_primitive:
         case nir_intrinsic_load_primitive_id:
         case nir_intrinsic_load_invocation_id:
         case nir_intrinsic_load_per_vertex_input:
         default:;
   }
      }
      bool
   GeometryShader::emit_vertex(nir_intrinsic_instr *instr, bool cut)
   {
      int stream = nir_intrinsic_stream_id(instr);
                     for (auto v : m_streamout_data) {
      if (stream == 0 || v.first != VARYING_SLOT_POS) {
      v.second->patch_ring(stream, m_export_base[stream]);
   cut_instr->add_required_instr(v.second);
      } else
      }
            emit_instruction(cut_instr);
            if (!cut) {
      auto ir = new AluInstr(op2_add_int,
                                          }
      bool
   GeometryShader::store_output(nir_intrinsic_instr *instr)
   {
      if (nir_intrinsic_io_semantics(instr).location == VARYING_SLOT_CLIP_VERTEX)
            auto location = nir_intrinsic_io_semantics(instr).location;
   auto index = nir_src_as_const_value(instr->src[1]);
   assert(index);
            uint32_t write_mask = nir_intrinsic_write_mask(instr);
            RegisterVec4::Swizzle src_swz{7, 7, 7, 7};
   for (unsigned i = shift; i < 4; ++i) {
                           AluInstr *ir = nullptr;
   if (m_streamout_data[location]) {
      const auto& value = m_streamout_data[location]->value();
   auto tmp = value_factory().temp_vec4(pin_chgr);
   for (unsigned i = 0; i < 4 - shift; ++i) {
      if (!(write_mask & (1 << i)))
         if (out_value[i + shift]->chan() < 4) {
      ir = new AluInstr(op1_mov,
                  } else if (value[i]->chan() < 4) {
         } else
            }
   ir->set_alu_flag(alu_last_instr);
   m_streamout_data[location] = new MemRingOutInstr(cf_mem_ring,
                                          sfn_log << SfnLog::io << "None-streamout ";
   bool need_copy = shift != 0;
   if (!need_copy) {
      for (int i = 0; i < 4; ++i) {
      if ((write_mask & (1 << i)) && (out_value[i]->chan() != i)) {
      need_copy = true;
                     if (need_copy) {
      auto tmp = value_factory().temp_vec4(pin_chgr);
   for (unsigned i = 0; i < 4 - shift; ++i) {
      if (out_value[i]->chan() < 4) {
      ir = new AluInstr(op1_mov, tmp[i], out_value[i], AluInstr::write);
         }
   ir->set_alu_flag(alu_last_instr);
   m_streamout_data[location] = new MemRingOutInstr(cf_mem_ring,
                              } else {
      for (auto i = 0; i < 4; ++i)
         m_streamout_data[location] = new MemRingOutInstr(cf_mem_ring,
                                             }
      bool
   GeometryShader::emit_load_per_vertex_input(nir_intrinsic_instr *instr)
   {
               RegisterVec4::Swizzle dest_swz{7, 7, 7, 7};
   for (unsigned i = 0; i < instr->def.num_components; ++i) {
                           if (!literal_index) {
      sfn_log << SfnLog::err << "GS: Indirect input addressing not (yet) supported\n";
      }
   assert(literal_index->u32 < 6);
            EVTXDataFormat fmt =
            auto addr = m_per_vertex_offsets[literal_index->u32];
   auto fetch = new LoadFromBuffer(dest,
                                          if (chip_class() >= ISA_CC_EVERGREEN)
            fetch->set_num_format(vtx_nf_norm);
            emit_instruction(fetch);
      }
      void
   GeometryShader::do_finalize()
   {
   }
      void
   GeometryShader::do_get_shader_info(r600_shader *sh_info)
   {
      sh_info->processor_type = PIPE_SHADER_GEOMETRY;
   sh_info->ring_item_sizes[0] = m_ring_item_sizes[0];
   sh_info->cc_dist_mask = m_cc_dist_mask;
      }
      bool
   GeometryShader::read_prop(std::istream& is)
   {
      (void)is;
      }
      void
   GeometryShader::do_print_properties(std::ostream& os) const
   {
         }
      void
   GeometryShader::emit_adj_fix()
   {
               emit_instruction(new AluInstr(op2_and_int,
                              int reg_indices[6];
            reg_indices[0] = reg_indices[1] = reg_indices[2] = m_export_base[1]->sel();
                     AluInstr *ir = nullptr;
   for (int i = 0; i < 6; i++) {
      adjhelp[i] = value_factory().temp_register();
   ir = new AluInstr(op3_cnde_int,
                     adjhelp[i],
               }
            for (int i = 0; i < 6; i++)
      }
      } // namespace r600
