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
      #include "sfn_shader.h"
      #include "gallium/drivers/r600/r600_shader.h"
   #include "nir.h"
   #include "nir_intrinsics.h"
   #include "nir_intrinsics_indices.h"
   #include "sfn_debug.h"
   #include "sfn_instr.h"
   #include "sfn_instr_alu.h"
   #include "sfn_instr_alugroup.h"
   #include "sfn_instr_controlflow.h"
   #include "sfn_instr_export.h"
   #include "sfn_instr_fetch.h"
   #include "sfn_instr_lds.h"
   #include "sfn_instr_mem.h"
   #include "sfn_liverangeevaluator.h"
   #include "sfn_shader_cs.h"
   #include "sfn_shader_fs.h"
   #include "sfn_shader_gs.h"
   #include "sfn_shader_tess.h"
   #include "sfn_shader_vs.h"
   #include "tgsi/tgsi_from_mesa.h"
      #include <numeric>
   #include <sstream>
      namespace r600 {
      using std::string;
      std::pair<unsigned, unsigned>
   r600_get_varying_semantic(unsigned varying_location)
   {
      std::pair<unsigned, unsigned> result;
   tgsi_get_gl_varying_semantic(static_cast<gl_varying_slot>(varying_location),
                        if (result.first == TGSI_SEMANTIC_GENERIC) {
         } else if (result.first == TGSI_SEMANTIC_PCOORD) {
         }
      }
      void
   ShaderIO::set_sid(int sid)
   {
      m_sid = sid;
   switch (m_name) {
   case TGSI_SEMANTIC_POSITION:
   case TGSI_SEMANTIC_PSIZE:
   case TGSI_SEMANTIC_EDGEFLAG:
   case TGSI_SEMANTIC_FACE:
   case TGSI_SEMANTIC_SAMPLEMASK:
   case TGSI_SEMANTIC_CLIPVERTEX:
      m_spi_sid = 0;
      case TGSI_SEMANTIC_GENERIC:
   case TGSI_SEMANTIC_TEXCOORD:
   case TGSI_SEMANTIC_PCOORD:
      m_spi_sid = m_sid + 1;
      default:
      /* For non-generic params - pack name and sid into 8 bits */
         }
      void
   ShaderIO::override_spi_sid(int spi)
   {
         }
      void
   ShaderIO::print(std::ostream& os) const
   {
      os << m_type << " LOC:" << m_location << " NAME:" << m_name;
            if (m_sid > 0) {
            }
      ShaderIO::ShaderIO(const char *type, int loc, int name):
      m_type(type),
   m_location(loc),
      {
   }
      ShaderOutput::ShaderOutput():
         {
   }
      ShaderOutput::ShaderOutput(int location, int name, int writemask):
      ShaderIO("OUTPUT", location, name),
      {
   }
      void
   ShaderOutput::do_print(std::ostream& os) const
   {
         }
      ShaderInput::ShaderInput(int location, int name):
         {
   }
      ShaderInput::ShaderInput():
         {
   }
      void
   ShaderInput::do_print(std::ostream& os) const
   {
      if (m_interpolator)
         if (m_interpolate_loc)
         if (m_uses_interpolate_at_centroid)
      }
      void
   ShaderInput::set_interpolator(int interp,
               {
      m_interpolator = interp;
   m_interpolate_loc = interp_loc;
      }
      void
   ShaderInput::set_uses_interpolate_at_centroid()
   {
         }
      Shader::Shader(const char *type_id, unsigned atomic_base):
      m_current_block(nullptr),
   m_type_id(type_id),
   m_chip_class(ISA_CC_R600),
   m_next_block(0),
      {
      m_instr_factory = new InstrFactory();
   m_chain_instr.this_shader = this;
      }
      void
   Shader::set_input_gpr(int driver_lcation, int gpr)
   {
      auto i = m_inputs.find(driver_lcation);
   assert(i != m_inputs.end());
      }
      bool
   Shader::add_info_from_string(std::istream& is)
   {
      std::string type;
            if (type == "CHIPCLASS")
         if (type == "FAMILY")
         if (type == "OUTPUT")
         if (type == "INPUT")
         if (type == "PROP")
         if (type == "SYSVALUES")
         if (type == "REGISTERS")
         if (type == "ARRAYS")
               }
      void
   Shader::emit_instruction_from_string(const std::string& s)
   {
         sfn_log << SfnLog::instr << "Create Instr from '" << s << "'\n";
   if (s == "BLOCK_START") {
      if (!m_current_block->empty()) {
      start_new_block(m_current_block->nesting_offset());
      }
               if (s == "BLOCK_END") {
                  auto ir = m_instr_factory->from_string(s, m_current_block->nesting_depth(),
         if (ir) {
      emit_instruction(ir);
   if (ir->end_block())
               }
      bool
   Shader::read_output(std::istream& is)
   {
      string value;
   is >> value;
   int pos = int_from_string_with_prefix(value, "LOC:");
   is >> value;
   int name = int_from_string_with_prefix(value, "NAME:");
   is >> value;
   int mask = int_from_string_with_prefix(value, "MASK:");
            value.clear();
   is >> value;
   if (!value.empty()) {
      int sid = int_from_string_with_prefix(value, "SID:");
   output.set_sid(sid);
   is >> value;
   ASSERTED int spi_sid = int_from_string_with_prefix(value, "SPI_SID:");
               add_output(output);
      }
      bool
   Shader::read_input(std::istream& is)
   {
      string value;
   is >> value;
   int pos = int_from_string_with_prefix(value, "LOC:");
   is >> value;
                              int interp = 0;
   int interp_loc = 0;
            is >> value;
   while (!value.empty()) {
      if (value.substr(0, 4) == "SID:") {
      int sid = int_from_string_with_prefix(value, "SID:");
      } else if (value.substr(0, 8) == "SPI_SID:") {
      ASSERTED int spi_sid = int_from_string_with_prefix(value, "SPI_SID:");
      } else if (value.substr(0, 7) == "INTERP:") {
         } else if (value.substr(0, 5) == "ILOC:") {
         } else if (value == "USE_CENTROID") {
         } else {
      std::cerr << "Unknown parse value '" << value << "'";
      }
   value.clear();
                        add_input(input);
      }
      bool
   Shader::allocate_registers_from_string(std::istream& is, Pin pin)
   {
      std::string line;
   if (!std::getline(is, line))
                     while (!iline.eof()) {
      string reg_str;
            if (reg_str.empty())
            if (strchr(reg_str.c_str(), '@') ||
      reg_str == "AR" ||
   reg_str.substr(0,3) == "IDX") {
      } else {
      RegisterVec4::Swizzle swz = {0, 1, 2, 3};
   auto regs = value_factory().dest_vec4_from_string(reg_str, swz, pin);
   for (int i = 0; i < 4; ++i) {
      if (swz[i] < 4 && pin == pin_fully) {
                  }
      }
      bool
   Shader::allocate_arrays_from_string(std::istream& is)
   {
      std::string line;
   if (!std::getline(is, line))
                     while (!iline.eof()) {
      string reg_str;
            if (reg_str.empty())
               }
      }
      bool
   Shader::read_chipclass(std::istream& is)
   {
      string name;
   is >> name;
   if (name == "R600")
         else if (name == "R700")
         else if (name == "EVERGREEN")
         else if (name == "CAYMAN")
         else
            }
      bool
   Shader::read_family(std::istream& is)
   {
      string name;
      #define CHECK_FAMILY(F) if (name == #F) m_chip_family = CHIP_ ## F
         CHECK_FAMILY(R600);
   else CHECK_FAMILY(R600);
   else CHECK_FAMILY(RV610);
   else CHECK_FAMILY(RV630);
   else CHECK_FAMILY(RV670);
   else CHECK_FAMILY(RV620);
   else CHECK_FAMILY(RV635);
   else CHECK_FAMILY(RS780);
   else CHECK_FAMILY(RS880);
   /* GFX3 (R7xx) */
   else CHECK_FAMILY(RV770);
   else CHECK_FAMILY(RV730);
   else CHECK_FAMILY(RV710);
   else CHECK_FAMILY(RV740);
   /* GFX4 (Evergreen) */
   else CHECK_FAMILY(CEDAR);
   else CHECK_FAMILY(REDWOOD);
   else CHECK_FAMILY(JUNIPER);
   else CHECK_FAMILY(CYPRESS);
   else CHECK_FAMILY(HEMLOCK);
   else CHECK_FAMILY(PALM);
   else CHECK_FAMILY(SUMO);
   else CHECK_FAMILY(SUMO2);
   else CHECK_FAMILY(BARTS);
   else CHECK_FAMILY(TURKS);
   else CHECK_FAMILY(CAICOS);
   /* GFX5 (Northern Islands) */
   else CHECK_FAMILY(CAYMAN);
   else CHECK_FAMILY(ARUBA);
   else
            }
      void
   Shader::allocate_reserved_registers()
   {
      m_instr_factory->value_factory().set_virtual_register_base(0);
   auto reserved_registers_end = do_allocate_reserved_registers();
   m_instr_factory->value_factory().set_virtual_register_base(reserved_registers_end);
   if (!m_atomics.empty()) {
      m_atomic_update = value_factory().temp_register();
   auto alu = new AluInstr(op1_mov,
                     alu->set_alu_flag(alu_no_schedule_bias);
               if (m_flags.test(sh_needs_sbo_ret_address)) {
      m_rat_return_address = value_factory().temp_register(0);
   auto temp0 = value_factory().temp_register(0);
   auto temp1 = value_factory().temp_register(1);
            auto group = new AluGroup();
   group->add_instruction(new AluInstr(
         group->add_instruction(new AluInstr(
         emit_instruction(group);
   emit_instruction(new AluInstr(op3_muladd_uint24,
                                 emit_instruction(new AluInstr(op3_muladd_uint24,
                                 }
      Shader *
   Shader::translate_from_nir(nir_shader *nir,
                                 {
               switch (nir->info.stage) {
   case MESA_SHADER_FRAGMENT:
      if (chip_class >= ISA_CC_EVERGREEN)
         else
            case MESA_SHADER_VERTEX:
      shader = new VertexShader(so_info, gs_shader, key);
      case MESA_SHADER_GEOMETRY:
      shader = new GeometryShader(key);
      case MESA_SHADER_TESS_CTRL:
      shader = new TCSShader(key);
      case MESA_SHADER_TESS_EVAL:
      shader = new TESShader(so_info, gs_shader, key);
      case MESA_SHADER_KERNEL:
   case MESA_SHADER_COMPUTE:
      shader = new ComputeShader(key, BITSET_COUNT(nir->info.samplers_used));
      default:
                           shader->set_chip_class(chip_class);
            if (!shader->process(nir))
               }
      void
   Shader::set_info(nir_shader *nir)
   {
         }
      ValueFactory&
   Shader::value_factory()
   {
         }
      bool
   Shader::process(nir_shader *nir)
   {
               if (nir->info.use_legacy_math_rules)
                     // at this point all functions should be inlined
   const nir_function *func =
            if (!scan_shader(func))
                     value_factory().allocate_registers(m_register_allocations);
            sfn_log << SfnLog::trans << "Process shader \n";
   foreach_list_typed(nir_cf_node, node, node, &func->impl->body)
   {
      if (!process_cf_node(node))
                           }
      bool
   Shader::scan_shader(const nir_function *func)
   {
         nir_foreach_block(block, func->impl)
   {
      nir_foreach_instr(instr, block)
   {
      if (!scan_instruction(instr)) {
      fprintf(stderr, "Unhandled sysvalue access ");
   nir_print_instr(instr, stderr);
   fprintf(stderr, "\n");
                     int lds_pos = 0;
   for (auto& [index, input] : m_inputs) {
      if (input.need_lds_pos()) {
      if (chip_class() < ISA_CC_EVERGREEN)
                        int param_id = 0;
   for (auto& [index, out] : m_outputs) {
      if (out.is_param())
                  }
      bool
   Shader::scan_uniforms(nir_variable *uniform)
   {
      if (uniform->type->contains_atomic()) {
      int natomics = uniform->type->atomic_size() / 4; /* ATOMIC_COUNTER_SIZE */
            if (uniform->type->is_array())
                              atom.buffer_id = uniform->data.binding;
            atom.start = uniform->data.offset >> 2;
            if (m_atomic_base_map.find(uniform->data.binding) == m_atomic_base_map.end())
                                                   auto type = uniform->type->is_array() ? uniform->type->without_array() : uniform->type;
   if (type->is_image() || uniform->data.mode == nir_var_mem_ssbo) {
      m_flags.set(sh_uses_images);
   if (uniform->type->is_array() && !(uniform->data.mode == nir_var_mem_ssbo))
                  }
      bool
   Shader::scan_instruction(nir_instr *instr)
   {
      if (do_scan_instruction(instr))
            if (instr->type != nir_instr_type_intrinsic)
                     // handle unhandled instructions
   switch (intr->intrinsic) {
   case nir_intrinsic_ssbo_atomic:
   case nir_intrinsic_ssbo_atomic_swap:
   case nir_intrinsic_image_load:
   case nir_intrinsic_image_atomic:
   case nir_intrinsic_image_atomic_swap:
      m_flags.set(sh_needs_sbo_ret_address);
      case nir_intrinsic_image_store:
   case nir_intrinsic_store_ssbo:
      m_flags.set(sh_writes_memory);
   m_flags.set(sh_uses_images);
      case nir_intrinsic_barrier:
      m_chain_instr.prepare_mem_barrier |=
         (nir_intrinsic_memory_modes(intr) &
   (nir_var_mem_ssbo | nir_var_mem_global | nir_var_image) &&
      case nir_intrinsic_decl_reg:
      m_register_allocations.push_back(intr);
      default:;
   }
      }
      bool
   Shader::process_cf_node(nir_cf_node *node)
   {
               switch (node->type) {
   case nir_cf_node_block:
         case nir_cf_node_if:
         case nir_cf_node_loop:
         default:
            }
      static bool
   child_block_empty(const exec_list& list)
   {
      if (list.is_empty())
                     foreach_list_typed(nir_cf_node, n, node, &list)
               if (n->type == nir_cf_node_block) {
      if (!nir_cf_node_as_block(n)->instr_list.is_empty())
      }
   if (n->type == nir_cf_node_if)
      }
      }
      static bool value_has_non_const_source(VirtualValue *value)
   {
      auto reg = value->as_register();
   if (reg) {
      // Non-ssa registers are probably the result of some control flow
   // that makes the values non-uniform across the work group
   if (!reg->has_flag(Register::ssa))
            for (const auto& p : reg->parents()) {
      auto alu = p->as_alu();
   if (alu) {
      for (auto& s : p->as_alu()->sources()) {
            } else {
               }
      }
      bool
   Shader::process_if(nir_if *if_stmt)
   {
                                 EAluOp op = child_block_empty(if_stmt->then_list) ? op2_prede_int :
            AluInstr *pred = new AluInstr(op,
                           pred->set_alu_flag(alu_update_exec);
   pred->set_alu_flag(alu_update_pred);
            IfInstr *ir = new IfInstr(pred);
   emit_instruction(ir);
   if (non_const_cond)
                  if (!child_block_empty(if_stmt->then_list)) {
      foreach_list_typed(nir_cf_node, n, node, &if_stmt->then_list)
   {
      SFN_TRACE_FUNC(SfnLog::flow, "IF-then");
   if (!process_cf_node(n))
      }
   if (!child_block_empty(if_stmt->else_list)) {
      if (!emit_control_flow(ControlFlowInstr::cf_else))
         foreach_list_typed(nir_cf_node,
                           } else {
      assert(!child_block_empty(if_stmt->else_list));
   foreach_list_typed(nir_cf_node,
                                 if (!emit_control_flow(ControlFlowInstr::cf_endif))
            if (non_const_cond)
               }
      bool
   Shader::emit_control_flow(ControlFlowInstr::CFType type)
   {
      auto ir = new ControlFlowInstr(type);
   emit_instruction(ir);
   int depth = 0;
   switch (type) {
   case ControlFlowInstr::cf_loop_begin:
      m_loops.push_back(ir);
   m_nloops++;
   depth = 1;
      case ControlFlowInstr::cf_loop_end:
      m_loops.pop_back();
      case ControlFlowInstr::cf_endif:
      depth = -1;
      default:;
            start_new_block(depth);
      }
      bool
   Shader::process_loop(nir_loop *node)
   {
      assert(!nir_loop_has_continue_construct(node));
   SFN_TRACE_FUNC(SfnLog::flow, "LOOP");
   if (!emit_control_flow(ControlFlowInstr::cf_loop_begin))
            foreach_list_typed(nir_cf_node,
                        if (!emit_control_flow(ControlFlowInstr::cf_loop_end))
               }
      bool
   Shader::process_block(nir_block *block)
   {
               nir_foreach_instr(instr, block)
   {
      sfn_log << SfnLog::instr << "FROM:" << *instr << "\n";
   bool r = process_instr(instr);
   if (!r) {
      sfn_log << SfnLog::err << "R600: Unsupported instruction: " << *instr << "\n";
         }
      }
      bool
   Shader::process_instr(nir_instr *instr)
   {
         }
      bool
   Shader::process_intrinsic(nir_intrinsic_instr *intr)
   {
      if (process_stage_intrinsic(intr))
            if (GDSInstr::emit_atomic_counter(intr, *this)) {
      set_flag(sh_writes_memory);
               if (RatInstr::emit(intr, *this))
            switch (intr->intrinsic) {
   case nir_intrinsic_store_output:
         case nir_intrinsic_load_input:
         case nir_intrinsic_load_ubo_vec4:
         case nir_intrinsic_store_scratch:
         case nir_intrinsic_load_scratch:
         case nir_intrinsic_store_local_shared_r600:
         case nir_intrinsic_load_global:
   case nir_intrinsic_load_global_constant:
         case nir_intrinsic_load_local_shared_r600:
         case nir_intrinsic_load_tcs_in_param_base_r600:
         case nir_intrinsic_load_tcs_out_param_base_r600:
         case nir_intrinsic_barrier:
         case nir_intrinsic_shared_atomic:
   case nir_intrinsic_shared_atomic_swap:
         case nir_intrinsic_shader_clock:
         case nir_intrinsic_load_reg:
         case nir_intrinsic_load_reg_indirect:
         case nir_intrinsic_store_reg:
         case nir_intrinsic_store_reg_indirect:
         case nir_intrinsic_decl_reg:
      // Registers and arrays are allocated at
   // conversion startup time
      default:
            }
      static ESDOp
   lds_op_from_intrinsic(nir_atomic_op op, bool ret)
   {
      switch (op) {
   case nir_atomic_op_iadd:
         case nir_atomic_op_iand:
         case nir_atomic_op_ior:
         case nir_atomic_op_imax:
         case nir_atomic_op_umax:
         case nir_atomic_op_imin:
         case nir_atomic_op_umin:
         case nir_atomic_op_ixor:
         case nir_atomic_op_xchg:
         case nir_atomic_op_cmpxchg:
         default:
            }
      PRegister
   Shader::emit_load_to_register(PVirtualValue src)
   {
      assert(src);
            if (!dest) {
      dest = value_factory().temp_register();
      }
      }
      // add visitor to resolve array and register
   class RegisterAccessHandler : public RegisterVisitor {
      public:
               void visit(LocalArrayValue& value) override {(void)value; assert(0);}
   void visit(UniformValue& value) override {(void)value; assert(0);}
   void visit(LiteralConstant& value) override {(void)value; assert(0);}
            Shader& sh;
   nir_intrinsic_instr *ir;
   PVirtualValue addr{nullptr};
      };
      class RegisterReadHandler : public RegisterAccessHandler {
      public:
      using RegisterAccessHandler::RegisterAccessHandler;
            void visit(LocalArray& value) override;
      };
      bool Shader::emit_load_reg(nir_intrinsic_instr *intr)
   {
      RegisterReadHandler visitor(*this, intr);
   auto handle = value_factory().src(intr->src[0], 0);
   handle->accept(visitor);
      }
      bool Shader::emit_load_reg_indirect(nir_intrinsic_instr *intr)
   {
      RegisterReadHandler visitor(*this, intr);
   visitor.addr =  value_factory().src(intr->src[1], 0);
   auto handle = value_factory().src(intr->src[0], 0);
   handle->accept(visitor);
      }
      class RegisterWriteHandler : public RegisterAccessHandler {
      public:
      using RegisterAccessHandler::RegisterAccessHandler;
            void visit(LocalArray& value) override;
      };
         bool Shader::emit_store_reg(nir_intrinsic_instr *intr)
   {
      RegisterWriteHandler visitor(*this, intr);
   auto handle = value_factory().src(intr->src[1], 0);
   handle->accept(visitor);
      }
      bool Shader::emit_store_reg_indirect(nir_intrinsic_instr *intr)
   {
      RegisterWriteHandler visitor(*this, intr);
            auto handle = value_factory().src(intr->src[1], 0);
   handle->accept(visitor);
      }
      RegisterAccessHandler::RegisterAccessHandler(Shader& shader, nir_intrinsic_instr *intr):
      sh(shader),
      {}
      void RegisterReadHandler::visit(LocalArray& array)
   {
      int slots =  ir->def.bit_size / 32;
   auto pin = ir->def.num_components > 1 ? pin_none : pin_free;
   for (int i = 0; i < ir->def.num_components; ++i) {
      for (int s = 0; s < slots; ++s) {
      int chan = i * slots + s;
   auto dest = sh.value_factory().dest(ir->def, chan, pin);
   auto src = array.element(nir_intrinsic_base(ir), addr, chan);
            }
      void RegisterReadHandler::visit(Register& reg)
   {
      auto dest = sh.value_factory().dest(ir->def, 0, pin_free);
      }
      void RegisterWriteHandler::visit(LocalArray& array)
   {
      int writemask = nir_intrinsic_write_mask(ir);
            for (int i = 0; i < ir->num_components; ++i) {
      if (!(writemask & (1 << i)))
         for (int s = 0; s < slots; ++s) {
               auto dest = array.element(nir_intrinsic_base(ir), addr, chan);
   auto src = sh.value_factory().src(ir->src[0], chan);
            }
      void RegisterWriteHandler::visit(Register& dest)
   {
      int writemask = nir_intrinsic_write_mask(ir);
   assert(writemask == 1);
   auto src = sh.value_factory().src(ir->src[0], 0);
      }
      bool
   Shader::emit_atomic_local_shared(nir_intrinsic_instr *instr)
   {
                                          /* For these two instructions we don't have opcodes that don't read back
   * the result, so we have to add a dummy-readback to remove the the return
   * value from read queue. */
   if (!uses_retval &&
      (op == LDS_XCHG_RET || op == LDS_CMP_XCHG_RET)) {
                        AluInstr::SrcValues src;
            if (unlikely(instr->intrinsic == nir_intrinsic_shared_atomic_swap))
         emit_instruction(new LDSAtomicInstr(op, dest_value, address, src));
      }
      auto
   Shader::evaluate_resource_offset(nir_intrinsic_instr *instr, int src_id)
         {
               PRegister uav_id{nullptr};
   int offset = nir_intrinsic_has_range_base(instr) ?
            auto uav_id_const = nir_src_as_const_value(instr->src[src_id]);
   if (uav_id_const) {
         } else {
      auto uav_id_val = vf.src(instr->src[src_id], 0);
   if (uav_id_val->as_register()) {
         } else {
      uav_id = vf.temp_register();
         }
      }
      bool
   Shader::emit_store_scratch(nir_intrinsic_instr *intr)
   {
                                 for (unsigned i = 0; i < intr->num_components; ++i)
            auto value = vf.temp_vec4(pin_group, swz);
   AluInstr *ir = nullptr;
   for (unsigned i = 0; i < intr->num_components; ++i) {
      if (value[i]->chan() < 4) {
      ir = new AluInstr(op1_mov, value[i], vf.src(intr->src[0], i), AluInstr::write);
   ir->set_alu_flag(alu_no_schedule_bias);
         }
   if (!ir)
                              int align = nir_intrinsic_align_mul(intr);
                     int offset = -1;
   if (address->as_literal()) {
         } else if (address->as_inline_const()) {
      auto il = address->as_inline_const();
   if (il->sel() == ALU_SRC_0)
         else if (il->sel() == ALU_SRC_1_INT)
               if (offset >= 0) {
         } else {
      auto addr_temp = vf.temp_register(0);
   auto load_addr = new AluInstr(op1_mov, addr_temp, address, AluInstr::last_write);
   load_addr->set_alu_flag(alu_no_schedule_bias);
            ws_ir = new ScratchIOInstr(
      }
            m_flags.set(sh_needs_scratch_space);
      }
      bool
   Shader::emit_load_scratch(nir_intrinsic_instr *intr)
   {
      auto addr = value_factory().src(intr->src[0], 0);
            if (chip_class() >= ISA_CC_R700) {
               for (unsigned i = 0; i < intr->num_components; ++i)
            auto *ir = new LoadFromScratch(dest, dest_swz, addr, m_scratch_size);
   emit_instruction(ir);
      } else {
      int align = nir_intrinsic_align_mul(intr);
            int offset = -1;
   if (addr->as_literal()) {
         } else if (addr->as_inline_const()) {
      auto il = addr->as_inline_const();
   if (il->sel() == ALU_SRC_0)
         else if (il->sel() == ALU_SRC_1_INT)
               ScratchIOInstr *ir = nullptr;
   if (offset >= 0) {
         } else {
      auto addr_temp = value_factory().temp_register(0);
   auto load_addr = new AluInstr(op1_mov, addr_temp, addr, AluInstr::last_write);
                  ir = new ScratchIOInstr(
      }
                           }
      bool Shader::emit_load_global(nir_intrinsic_instr *intr)
   {
               auto src_value = value_factory().src(intr->src[0], 0);
   auto src = src_value->as_register();
   if (!src) {
      src = value_factory().temp_register();
      }
   auto load = new LoadFromBuffer(dest, {0,7,7,7}, src, 0, 1, NULL, fmt_32);
   load->set_mfc(4);
   load->set_num_format(vtx_nf_int);
            emit_instruction(load);
      }
      bool
   Shader::emit_local_store(nir_intrinsic_instr *instr)
   {
               auto address = value_factory().src(instr->src[1], 0);
   int swizzle_base = 0;
   unsigned w = write_mask;
   while (!(w & 1)) {
      ++swizzle_base;
      }
            if ((write_mask & 3) != 3) {
      auto value = value_factory().src(instr->src[0], swizzle_base);
      } else {
      auto value = value_factory().src(instr->src[0], swizzle_base);
   auto value1 = value_factory().src(instr->src[0], swizzle_base + 1);
   emit_instruction(
      }
      }
      bool
   Shader::emit_local_load(nir_intrinsic_instr *instr)
   {
      auto address = value_factory().src_vec(instr->src[0], instr->num_components);
   auto dest_value = value_factory().dest_vec(instr->def, instr->num_components);
   emit_instruction(new LDSReadInstr(dest_value, address));
      }
      void
   Shader::chain_scratch_read(Instr *instr)
   {
         }
      void
   Shader::chain_ssbo_read(Instr *instr)
   {
         }
      bool
   Shader::emit_wait_ack()
   {
      start_new_block(0);
   emit_instruction(new ControlFlowInstr(ControlFlowInstr::cf_wait_ack));
   start_new_block(0);
      }
      static uint32_t get_array_hash(const VirtualValue& value)
   {
      assert (value.pin() == pin_array);
   const LocalArrayValue& av = static_cast<const LocalArrayValue&>(value);
      }
      void Shader::InstructionChain::visit(AluInstr *instr)
   {
      if (instr->is_kill()) {
               // these instructions have side effects, they should
   // not be re-order with kill
   if (last_gds_instr)
            if (last_ssbo_instr)
               /* Make sure array reads and writes depends on the last indirect access
            if (auto d = instr->dest()) {
      if (d->pin() == pin_array) {
      if (d->addr()) {
      last_alu_with_indirect_reg[get_array_hash(*d)] = instr;
      }
   auto pos = last_alu_with_indirect_reg.find(get_array_hash(*d));
   if (pos != last_alu_with_indirect_reg.end()) {
                        for (auto& s : instr->sources()) {
      if (s->pin() == pin_array) {
      if (s->get_addr()) {
      last_alu_with_indirect_reg[get_array_hash(*s)] = instr;
      }
   auto pos = last_alu_with_indirect_reg.find(get_array_hash(*s));
   if (pos != last_alu_with_indirect_reg.end()) {
                  }
      void
   Shader::InstructionChain::visit(ScratchIOInstr *instr)
   {
         }
      void
   Shader::InstructionChain::visit(GDSInstr *instr)
   {
      apply(instr, &last_gds_instr);
   Instr::Flags flag = instr->has_instr_flag(Instr::helper) ? Instr::helper : Instr::vpm;
   for (auto& loop : this_shader->m_loops) {
         }
   if (last_kill_instr)
         }
      void
   Shader::InstructionChain::visit(RatInstr *instr)
   {
      apply(instr, &last_ssbo_instr);
   Instr::Flags flag = instr->has_instr_flag(Instr::helper) ? Instr::helper : Instr::vpm;
   for (auto& loop : this_shader->m_loops) {
                  if (prepare_mem_barrier)
            if (this_shader->m_current_block->inc_rat_emitted() > 15)
            if (last_kill_instr)
      }
      void
   Shader::InstructionChain::apply(Instr *current, Instr **last)
   {
      if (*last)
            }
      void
   Shader::emit_instruction(PInst instr)
   {
      sfn_log << SfnLog::instr << "   " << *instr << "\n";
   instr->accept(m_chain_instr);
      }
      bool
   Shader::emit_load_tcs_param_base(nir_intrinsic_instr *instr, int offset)
   {
      auto src = value_factory().temp_register();
   emit_instruction(
            auto dest = value_factory().dest_vec4(instr->def, pin_group);
   auto fetch = new LoadFromBuffer(dest,
                                          fetch->set_fetch_flag(LoadFromBuffer::srf_mode);
               }
      bool
   Shader::emit_shader_clock(nir_intrinsic_instr *instr)
   {
      auto& vf = value_factory();
   auto group = new AluGroup();
   group->add_instruction(new AluInstr(op1_mov,
                     group->add_instruction(new AluInstr(op1_mov,
                     emit_instruction(group);
      }
      bool
   Shader::emit_group_barrier(nir_intrinsic_instr *intr)
   {
      assert(m_control_flow_depth == 0);
   (void)intr;
   /* Put barrier into it's own block, so that optimizers and the
   * scheduler don't move code */
   start_new_block(0);
   auto op = new AluInstr(op0_group_barrier, 0);
   op->set_alu_flag(alu_last_instr);
   emit_instruction(op);
   start_new_block(0);
      }
      bool Shader::emit_barrier(nir_intrinsic_instr *intr)
   {
         if ((nir_intrinsic_execution_scope(intr) == SCOPE_WORKGROUP)) {
      if (!emit_group_barrier(intr))
               /* We don't check nir_var_mem_shared because we don't emit a real barrier -
   * for this we need to implement GWS (Global Wave Sync).
   * Here we just emit a wait_ack - this is no real barrier,
   * it's just a wait for RAT writes to be finished (if they
   * are emitted with the _ACK opcode and the `mark` flag set - it
   * is very likely that WAIT_ACK is also only relevant for this
   * shader instance). */
            if ((nir_intrinsic_memory_scope(intr) != SCOPE_NONE) &&
      (nir_intrinsic_memory_modes(intr) & full_barrier_mem_modes)) {
                  }
      bool
   Shader::load_ubo(nir_intrinsic_instr *instr)
   {
      auto bufid = nir_src_as_const_value(instr->src[0]);
   auto buf_offset = nir_src_as_const_value(instr->src[1]);
            if (!buf_offset) {
      /* TODO: if bufid is constant then this can also be solved by using the
   * CF index on the ALU block, and this would probably make sense when
            auto addr = value_factory().src(instr->src[1], 0)->as_register();
   RegisterVec4::Swizzle dest_swz{7, 7, 7, 7};
            for (unsigned i = 0; i < instr->def.num_components; ++i) {
                  LoadFromBuffer *ir;
   if (bufid) {
      ir = new LoadFromBuffer(
      } else {
      auto buffer_id = emit_load_to_register(value_factory().src(instr->src[0], 0));
   ir = new LoadFromBuffer(
      }
   emit_instruction(ir);
               /* direct load using the constant cache */
   if (bufid) {
               AluInstr *ir = nullptr;
   auto pin = instr->def.num_components == 1
                                          auto uniform =
         ir = new AluInstr(op1_mov,
                        }
   if (ir)
            } else {
      int buf_cmp = nir_intrinsic_component(instr);
   AluInstr *ir = nullptr;
            for (unsigned i = 0; i < instr->def.num_components; ++i) {
      int cmp = buf_cmp + i;
   auto u =
         auto dest = value_factory().dest(instr->def, i, pin_none);
   ir = new AluInstr(op1_mov, dest, u, AluInstr::write);
      }
   if (ir)
         m_indirect_files |= 1 << TGSI_FILE_CONSTANT;
         }
      void
   Shader::start_new_block(int depth)
   {
      int depth_offset = m_current_block ? m_current_block->nesting_depth() : 0;
   m_current_block = new Block(depth + depth_offset, m_next_block++);
      }
      bool
   Shader::emit_simple_mov(nir_def& def, int chan, PVirtualValue src, Pin pin)
   {
      auto dst = value_factory().dest(def, chan, pin);
   emit_instruction(new AluInstr(op1_mov, dst, src, AluInstr::last_write));
      }
      void
   Shader::print(std::ostream& os) const
   {
               for (auto& [dummy, i] : m_inputs) {
      i.print(os);
               for (auto& [dummy, o] : m_outputs) {
      o.print(os);
               os << "SHADER\n";
   for (auto& b : m_root)
      }
      const char *chip_class_names[] = {"R600", "R700", "EVERGREEN", "CAYMAN"};
      void
   Shader::print_header(std::ostream& os) const
   {
      assert(m_chip_class <= ISA_CC_CAYMAN);
   os << m_type_id << "\n";
   os << "CHIPCLASS " << chip_class_names[m_chip_class] << "\n";
      }
      void
   Shader::print_properties(std::ostream& os) const
   {
         }
      bool
   Shader::equal_to(const Shader& other) const
   {
      if (m_root.size() != other.m_root.size())
         return std::inner_product(
      m_root.begin(),
   m_root.end(),
   other.m_root.begin(),
   true,
   [](bool lhs, bool rhs) { return lhs & rhs; },
   [](const Block::Pointer lhs, const Block::Pointer rhs) -> bool {
         }
      void
   Shader::get_shader_info(r600_shader *sh_info)
   {
      sh_info->ninput = m_inputs.size();
   int lds_pos = 0;
   int input_array_array_loc = 0;
   for (auto& [index, info] : m_inputs) {
               io.sid = info.sid();
   io.gpr = info.gpr();
   io.spi_sid = info.spi_sid();
   io.ij_index = info.ij_index();
   io.name = info.name();
   io.interpolate = info.interpolator();
   io.interpolate_location = info.interpolate_loc();
   if (info.need_lds_pos())
         else
            io.ring_offset = info.ring_offset();
            sfn_log << SfnLog::io << "Emit Input [" << index << "] sid:" << io.sid
                     sh_info->nlds = lds_pos;
   sh_info->noutput = m_outputs.size();
   sh_info->num_loops = m_nloops;
            for (auto& [index, info] : m_outputs) {
      r600_shader_io& io = sh_info->output[output_array_array_loc++];
   io.sid = info.sid();
   io.gpr = info.gpr();
   io.spi_sid = info.spi_sid();
   io.name = info.name();
            sfn_log << SfnLog::io << "Emit output[" << index << "] sid:" << io.sid
                     sh_info->nhwatomic = m_nhwatomic;
   sh_info->atomic_base = m_atomic_base;
   sh_info->nhwatomic_ranges = m_atomics.size();
   for (unsigned i = 0; i < m_atomics.size(); ++i)
            if (m_flags.test(sh_indirect_const_file))
            if (m_flags.test(sh_indirect_atomic))
                              sh_info->needs_scratch_space = m_flags.test(sh_needs_scratch_space);
   sh_info->uses_images = m_flags.test(sh_uses_images);
   sh_info->uses_atomics = m_flags.test(sh_uses_atomics);
   sh_info->disable_sb = m_flags.test(sh_disble_sb);
   sh_info->has_txq_cube_array_z_comp = m_flags.test(sh_txs_cube_array_comp);
   sh_info->indirect_files = m_indirect_files;
      }
      PRegister
   Shader::atomic_update()
   {
      assert(m_atomic_update);
      }
      int
   Shader::remap_atomic_base(int base)
   {
         }
      void
   Shader::do_get_shader_info(r600_shader *sh_info)
   {
         }
      const ShaderInput&
   Shader::input(int base) const
   {
      auto io = m_inputs.find(base);
   assert(io != m_inputs.end());
      }
      const ShaderOutput&
   Shader::output(int base) const
   {
      auto io = m_outputs.find(base);
   assert(io != m_outputs.end());
      }
      LiveRangeMap
   Shader::prepare_live_range_map()
   {
         }
      void
   Shader::reset_function(ShaderBlocks& new_root)
   {
         }
      void
   Shader::finalize()
   {
         }
      void
   Shader::do_finalize()
   {
   }
      } // namespace r600
