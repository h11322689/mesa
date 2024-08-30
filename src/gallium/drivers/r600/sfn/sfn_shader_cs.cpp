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
      #include "sfn_shader_cs.h"
      #include "sfn_instr_fetch.h"
      namespace r600 {
      ComputeShader::ComputeShader(UNUSED const r600_shader_key& key, int num_samplers):
      Shader("CS", 0),
      {
   }
      bool
   ComputeShader::do_scan_instruction(UNUSED nir_instr *instr)
   {
         }
      int
   ComputeShader::do_allocate_reserved_registers()
   {
               const int thread_id_sel = 0;
            for (int i = 0; i < 3; ++i) {
      m_local_invocation_id[i] = vf.allocate_pinned_register(thread_id_sel, i);
   m_local_invocation_id[i]->set_flag(Register::pin_end);
   m_workgroup_id[i] = vf.allocate_pinned_register(wg_id_sel, i);
      }
      }
      bool
   ComputeShader::process_stage_intrinsic(nir_intrinsic_instr *instr)
   {
      switch (instr->intrinsic) {
   case nir_intrinsic_load_local_invocation_id:
         case nir_intrinsic_load_workgroup_id:
         case nir_intrinsic_load_workgroup_size:
         case nir_intrinsic_load_num_workgroups:
         default:
            }
      void
   ComputeShader::do_get_shader_info(r600_shader *sh_info)
   {
         }
      bool
   ComputeShader::read_prop(UNUSED std::istream& is)
   {
         }
      void
   ComputeShader::do_print_properties(UNUSED std::ostream& os) const
   {
   }
      bool
   ComputeShader::emit_load_from_info_buffer(nir_intrinsic_instr *instr, int offset)
   {
      if (!m_zero_register) {
      m_zero_register = value_factory().temp_register();
   emit_instruction(new AluInstr(op1_mov,
                                    auto ir = new LoadFromBuffer(dest,
                              {0, 1, 2, 7},
         ir->set_fetch_flag(LoadFromBuffer::srf_mode);
   ir->reset_fetch_flag(LoadFromBuffer::format_comp_signed);
   ir->set_num_format(vtx_nf_int);
   emit_instruction(ir);
      }
      bool
   ComputeShader::emit_load_3vec(nir_intrinsic_instr *instr,
         {
               for (int i = 0; i < 3; ++i) {
      auto dest = vf.dest(instr->def, i, pin_none);
   emit_instruction(new AluInstr(
      }
      }
      } // namespace r600
