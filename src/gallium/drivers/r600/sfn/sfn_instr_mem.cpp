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
      #include "sfn_instr_mem.h"
      #include "nir_intrinsics.h"
   #include "nir_intrinsics_indices.h"
   #include "sfn_alu_defines.h"
   #include "sfn_instr_alu.h"
   #include "sfn_instr_fetch.h"
   #include "sfn_instr_tex.h"
   #include "sfn_shader.h"
   #include "sfn_virtualvalues.h"
      namespace r600 {
      GDSInstr::GDSInstr(
      ESDOp op, Register *dest, const RegisterVec4& src, int uav_base, PRegister uav_id):
   Resource(this, uav_base, uav_id),
   m_op(op),
   m_dest(dest),
      {
               m_src.add_use(this);
   if (m_dest)
      }
      bool
   GDSInstr::is_equal_to(const GDSInstr& rhs) const
   {
   #define NE(X) (X != rhs.X)
         if (NE(m_op) || NE(m_src))
                        }
      void
   GDSInstr::accept(ConstInstrVisitor& visitor) const
   {
         }
      void
   GDSInstr::accept(InstrVisitor& visitor)
   {
         }
      bool
   GDSInstr::do_ready() const
   {
         }
      void
   GDSInstr::do_print(std::ostream& os) const
   {
      os << "GDS " << lds_ops.at(m_op).name;
   if (m_dest)
         else
         os << " " << m_src;
               }
      bool
   GDSInstr::emit_atomic_counter(nir_intrinsic_instr *intr, Shader& shader)
   {
      switch (intr->intrinsic) {
   case nir_intrinsic_atomic_counter_add:
   case nir_intrinsic_atomic_counter_and:
   case nir_intrinsic_atomic_counter_exchange:
   case nir_intrinsic_atomic_counter_max:
   case nir_intrinsic_atomic_counter_min:
   case nir_intrinsic_atomic_counter_or:
   case nir_intrinsic_atomic_counter_xor:
   case nir_intrinsic_atomic_counter_comp_swap:
         case nir_intrinsic_atomic_counter_read:
   case nir_intrinsic_atomic_counter_post_dec:
         case nir_intrinsic_atomic_counter_inc:
         case nir_intrinsic_atomic_counter_pre_dec:
         default:
            }
      uint8_t GDSInstr::allowed_src_chan_mask() const
   {
         }
      static ESDOp
   get_opcode(const nir_intrinsic_op opcode)
   {
      switch (opcode) {
   case nir_intrinsic_atomic_counter_add:
         case nir_intrinsic_atomic_counter_and:
         case nir_intrinsic_atomic_counter_exchange:
         case nir_intrinsic_atomic_counter_inc:
         case nir_intrinsic_atomic_counter_max:
         case nir_intrinsic_atomic_counter_min:
         case nir_intrinsic_atomic_counter_or:
         case nir_intrinsic_atomic_counter_read:
         case nir_intrinsic_atomic_counter_xor:
         case nir_intrinsic_atomic_counter_post_dec:
         case nir_intrinsic_atomic_counter_comp_swap:
         case nir_intrinsic_atomic_counter_pre_dec:
   default:
            }
      static ESDOp
   get_opcode_wo(const nir_intrinsic_op opcode)
   {
      switch (opcode) {
   case nir_intrinsic_atomic_counter_add:
         case nir_intrinsic_atomic_counter_and:
         case nir_intrinsic_atomic_counter_inc:
         case nir_intrinsic_atomic_counter_max:
         case nir_intrinsic_atomic_counter_min:
         case nir_intrinsic_atomic_counter_or:
         case nir_intrinsic_atomic_counter_xor:
         case nir_intrinsic_atomic_counter_post_dec:
         case nir_intrinsic_atomic_counter_comp_swap:
         case nir_intrinsic_atomic_counter_exchange:
         case nir_intrinsic_atomic_counter_pre_dec:
   default:
            }
      bool
   GDSInstr::emit_atomic_op2(nir_intrinsic_instr *instr, Shader& shader)
   {
      auto& vf = shader.value_factory();
            ESDOp op =
            if (DS_OP_INVALID == op)
            auto [offset, uav_id] = shader.evaluate_resource_offset(instr, 0);
   {
   }
                     PRegister src_as_register = nullptr;
   auto src_val = vf.src(instr->src[1], 0);
   if (!src_val->as_register()) {
      auto temp_src_val = vf.temp_register();
   shader.emit_instruction(
            } else
            if (uav_id != nullptr)
            GDSInstr *ir = nullptr;
   if (shader.chip_class() < ISA_CC_CAYMAN) {
      RegisterVec4 src(nullptr, src_as_register, nullptr, nullptr, pin_free);
         } else {
      auto dest = vf.dest(instr->def, 0, pin_free);
   auto tmp = vf.temp_vec4(pin_group, {0, 1, 7, 7});
   if (uav_id)
      shader.emit_instruction(new AluInstr(op3_muladd_uint24,
                              else
      shader.emit_instruction(
      shader.emit_instruction(
            }
   shader.emit_instruction(ir);
      }
      bool
   GDSInstr::emit_atomic_read(nir_intrinsic_instr *instr, Shader& shader)
   {
               auto [offset, uav_id] = shader.evaluate_resource_offset(instr, 0);
   {
   }
                              if (shader.chip_class() < ISA_CC_CAYMAN) {
      RegisterVec4 src = RegisterVec4(0, true, {7, 7, 7, 7});
      } else {
      auto tmp = vf.temp_vec4(pin_group, {0, 7, 7, 7});
   if (uav_id)
      shader.emit_instruction(new AluInstr(op3_muladd_uint24,
                              else
                              shader.emit_instruction(ir);
      }
      bool
   GDSInstr::emit_atomic_inc(nir_intrinsic_instr *instr, Shader& shader)
   {
      auto& vf = shader.value_factory();
            auto [offset, uav_id] = shader.evaluate_resource_offset(instr, 0);
   {
   }
            GDSInstr *ir = nullptr;
            if (shader.chip_class() < ISA_CC_CAYMAN) {
            ir =
      } else {
               if (uav_id)
      shader.emit_instruction(new AluInstr(op3_muladd_uint24,
                              else
                  shader.emit_instruction(
            }
   shader.emit_instruction(ir);
      }
      bool
   GDSInstr::emit_atomic_pre_dec(nir_intrinsic_instr *instr, Shader& shader)
   {
                                 auto [offset, uav_id] = shader.evaluate_resource_offset(instr, 0);
   {
   }
                                 if (shader.chip_class() < ISA_CC_CAYMAN) {
      RegisterVec4 src(nullptr, shader.atomic_update(), nullptr, nullptr, pin_chan);
      } else {
      auto tmp = vf.temp_vec4(pin_group, {0, 1, 7, 7});
   if (uav_id)
      shader.emit_instruction(new AluInstr(op3_muladd_uint24,
                              else
                  shader.emit_instruction(
                     shader.emit_instruction(ir);
   if (read_result)
      shader.emit_instruction(new AluInstr(op2_sub_int,
                           }
      void GDSInstr::update_indirect_addr(PRegister old_reg, PRegister addr)
   {
      (void)old_reg;
      }
      RatInstr::RatInstr(ECFOpCode cf_opcode,
                     ERatOp rat_op,
   const RegisterVec4& data,
   const RegisterVec4& index,
   int rat_id,
   PRegister rat_id_offset,
      Resource(this, rat_id, rat_id_offset),
   m_cf_opcode(cf_opcode),
   m_rat_op(rat_op),
   m_data(data),
   m_index(index),
   m_burst_count(burst_count),
   m_comp_mask(comp_mask),
      {
      set_always_keep();
   m_data.add_use(this);
      }
      void
   RatInstr::accept(ConstInstrVisitor& visitor) const
   {
         }
      void
   RatInstr::accept(InstrVisitor& visitor)
   {
         }
      bool
   RatInstr::is_equal_to(const RatInstr& lhs) const
   {
      (void)lhs;
   assert(0);
      }
      bool
   RatInstr::do_ready() const
   {
      if (m_rat_op != STORE_TYPED) {
      for (auto i : required_instr()) {
      if (!i->is_scheduled()) {
                           }
      void
   RatInstr::do_print(std::ostream& os) const
   {
      os << "MEM_RAT RAT " << resource_id();
   print_resource_offset(os);
   os << " @" << m_index;
   os << " OP:" << m_rat_op << " " << m_data;
   os << " BC:" << m_burst_count << " MASK:" << m_comp_mask << " ES:" << m_element_size;
   if (m_need_ack)
      }
      void RatInstr::update_indirect_addr(UNUSED PRegister old_reg, PRegister addr)
   {
         }
      static RatInstr::ERatOp
   get_rat_opcode(const nir_atomic_op opcode)
   {
      switch (opcode) {
   case nir_atomic_op_iadd:
         case nir_atomic_op_iand:
         case nir_atomic_op_ior:
         case nir_atomic_op_imin:
         case nir_atomic_op_imax:
         case nir_atomic_op_umin:
         case nir_atomic_op_umax:
         case nir_atomic_op_ixor:
         case nir_atomic_op_cmpxchg:
         case nir_atomic_op_xchg:
         default:
            }
      static RatInstr::ERatOp
   get_rat_opcode_wo(const nir_atomic_op opcode)
   {
      switch (opcode) {
   case nir_atomic_op_iadd:
         case nir_atomic_op_iand:
         case nir_atomic_op_ior:
         case nir_atomic_op_imin:
         case nir_atomic_op_imax:
         case nir_atomic_op_umin:
         case nir_atomic_op_umax:
         case nir_atomic_op_ixor:
         case nir_atomic_op_cmpxchg:
         case nir_atomic_op_xchg:
         default:
            }
      bool
   RatInstr::emit(nir_intrinsic_instr *intr, Shader& shader)
   {
      switch (intr->intrinsic) {
   case nir_intrinsic_load_ssbo:
         case nir_intrinsic_store_ssbo:
         case nir_intrinsic_ssbo_atomic:
   case nir_intrinsic_ssbo_atomic_swap:
         case nir_intrinsic_store_global:
         case nir_intrinsic_image_store:
         case nir_intrinsic_image_load:
   case nir_intrinsic_image_atomic:
   case nir_intrinsic_image_atomic_swap:
         case nir_intrinsic_image_size:
         case nir_intrinsic_image_samples:
         case nir_intrinsic_get_ssbo_size:
         default:
            }
      bool
   RatInstr::emit_ssbo_load(nir_intrinsic_instr *intr, Shader& shader)
   {
      auto& vf = shader.value_factory();
            /** src0 not used, should be some offset */
   auto addr = vf.src(intr->src[1], 0);
            /** Should be lowered in nir */
   shader.emit_instruction(new AluInstr(
                     RegisterVec4::Swizzle dest_swz[4] = {
      {0, 7, 7, 7},
   {0, 1, 7, 7},
   {0, 1, 2, 7},
                        auto [offset, res_offset] = shader.evaluate_resource_offset(intr, 0);
   {
                     auto ir = new LoadFromBuffer(
         ir->set_fetch_flag(FetchInstr::use_tc);
            shader.emit_instruction(ir);
      }
      bool
   RatInstr::emit_global_store(nir_intrinsic_instr *intr, Shader& shader)
   {
      auto& vf = shader.value_factory();
   auto addr_orig = vf.src(intr->src[1], 0);
            shader.emit_instruction(
      new AluInstr(op2_lshr_int, addr_vec[0], addr_orig, vf.literal(2),
         RegisterVec4::Swizzle value_swz = {0,7,7,7};
   auto mask = nir_intrinsic_write_mask(intr);
   for (int i = 0; i < 4; ++i) {
      if (mask & (1 << i))
                        AluInstr *ir = nullptr;
   for (int i = 0; i < 4; ++i) {
      if (value_swz[i] < 4) {
      ir = new AluInstr(op1_mov, value_vec[i],
               }
   if (ir)
            auto store = new RatInstr(cf_mem_rat_cacheless,
                           RatInstr::STORE_RAW,
   value_vec,
   addr_vec,
   shader.ssbo_image_offset(),
   shader.emit_instruction(store);
      }
      bool
   RatInstr::emit_ssbo_store(nir_intrinsic_instr *instr, Shader& shader)
   {
      auto& vf = shader.value_factory();
                              shader.emit_instruction(
            for (unsigned i = 0; i < nir_src_num_components(instr->src[0]); ++i) {
      auto addr_vec = vf.temp_vec4(pin_group, {0, 1, 2, 7});
   if (i == 0) {
      shader.emit_instruction(
      } else {
      shader.emit_instruction(new AluInstr(
      }
   auto value = vf.src(instr->src[0], i);
   PRegister v = vf.temp_register(0);
   shader.emit_instruction(new AluInstr(op1_mov, v, value, AluInstr::last_write));
   auto value_vec = RegisterVec4(v, nullptr, nullptr, nullptr, pin_chan);
   auto store = new RatInstr(cf_mem_rat,
                           RatInstr::STORE_TYPED,
   value_vec,
   addr_vec,
   offset + shader.ssbo_image_offset(),
                  }
      bool
   RatInstr::emit_ssbo_atomic_op(nir_intrinsic_instr *intr, Shader& shader)
   {
      auto& vf = shader.value_factory();
   auto [imageid, image_offset] = shader.evaluate_resource_offset(intr, 0);
   {
            bool read_result = !list_is_empty(&intr->def.uses);
   auto opcode = read_result ? get_rat_opcode(nir_intrinsic_atomic_op(intr))
            auto coord_orig = vf.src(intr->src[1], 0);
                     shader.emit_instruction(
            shader.emit_instruction(
            if (intr->intrinsic == nir_intrinsic_ssbo_atomic_swap) {
      shader.emit_instruction(
         shader.emit_instruction(
      new AluInstr(op1_mov,
               } else {
      shader.emit_instruction(new AluInstr(
                        auto atomic = new RatInstr(cf_mem_rat,
                              opcode,
   data_vec4,
   out_vec,
               atomic->set_ack();
   if (read_result) {
      atomic->set_instr_flag(ack_rat_return_write);
            auto fetch = new FetchInstr(vc_fetch,
                              dest,
   {0, 1, 2, 3},
   shader.rat_return_address(),
   0,
   no_index_offset,
      fetch->set_mfc(15);
   fetch->set_fetch_flag(FetchInstr::srf_mode);
   fetch->set_fetch_flag(FetchInstr::use_tc);
   fetch->set_fetch_flag(FetchInstr::vpm);
   fetch->set_fetch_flag(FetchInstr::wait_ack);
   fetch->add_required_instr(atomic);
   shader.chain_ssbo_read(fetch);
                  }
      bool
   RatInstr::emit_ssbo_size(nir_intrinsic_instr *intr, Shader& shader)
   {
      auto& vf = shader.value_factory();
            auto const_offset = nir_src_as_const_value(intr->src[0]);
   int res_id = R600_IMAGE_REAL_RESOURCE_OFFSET;
   if (const_offset)
         else
            shader.emit_instruction(new QueryBufferSizeInstr(dest, {0, 1, 2, 3}, res_id));
      }
      bool
   RatInstr::emit_image_store(nir_intrinsic_instr *intrin, Shader& shader)
   {
      auto& vf = shader.value_factory();
   auto [imageid, image_offset] = shader.evaluate_resource_offset(intrin, 0);
   {
            auto coord_load = vf.src_vec4(intrin->src[1], pin_chan);
            auto value_load = vf.src_vec4(intrin->src[3], pin_chan);
            RegisterVec4::Swizzle swizzle = {0, 1, 2, 3};
   if (nir_intrinsic_image_dim(intrin) == GLSL_SAMPLER_DIM_1D &&
      nir_intrinsic_image_array(intrin))
         for (int i = 0; i < 4; ++i) {
      auto flags = i != 3 ? AluInstr::write : AluInstr::last_write;
   shader.emit_instruction(
      }
   for (int i = 0; i < 4; ++i) {
      auto flags = i != 3 ? AluInstr::write : AluInstr::last_write;
               auto op = cf_mem_rat; // nir_intrinsic_access(intrin) & ACCESS_COHERENT ?
         auto store = new RatInstr(
            store->set_ack();
   if (nir_intrinsic_access(intrin) & ACCESS_INCLUDE_HELPERS)
            shader.emit_instruction(store);
      }
      bool
   RatInstr::emit_image_load_or_atomic(nir_intrinsic_instr *intrin, Shader& shader)
   {
      auto& vf = shader.value_factory();
   auto [imageid, image_offset] = shader.evaluate_resource_offset(intrin, 0);
   {
            bool read_result = !list_is_empty(&intrin->def.uses);
   bool image_load = (intrin->intrinsic == nir_intrinsic_image_load);
   auto opcode = image_load  ? RatInstr::NOP_RTN :
                  auto coord_orig = vf.src_vec4(intrin->src[1], pin_chan);
                     RegisterVec4::Swizzle swizzle = {0, 1, 2, 3};
   if (nir_intrinsic_image_dim(intrin) == GLSL_SAMPLER_DIM_1D &&
      nir_intrinsic_image_array(intrin))
         for (int i = 0; i < 4; ++i) {
      auto flags = i != 3 ? AluInstr::write : AluInstr::last_write;
   shader.emit_instruction(
               shader.emit_instruction(
            if (intrin->intrinsic == nir_intrinsic_image_atomic_swap) {
      shader.emit_instruction(
         shader.emit_instruction(
      new AluInstr(op1_mov,
               } else {
      shader.emit_instruction(
         shader.emit_instruction(
               auto atomic =
                  atomic->set_ack();
   if (read_result) {
      atomic->set_instr_flag(ack_rat_return_write);
            pipe_format format = nir_intrinsic_format(intrin);
   unsigned fmt = fmt_32;
   unsigned num_format = 0;
   unsigned format_comp = 0;
   unsigned endian = 0;
            auto fetch = new FetchInstr(vc_fetch,
                              dest,
   {0, 1, 2, 3},
   shader.rat_return_address(),
   0,
   no_index_offset,
      fetch->set_mfc(3);
   fetch->set_fetch_flag(FetchInstr::srf_mode);
   fetch->set_fetch_flag(FetchInstr::use_tc);
   fetch->set_fetch_flag(FetchInstr::vpm);
   fetch->set_fetch_flag(FetchInstr::wait_ack);
   if (format_comp)
            shader.chain_ssbo_read(fetch);
                  }
      #define R600_SHADER_BUFFER_INFO_SEL (512 + R600_BUFFER_INFO_OFFSET / 16)
      bool
   RatInstr::emit_image_size(nir_intrinsic_instr *intrin, Shader& shader)
   {
                                 auto const_offset = nir_src_as_const_value(intrin->src[0]);
            int res_id = R600_IMAGE_REAL_RESOURCE_OFFSET + nir_intrinsic_range_base(intrin);
   if (const_offset)
         else
            if (nir_intrinsic_image_dim(intrin) == GLSL_SAMPLER_DIM_BUF) {
      auto dest = vf.dest_vec4(intrin->def, pin_group);
   shader.emit_instruction(new QueryBufferSizeInstr(dest, {0, 1, 2, 3}, res_id));
                  if (nir_intrinsic_image_dim(intrin) == GLSL_SAMPLER_DIM_CUBE &&
      nir_intrinsic_image_array(intrin) &&
                  auto dest = vf.dest_vec4(intrin->def, pin_group);
   shader.emit_instruction(new TexInstr(TexInstr::get_resinfo,
                                             if (const_offset) {
      unsigned lookup_resid = const_offset[0].u32 + shader.image_size_const_offset();
   shader.emit_instruction(
      new AluInstr(op1_mov,
               dest[2],
   vf.uniform(lookup_resid / 4 + R600_SHADER_BUFFER_INFO_SEL,
   } else {
      /* If the addressing is indirect we have to get the z-value by
   * using a binary search */
   auto addr = vf.temp_register();
   auto comp1 = vf.temp_register();
   auto comp2 = vf.temp_register();
                           shader.emit_instruction(new AluInstr(op2_lshr_int,
                           shader.emit_instruction(new AluInstr(op2_and_int,
                           shader.emit_instruction(new AluInstr(op2_and_int,
                              shader.emit_instruction(new LoadFromBuffer(trgt,
                                          // this may be wrong
   shader.emit_instruction(new AluInstr(
         shader.emit_instruction(new AluInstr(
         shader.emit_instruction(new AluInstr(
         } else {
      auto dest = vf.dest_vec4(intrin->def, pin_group);
   shader.emit_instruction(new TexInstr(TexInstr::get_resinfo,
                                 }
      }
      bool
   RatInstr::emit_image_samples(nir_intrinsic_instr *intrin, Shader& shader)
   {
                        auto tmp =  shader.value_factory().temp_vec4(pin_group);
            auto const_offset = nir_src_as_const_value(intrin->src[0]);
            int res_id = R600_IMAGE_REAL_RESOURCE_OFFSET + nir_intrinsic_range_base(intrin);
   if (const_offset)
         else
            shader.emit_instruction(new TexInstr(TexInstr::get_resinfo,
                                    shader.emit_instruction(new AluInstr(op1_mov, dest, tmp[0], AluInstr::last_write));
      }
      } // namespace r600
