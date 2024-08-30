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
      #include "sfn_instr_tex.h"
      #include "nir_builder.h"
   #include "sfn_debug.h"
   #include "sfn_instr_alu.h"
   #include "sfn_instr_fetch.h"
   #include "sfn_nir.h"
      namespace r600 {
      using std::string;
      TexInstr::TexInstr(Opcode op,
                     const RegisterVec4& dest,
   const RegisterVec4::Swizzle& dest_swizzle,
   const RegisterVec4& src,
      InstrWithVectorResult(dest, dest_swizzle, resource_id, resource_offs),
   m_opcode(op),
   m_src(src),
   m_inst_mode(0),
      {
      memset(m_coord_offset, 0, sizeof(m_coord_offset));
      }
      void
   TexInstr::accept(ConstInstrVisitor& visitor) const
   {
         }
      void
   TexInstr::accept(InstrVisitor& visitor)
   {
         }
      void
   TexInstr::set_offset(unsigned index, int32_t val)
   {
      assert(index < 3);
      }
      int
   TexInstr::get_offset(unsigned index) const
   {
      assert(index < 3);
      }
      void
   TexInstr::set_gather_comp(int cmp)
   {
         }
      bool
   TexInstr::is_equal_to(const TexInstr& lhs) const
   {
      if (m_opcode != lhs.m_opcode)
            if (!comp_dest(lhs.dst(), lhs.all_dest_swizzle()))
            if (m_src != lhs.m_src)
            if (resource_offset() && lhs.resource_offset()) {
      if (!resource_offset()->equal_to(*lhs.resource_offset()))
      } else if ((resource_offset() && !lhs.resource_offset()) ||
                  if (sampler_offset() && lhs.sampler_offset()) {
      if (!sampler_offset()->equal_to(*lhs.sampler_offset()))
      } else if ((sampler_offset() && !lhs.sampler_offset()) ||
                  if (m_tex_flags != lhs.m_tex_flags)
            for (int i = 0; i < 3; ++i) {
      if (m_coord_offset[i] != lhs.m_coord_offset[i])
               return m_inst_mode == lhs.m_inst_mode &&
         resource_id() == lhs.resource_id() &&
   resource_index_mode() == lhs.resource_index_mode() &&
      }
      bool
   TexInstr::propagate_death()
   {
      m_src.del_use(this);
      }
      void TexInstr::forward_set_blockid(int id, int index)
   {
      for (auto p : m_prepare_instr)
      }
      bool
   TexInstr::do_ready() const
   {
      for (auto p : m_prepare_instr)
      if (!p->ready())
         for (auto p : required_instr())
      if (!p->is_scheduled() && !p->is_dead()) {
               if (resource_offset() && !resource_offset()->ready(block_id(), index()))
            }
      void
   TexInstr::do_print(std::ostream& os) const
   {
         for (auto& p : prepare_instr()) {
                  os << "TEX " << opname(m_opcode) << " ";
            os << " : ";
            os << " RID:" << resource_id();
   if (resource_offset())
            os << " SID:" << sampler_id();
   if (sampler_offset())
            if (m_coord_offset[0])
         if (m_coord_offset[1])
         if (m_coord_offset[2])
            if (m_inst_mode || is_gather(m_opcode))
            os << " ";
   os << (m_tex_flags.test(x_unnormalized) ? "U" : "N");
   os << (m_tex_flags.test(y_unnormalized) ? "U" : "N");
   os << (m_tex_flags.test(z_unnormalized) ? "U" : "N");
      }
      const char *
   TexInstr::opname(Opcode op)
   {
      switch (op) {
   case ld:
         case get_resinfo:
         case get_nsamples:
         case get_tex_lod:
         case get_gradient_h:
         case get_gradient_v:
         case set_offsets:
         case keep_gradients:
         case set_gradient_h:
         case set_gradient_v:
         case sample:
         case sample_l:
         case sample_lb:
         case sample_lz:
         case sample_g:
         case sample_g_lb:
         case gather4:
         case gather4_o:
         case sample_c:
         case sample_c_l:
         case sample_c_lb:
         case sample_c_lz:
         case sample_c_g:
         case sample_c_g_lb:
         case gather4_c:
         case gather4_c_o:
         default:
            }
      const std::map<TexInstr::Opcode, std::string> TexInstr::s_opcode_map = {
      {ld,             "LD"                   },
   {get_resinfo,    "GET_TEXTURE_RESINFO"  },
   {get_nsamples,   "GET_NUMBER_OF_SAMPLES"},
   {get_tex_lod,    "GET_LOD"              },
   {get_gradient_h, "GET_GRADIENTS_H"      },
   {get_gradient_v, "GET_GRADIENTS_V"      },
   {set_offsets,    "SET_TEXTURE_OFFSETS"  },
   {keep_gradients, "KEEP_GRADIENTS"       },
   {set_gradient_h, "SET_GRADIENTS_H"      },
   {set_gradient_v, "SET_GRADIENTS_V"      },
   {sample,         "SAMPLE"               },
   {sample_l,       "SAMPLE_L"             },
   {sample_lb,      "SAMPLE_LB"            },
   {sample_lz,      "SAMPLE_LZ"            },
   {sample_g,       "SAMPLE_G"             },
   {sample_g_lb,    "SAMPLE_G_L"           },
   {gather4,        "GATHER4"              },
   {gather4_o,      "GATHER4_O"            },
   {sample_c,       "SAMPLE_C"             },
   {sample_c_l,     "SAMPLE_C_L"           },
   {sample_c_lb,    "SAMPLE_C_LB"          },
   {sample_c_lz,    "SAMPLE_C_LZ"          },
   {sample_c_g,     "SAMPLE_C_G"           },
   {sample_c_g_lb,  "SAMPLE_C_G_L"         },
   {gather4_c,      "GATHER4_C"            },
   {gather4_c_o,    "OP_GATHER4_C_O"       },
      };
      bool
   TexInstr::is_gather(Opcode op)
   {
         }
      TexInstr::Opcode
   TexInstr::op_from_string(const std::string& s)
   {
      for (auto& [op, str] : s_opcode_map) {
      if (s == str)
      }
      }
      Instr::Pointer
   TexInstr::from_string(std::istream& is, ValueFactory& value_fctory)
   {
      string opstr;
   string deststr;
                                       char dummy;
   is >> dummy;
            string srcstr;
                     string res_id_str;
                     int res_id = int_from_string_with_prefix(res_id_str, "RID:");
            auto tex = new TexInstr(opcode, dest, dest_swz, src, res_id, nullptr,
            while (!is.eof() && is.good()) {
      std::string next_token;
            if (next_token.empty())
            if (next_token[0] == 'U' || next_token[0] == 'N') {
         } else {
                        }
      void
   TexInstr::read_tex_coord_normalitazion(const std::string& flags)
   {
      assert(flags.length() == 4);
   if (flags[0] == 'U')
         if (flags[1] == 'U')
         if (flags[2] == 'U')
         if (flags[3] == 'U')
      }
      void
   TexInstr::set_tex_param(const std::string& token)
   {
      if (token.substr(0, 3) == "OX:")
         else if (token.substr(0, 3) == "OY:")
         else if (token.substr(0, 3) == "OZ:")
         else if (token.substr(0, 5) == "MODE:")
         else if (token.substr(0, 3) == "SO:")
         else if (token.substr(0, 3) == "RO:")
         else {
      std::cerr << "Token '" << token << "': ";
         }
      bool
   TexInstr::from_nir(nir_tex_instr *tex, Shader& shader)
   {
               if (nir_tex_instr_src_index(tex, nir_tex_src_backend1) != -1)
            if (tex->sampler_dim == GLSL_SAMPLER_DIM_BUF) {
      switch (tex->op) {
   case nir_texop_txs:
         case nir_texop_txf:
         default:
            } else {
      switch (tex->op) {
   case nir_texop_txs:
         case nir_texop_lod:
         case nir_texop_query_levels:
         case nir_texop_texture_samples:
         default:
            }
      }
      bool
   TexInstr::replace_source(PRegister old_src, PVirtualValue new_src)
   {
      if (old_src->pin() != pin_free)
            if (!new_src->as_register())
            bool success = false;
   for (int i = 0; i < 4; ++i) {
      if (m_src[i]->equal_to(*old_src)) {
      m_src.set_value(i, new_src->as_register());
         }
   m_src.validate();
   if (success) {
      old_src->del_use(this);
      }
      }
      void TexInstr::update_indirect_addr(PRegister old_reg, PRegister addr)
   {
      if (resource_offset() && old_reg->equal_to(*resource_offset()))
         else if (sampler_offset() && old_reg->equal_to(*sampler_offset()))
            for (auto& p : m_prepare_instr)
      }
      uint8_t
   TexInstr::allowed_src_chan_mask() const
   {
         }
      struct SamplerId {
      int id;
      };
      SamplerId
   get_sampler_id(int sampler_id, const nir_variable *deref)
   {
               if (deref) {
      assert(glsl_type_is_sampler(deref->type));
      }
      }
      void
   TexInstr::emit_set_gradients(
         {
      TexInstr *grad[2] = {nullptr, nullptr};
   RegisterVec4 empty_dst(0, false, {0, 0, 0, 0}, pin_group);
   grad[0] = new TexInstr(set_gradient_h,
                        empty_dst,
      grad[0]->set_rect_coordinate_flags(tex);
            grad[1] = new TexInstr(set_gradient_v,
                        empty_dst,
      grad[1]->set_rect_coordinate_flags(tex);
   grad[1]->set_always_keep();
   irt->add_prepare_instr(grad[0]);
   irt->add_prepare_instr(grad[1]);
   if (shader.last_txd())
            }
      void
   TexInstr::emit_set_offsets(nir_tex_instr *tex, int texture_id, Inputs& src, TexInstr *irt, Shader& shader)
   {
      RegisterVec4::Swizzle swizzle = {4, 4, 4, 4};
   int src_components = tex->coord_components;
   if (tex->is_array)
            for (int i = 0; i < src_components; ++i)
            auto ofs = shader.value_factory().src_vec4(*src.offset, pin_group, swizzle);
            auto set_ofs = new TexInstr(TexInstr::set_offsets,
                                 set_ofs->set_always_keep();
      }
      bool
   TexInstr::emit_lowered_tex(nir_tex_instr *tex, Inputs& src, Shader& shader)
   {
      assert(src.backend1);
            auto& vf = shader.value_factory();
   sfn_log << SfnLog::instr << "emit '" << *reinterpret_cast<nir_instr *>(tex) << "' ("
            auto params = nir_src_as_const_value(*src.backend2);
   int32_t coord_mask = params[0].i32;
   int32_t flags = params[1].i32;
   int32_t inst_mode = params[2].i32;
                     RegisterVec4::Swizzle src_swizzle = {0};
   for (int i = 0; i < 4; ++i)
                     RegisterVec4::Swizzle dst_swz = {0, 1, 2, 3};
   if (dst_swz_packed) {
      for (int i = 0; i < 4; ++i) {
                     int texture_id = tex->texture_index + R600_MAX_CONST_BUFFERS;
   auto irt = new TexInstr(src.opcode,
                           dst,
   dst_swz,
            if (tex->op == nir_texop_txd)
            if (!irt->set_coord_offsets(src.offset)) {
      assert(tex->op == nir_texop_tg4);
               for (const auto f : TexFlags) {
      if (flags & (1 << f))
                        shader.emit_instruction(irt);
      }
      bool
   TexInstr::emit_buf_txf(nir_tex_instr *tex, Inputs& src, Shader& shader)
   {
      auto& vf = shader.value_factory();
            PRegister tex_offset = nullptr;
   if (src.sampler_offset)
            auto *real_dst = &dst;
            if (shader.chip_class() < ISA_CC_EVERGREEN) {
                  auto ir = new LoadFromBuffer(*real_dst,
                              {0, 1, 2, 3},
      ir->set_fetch_flag(FetchInstr::use_const_field);
   shader.emit_instruction(ir);
            if (shader.chip_class() < ISA_CC_EVERGREEN) {
      auto tmp_w = vf.temp_register();
   int buf_sel = (512 + R600_BUFFER_INFO_OFFSET / 16) + 2 * tex->texture_index;
   AluInstr *ir = nullptr;
   for (int i = 0; i < 4; ++i) {
      auto d = i < 3 ? dst[i] : tmp_w;
   ir = new AluInstr(op2_and_int,
                     d,
               ir->set_alu_flag(alu_last_instr);
   shader.emit_instruction(
      new AluInstr(op2_or_int,
               dst[3],
               }
      bool
   TexInstr::emit_tex_texture_samples(nir_tex_instr *instr, Inputs& src, Shader& shader)
   {
      RegisterVec4 dest = shader.value_factory().dest_vec4(instr->def, pin_chan);
   RegisterVec4 help{
                           // Fishy: should the zero be instr->sampler_index?
   auto ir =
         shader.emit_instruction(ir);
      }
      bool
   TexInstr::emit_tex_txs(nir_tex_instr *tex,
                     {
                        if (tex->sampler_dim == GLSL_SAMPLER_DIM_BUF) {
      if (shader.chip_class() >= ISA_CC_EVERGREEN) {
      shader.emit_instruction(new QueryBufferSizeInstr(
      } else {
      int id = 2 * tex->sampler_index + (512 + R600_BUFFER_INFO_OFFSET / 16) + 1;
   auto src = vf.uniform(id, 1, R600_BUFFER_INFO_CONST_BUFFER);
   shader.emit_instruction(
                           auto src_lod = vf.temp_register();
   shader.emit_instruction(
                     if (tex->is_array && tex->sampler_dim == GLSL_SAMPLER_DIM_CUBE)
            auto ir = new TexInstr(get_resinfo,
                                    ir->set_dest_swizzle(dest_swz);
            if (tex->is_array && tex->sampler_dim == GLSL_SAMPLER_DIM_CUBE) {
      auto src_loc = vf.uniform(512 + R600_BUFFER_INFO_OFFSET / 16 + (tex->texture_index >> 2),
                  auto alu = new AluInstr(op1_mov, dest[2], src_loc, AluInstr::last_write);
   shader.emit_instruction(alu);
                     }
      auto
   TexInstr::prepare_source(nir_tex_instr *tex, const Inputs& inputs, Shader& shader)
         {
      RegisterVec4::Swizzle target{7, 7, 7, 7};
            for (unsigned i = 0; i < tex->coord_components; ++i) {
      target[i] = i;
               // array index always goes into z
   if (tex->is_array && tex->sampler_dim == GLSL_SAMPLER_DIM_1D) {
      target[2] = 1;
   target[1] = 7;
               /* With txl and txb shadow goes into z and lod or bias go into w */
   if (tex->op == nir_texop_txl || tex->op == nir_texop_txb) {
      target[3] = 3;
   src[3] = tex->op == nir_texop_txl ? inputs.lod : inputs.bias;
   if (tex->is_shadow) {
      target[2] = 2;
         } else if (tex->is_shadow) {
      /* Other ops have shadow in w */
   target[3] = 3;
                        AluInstr *ir = nullptr;
   for (int i = 0; i < 4; ++i) {
      if (target[i] > 3)
                     ir = new AluInstr(op, src_coord[i], src[i], AluInstr::write);
               if (ir)
               }
      TexInstr::Inputs::Inputs(const nir_tex_instr& instr, ValueFactory& vf):
      sampler_deref(nullptr),
   texture_deref(nullptr),
   bias(nullptr),
   comperator(nullptr),
   lod(nullptr),
   offset(nullptr),
   gather_comp(nullptr),
   ms_index(nullptr),
   texture_offset(nullptr),
   sampler_offset(nullptr),
   backend1(nullptr),
   backend2(nullptr),
      {
      // sfn_log << SfnLog::tex << "Get Inputs with " << instr.coord_components
            unsigned grad_components = instr.coord_components;
   if (instr.is_array && !instr.array_is_lowered_cube)
            for (unsigned i = 0; i < instr.num_srcs; ++i) {
      switch (instr.src[i].src_type) {
   case nir_tex_src_bias:
                  case nir_tex_src_coord: {
      coord = vf.src_vec4(instr.src[i].src,
            } break;
   case nir_tex_src_comparator:
      comperator = vf.src(instr.src[i], 0);
      case nir_tex_src_ddx:
      ddx = vf.src_vec4(instr.src[i].src,
                  case nir_tex_src_ddy:
      ddy = vf.src_vec4(instr.src[i].src,
                  case nir_tex_src_lod:
      lod = vf.src(instr.src[i].src, 0);
      case nir_tex_src_offset:
      offset = &instr.src[i].src;
   break;
   /* case nir_tex_src_sampler_deref:
   sampler_deref = get_deref_location(instr.src[i].src);
      case nir_tex_src_texture_deref:
      texture_deref = get_deref_location(instr.src[i].src);
      */
   case nir_tex_src_ms_index:
      ms_index = vf.src(instr.src[i], 0);
      case nir_tex_src_texture_offset:
      texture_offset = vf.src(instr.src[i], 0)->as_register();
      case nir_tex_src_sampler_offset:
      sampler_offset = vf.src(instr.src[i], 0)->as_register();
      case nir_tex_src_backend1:
      backend1 = &instr.src[i].src;
      case nir_tex_src_backend2:
      backend2 = &instr.src[i].src;
      case nir_tex_src_plane:
   case nir_tex_src_projector:
   case nir_tex_src_min_lod:
   default:
                        }
      auto
   TexInstr::Inputs::get_opcode(const nir_tex_instr& instr) -> Opcode
   {
      switch (instr.op) {
   case nir_texop_tex:
         case nir_texop_txf:
         case nir_texop_txb:
         case nir_texop_txl:
         case nir_texop_txs:
         case nir_texop_lod:
         case nir_texop_txd:
         case nir_texop_tg4: {
      auto var_offset = offset && nir_src_as_const_value(*offset) == nullptr;
   return instr.is_shadow ? (var_offset ? gather4_c_o : gather4_c)
      }
   case nir_texop_txf_ms:
         case nir_texop_query_levels:
         case nir_texop_texture_samples:
         default:
            }
      bool
   TexInstr::emit_tex_lod(nir_tex_instr *tex, Inputs& src, Shader& shader)
   {
      auto& vf = shader.value_factory();
   auto sampler = get_sampler_id(tex->sampler_index, src.sampler_deref);
                                       AluInstr *ir = nullptr;
   for (unsigned i = 0; i < tex->coord_components; ++i) {
      ir = new AluInstr(op1_mov, src_coord[i], src.coord[i], AluInstr::write);
      }
   if (ir)
            auto irt = new TexInstr(TexInstr::get_tex_lod,
                                    shader.emit_instruction(irt);
      }
      RegisterVec4::Swizzle
   TexInstr::Inputs::swizzle_from_ncomps(int comps) const
   {
      RegisterVec4::Swizzle swz;
   for (int i = 0; i < 4; ++i)
            }
      bool
   TexInstr::set_coord_offsets(nir_src *offset)
   {
      if (!offset)
            auto literal = nir_src_as_const_value(*offset);
   if (!literal)
            for (int i = 0; i < offset->ssa->num_components; ++i)
            }
      void
   TexInstr::set_rect_coordinate_flags(nir_tex_instr *instr)
   {
      if (instr->sampler_dim == GLSL_SAMPLER_DIM_RECT) {
      set_tex_flag(x_unnormalized);
         }
      class LowerTexToBackend : public NirLowerInstruction {
   public:
            private:
      bool filter(const nir_instr *instr) const override;
            nir_def *lower_tex(nir_tex_instr *tex);
   nir_def *lower_txf(nir_tex_instr *tex);
   nir_def *lower_tg4(nir_tex_instr *tex);
   nir_def *lower_txf_ms(nir_tex_instr *tex);
            nir_def *
   prepare_coord(nir_tex_instr *tex, int& unnormalized_mask, int& used_coord_mask);
   int get_src_coords(nir_tex_instr *tex,
               nir_def *prep_src(std::array<nir_def *, 4>& coord, int& used_coord_mask);
   nir_def *
                     amd_gfx_level m_chip_class;
      };
      bool
   r600_nir_lower_tex_to_backend(nir_shader *shader, amd_gfx_level chip_class)
   {
         }
      LowerTexToBackend::LowerTexToBackend(amd_gfx_level chip_class):
         {
   }
      bool
   LowerTexToBackend::filter(const nir_instr *instr) const
   {
      if (instr->type != nir_instr_type_tex)
            auto tex = nir_instr_as_tex(instr);
   if (tex->sampler_dim == GLSL_SAMPLER_DIM_BUF)
         switch (tex->op) {
   case nir_texop_tex:
   case nir_texop_txb:
   case nir_texop_txl:
   case nir_texop_txf:
   case nir_texop_txd:
   case nir_texop_tg4:
   case nir_texop_txf_ms:
         default:
                     }
      nir_def *LowerTexToBackend::get_undef()
   {
      if (!m_undef)
            }
      nir_def *
   LowerTexToBackend::lower(nir_instr *instr)
   {
               auto tex = nir_instr_as_tex(instr);
   switch (tex->op) {
   case nir_texop_tex:
   case nir_texop_txb:
   case nir_texop_txl:
   case nir_texop_txd:
         case nir_texop_txf:
         case nir_texop_tg4:
         case nir_texop_txf_ms:
      if (m_chip_class < EVERGREEN)
         else
      default:
            }
      nir_def *
   LowerTexToBackend::lower_tex(nir_tex_instr *tex)
   {
      int unnormalized_mask = 0;
                                 }
      nir_def *
   LowerTexToBackend::lower_txf(nir_tex_instr *tex)
   {
                        int lod_idx = nir_tex_instr_src_index(tex, nir_tex_src_lod);
            int used_coord_mask = 0;
   nir_def *backend1 = prep_src(new_coord, used_coord_mask);
   nir_def *backend2 =
               }
      nir_def *
   LowerTexToBackend::lower_tg4(nir_tex_instr *tex)
   {
               get_src_coords(tex, new_coord, false);
   uint32_t dest_swizzle =
            int used_coord_mask = 0;
   int unnormalized_mask = 0;
            nir_def *backend2 =
            }
      nir_def *
   LowerTexToBackend::lower_txf_ms(nir_tex_instr *tex)
   {
                        int ms_index = nir_tex_instr_src_index(tex, nir_tex_src_ms_index);
            int offset_index = nir_tex_instr_src_index(tex, nir_tex_src_offset);
   if (offset_index >= 0) {
      auto offset = tex->src[offset_index].src.ssa;
   for (int i = 0; i < offset->num_components; ++i) {
                     auto fetch_sample = nir_instr_as_tex(nir_instr_clone(b->shader, &tex->instr));
            int used_coord_mask = 0;
   nir_def *backend1 = prep_src(new_coord, used_coord_mask);
            nir_builder_instr_insert(b, &fetch_sample->instr);
            new_coord[3] = nir_iand_imm(b,
                              nir_def *backend1b = prep_src(new_coord, used_coord_mask);
   nir_def *backend2b = nir_imm_ivec4(b, used_coord_mask, 0, 0, 0);
      }
      nir_def *
   LowerTexToBackend::lower_txf_ms_direct(nir_tex_instr *tex)
   {
                        int ms_index = nir_tex_instr_src_index(tex, nir_tex_src_ms_index);
            int used_coord_mask = 0;
   nir_def *backend1 = prep_src(new_coord, used_coord_mask);
               }
      nir_def *
   LowerTexToBackend::finalize(nir_tex_instr *tex,
               {
      nir_tex_instr_add_src(tex, nir_tex_src_backend1, backend1);
            static const nir_tex_src_type cleanup[] = {nir_tex_src_coord,
                              for (const auto type : cleanup) {
      int pos = nir_tex_instr_src_index(tex, type);
   if (pos >= 0)
      }
      }
      nir_def *
   LowerTexToBackend::prep_src(std::array<nir_def *, 4>& coord, int& used_coord_mask)
   {
      int max_coord = 0;
   for (int i = 0; i < 4; ++i) {
      if (coord[i]) {
      used_coord_mask |= 1 << i;
      } else
                  }
      nir_def *
   LowerTexToBackend::prepare_coord(nir_tex_instr *tex,
               {
               unnormalized_mask = get_src_coords(tex, new_coord, true);
            int comp_idx =
            if (tex->op == nir_texop_txl || tex->op == nir_texop_txb) {
      int idx = tex->op == nir_texop_txl ? nir_tex_instr_src_index(tex, nir_tex_src_lod)
         assert(idx != -1);
            if (comp_idx >= 0)
      } else if (comp_idx >= 0) {
         }
      }
      int
   LowerTexToBackend::get_src_coords(nir_tex_instr *tex,
               {
      int unnormalized_mask = 0;
   auto coord_idx = nir_tex_instr_src_index(tex, nir_tex_src_coord);
   assert(coord_idx != -1);
                     if (tex->coord_components > 1) {
      if (tex->is_array && tex->sampler_dim == GLSL_SAMPLER_DIM_1D)
         else
               if (tex->coord_components > 2) {
         }
   if (tex->is_array) {
      unnormalized_mask |= 0x4;
   if (round_array_index)
               if (tex->sampler_dim == GLSL_SAMPLER_DIM_RECT) {
                     }
      } // namespace r600
