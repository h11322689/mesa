   /*
   * Copyright © 2018 Valve Corporation
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
   *
   */
      #include "aco_builder.h"
   #include "aco_ir.h"
      #include "util/half_float.h"
   #include "util/memstream.h"
      #include <algorithm>
   #include <array>
   #include <vector>
      namespace aco {
      #ifndef NDEBUG
   void
   perfwarn(Program* program, bool cond, const char* msg, Instruction* instr)
   {
      if (cond) {
      char* out;
   size_t outsize;
   struct u_memstream mem;
   u_memstream_open(&mem, &out, &outsize);
            fprintf(memf, "%s: ", msg);
   aco_print_instr(program->gfx_level, instr, memf);
            aco_perfwarn(program, out);
            if (debug_flags & DEBUG_PERFWARN)
         }
   #endif
      /**
   * The optimizer works in 4 phases:
   * (1) The first pass collects information for each ssa-def,
   *     propagates reg->reg operands of the same type, inline constants
   *     and neg/abs input modifiers.
   * (2) The second pass combines instructions like mad, omod, clamp and
   *     propagates sgpr's on VALU instructions.
   *     This pass depends on information collected in the first pass.
   * (3) The third pass goes backwards, and selects instructions,
   *     i.e. decides if a mad instruction is profitable and eliminates dead code.
   * (4) The fourth pass cleans up the sequence: literals get applied and dead
   *     instructions are removed from the sequence.
   */
      struct mad_info {
      aco_ptr<Instruction> add_instr;
   uint32_t mul_temp_id;
   uint16_t literal_mask;
            mad_info(aco_ptr<Instruction> instr, uint32_t id)
            };
      enum Label {
      label_vec = 1 << 0,
   label_constant_32bit = 1 << 1,
   /* label_{abs,neg,mul,omod2,omod4,omod5,clamp} are used for both 16 and
   * 32-bit operations but this shouldn't cause any issues because we don't
   * look through any conversions */
   label_abs = 1 << 2,
   label_neg = 1 << 3,
   label_mul = 1 << 4,
   label_temp = 1 << 5,
   label_literal = 1 << 6,
   label_mad = 1 << 7,
   label_omod2 = 1 << 8,
   label_omod4 = 1 << 9,
   label_omod5 = 1 << 10,
   label_clamp = 1 << 12,
   label_undefined = 1 << 14,
   label_vcc = 1 << 15,
   label_b2f = 1 << 16,
   label_add_sub = 1 << 17,
   label_bitwise = 1 << 18,
   label_minmax = 1 << 19,
   label_vopc = 1 << 20,
   label_uniform_bool = 1 << 21,
   label_constant_64bit = 1 << 22,
   label_uniform_bitwise = 1 << 23,
   label_scc_invert = 1 << 24,
   label_scc_needed = 1 << 26,
   label_b2i = 1 << 27,
   label_fcanonicalize = 1 << 28,
   label_constant_16bit = 1 << 29,
   label_usedef = 1 << 30,   /* generic label */
   label_vop3p = 1ull << 31, /* 1ull to prevent sign extension */
   label_canonicalized = 1ull << 32,
   label_extract = 1ull << 33,
   label_insert = 1ull << 34,
   label_dpp16 = 1ull << 35,
   label_dpp8 = 1ull << 36,
   label_f2f32 = 1ull << 37,
   label_f2f16 = 1ull << 38,
   label_split = 1ull << 39,
      };
      static constexpr uint64_t instr_usedef_labels =
      label_vec | label_mul | label_add_sub | label_vop3p | label_bitwise | label_uniform_bitwise |
   label_minmax | label_vopc | label_usedef | label_extract | label_dpp16 | label_dpp8 |
      static constexpr uint64_t instr_mod_labels =
            static constexpr uint64_t instr_labels = instr_usedef_labels | instr_mod_labels | label_split;
   static constexpr uint64_t temp_labels = label_abs | label_neg | label_temp | label_vcc | label_b2f |
               static constexpr uint32_t val_labels =
            static_assert((instr_labels & temp_labels) == 0, "labels cannot intersect");
   static_assert((instr_labels & val_labels) == 0, "labels cannot intersect");
   static_assert((temp_labels & val_labels) == 0, "labels cannot intersect");
      struct ssa_info {
      uint64_t label;
   union {
      uint32_t val;
   Temp temp;
                        void add_label(Label new_label)
   {
      /* Since all the instr_usedef_labels use instr for the same thing
   * (indicating the defining instruction), there is usually no need to
   * clear any other instr labels. */
   if (new_label & instr_usedef_labels)
            if (new_label & instr_mod_labels) {
      label &= ~instr_labels;
               if (new_label & temp_labels) {
      label &= ~temp_labels;
               uint32_t const_labels =
         if (new_label & const_labels) {
      label &= ~val_labels | const_labels;
      } else if (new_label & val_labels) {
      label &= ~val_labels;
                           void set_vec(Instruction* vec)
   {
      add_label(label_vec);
                        void set_constant(amd_gfx_level gfx_level, uint64_t constant)
   {
      Operand op16 = Operand::c16(constant);
   Operand op32 = Operand::get_const(gfx_level, constant, 4);
   add_label(label_literal);
            /* check that no upper bits are lost in case of packed 16bit constants */
   if (gfx_level >= GFX8 && !op16.isLiteral() &&
                  if (!op32.isLiteral())
            if (Operand::is_constant_representable(constant, 8))
            if (label & label_constant_64bit) {
      val = Operand::c64(constant).constantValue();
   if (val != constant)
                  bool is_constant(unsigned bits)
   {
      switch (bits) {
   case 8: return label & label_literal;
   case 16: return label & label_constant_16bit;
   case 32: return label & label_constant_32bit;
   case 64: return label & label_constant_64bit;
   }
               bool is_literal(unsigned bits)
   {
      bool is_lit = label & label_literal;
   switch (bits) {
   case 8: return false;
   case 16: return is_lit && ~(label & label_constant_16bit);
   case 32: return is_lit && ~(label & label_constant_32bit);
   case 64: return false;
   }
               bool is_constant_or_literal(unsigned bits)
   {
      if (bits == 64)
         else
               void set_abs(Temp abs_temp)
   {
      add_label(label_abs);
                        void set_neg(Temp neg_temp)
   {
      add_label(label_neg);
                        void set_neg_abs(Temp neg_abs_temp)
   {
      add_label((Label)((uint32_t)label_abs | (uint32_t)label_neg));
               void set_mul(Instruction* mul)
   {
      add_label(label_mul);
                        void set_temp(Temp tmp)
   {
      add_label(label_temp);
                        void set_mad(uint32_t mad_info_idx)
   {
      add_label(label_mad);
                        void set_omod2(Instruction* mul)
   {
      if (label & temp_labels)
         add_label(label_omod2);
                        void set_omod4(Instruction* mul)
   {
      if (label & temp_labels)
         add_label(label_omod4);
                        void set_omod5(Instruction* mul)
   {
      if (label & temp_labels)
         add_label(label_omod5);
                        void set_clamp(Instruction* med3)
   {
      if (label & temp_labels)
         add_label(label_clamp);
                        void set_f2f16(Instruction* conv)
   {
      if (label & temp_labels)
         add_label(label_f2f16);
                                          void set_vcc(Temp vcc_val)
   {
      add_label(label_vcc);
                        void set_b2f(Temp b2f_val)
   {
      add_label(label_b2f);
                        void set_add_sub(Instruction* add_sub_instr)
   {
      add_label(label_add_sub);
                        void set_bitwise(Instruction* bitwise_instr)
   {
      add_label(label_bitwise);
                                          void set_minmax(Instruction* minmax_instr)
   {
      add_label(label_minmax);
                        void set_vopc(Instruction* vopc_instr)
   {
      add_label(label_vopc);
                                          void set_scc_invert(Temp scc_inv)
   {
      add_label(label_scc_invert);
                        void set_uniform_bool(Temp uniform_bool)
   {
      add_label(label_uniform_bool);
                        void set_b2i(Temp b2i_val)
   {
      add_label(label_b2i);
                        void set_usedef(Instruction* label_instr)
   {
      add_label(label_usedef);
                        void set_vop3p(Instruction* vop3p_instr)
   {
      add_label(label_vop3p);
                        void set_fcanonicalize(Temp tmp)
   {
      add_label(label_fcanonicalize);
                                          void set_f2f32(Instruction* cvt)
   {
      add_label(label_f2f32);
                        void set_extract(Instruction* extract)
   {
      add_label(label_extract);
                        void set_insert(Instruction* insert)
   {
      if (label & temp_labels)
         add_label(label_insert);
                        void set_dpp16(Instruction* mov)
   {
      add_label(label_dpp16);
               void set_dpp8(Instruction* mov)
   {
      add_label(label_dpp8);
               bool is_dpp() { return label & (label_dpp16 | label_dpp8); }
   bool is_dpp16() { return label & label_dpp16; }
            void set_split(Instruction* split)
   {
      add_label(label_split);
                        void set_subgroup_invocation(Instruction* label_instr)
   {
      add_label(label_subgroup_invocation);
                  };
      struct opt_ctx {
      Program* program;
   float_mode fp_mode;
   std::vector<aco_ptr<Instruction>> instructions;
   ssa_info* info;
   std::pair<uint32_t, Temp> last_literal;
   std::vector<mad_info> mad_infos;
      };
      bool
   can_use_VOP3(opt_ctx& ctx, const aco_ptr<Instruction>& instr)
   {
      if (instr->isVOP3())
            if (instr->isVOP3P())
            if (instr->operands.size() && instr->operands[0].isLiteral() && ctx.program->gfx_level < GFX10)
            if (instr->isSDWA())
            if (instr->isDPP() && ctx.program->gfx_level < GFX11)
            return instr->opcode != aco_opcode::v_madmk_f32 && instr->opcode != aco_opcode::v_madak_f32 &&
         instr->opcode != aco_opcode::v_madmk_f16 && instr->opcode != aco_opcode::v_madak_f16 &&
   instr->opcode != aco_opcode::v_fmamk_f32 && instr->opcode != aco_opcode::v_fmaak_f32 &&
   instr->opcode != aco_opcode::v_fmamk_f16 && instr->opcode != aco_opcode::v_fmaak_f16 &&
   instr->opcode != aco_opcode::v_readlane_b32 &&
      }
      bool
   pseudo_propagate_temp(opt_ctx& ctx, aco_ptr<Instruction>& instr, Temp temp, unsigned index)
   {
      if (instr->definitions.empty())
            const bool vgpr =
      instr->opcode == aco_opcode::p_as_uniform ||
   std::all_of(instr->definitions.begin(), instr->definitions.end(),
         /* don't propagate VGPRs into SGPR instructions */
   if (temp.type() == RegType::vgpr && !vgpr)
            bool can_accept_sgpr =
      ctx.program->gfx_level >= GFX9 ||
   std::none_of(instr->definitions.begin(), instr->definitions.end(),
         switch (instr->opcode) {
   case aco_opcode::p_phi:
   case aco_opcode::p_linear_phi:
   case aco_opcode::p_parallelcopy:
   case aco_opcode::p_create_vector:
      if (temp.bytes() != instr->operands[index].bytes())
            case aco_opcode::p_extract_vector:
   case aco_opcode::p_extract:
      if (temp.type() == RegType::sgpr && !can_accept_sgpr)
            case aco_opcode::p_split_vector: {
      if (temp.type() == RegType::sgpr && !can_accept_sgpr)
         /* don't increase the vector size */
   if (temp.bytes() > instr->operands[index].bytes())
         /* We can decrease the vector size as smaller temporaries are only
   * propagated by p_as_uniform instructions.
   * If this propagation leads to invalid IR or hits the assertion below,
   * it means that some undefined bytes within a dword are begin accessed
   * and a bug in instruction_selection is likely. */
   int decrease = instr->operands[index].bytes() - temp.bytes();
   while (decrease > 0) {
      decrease -= instr->definitions.back().bytes();
      }
   assert(decrease == 0);
      }
   case aco_opcode::p_as_uniform:
      if (temp.regClass() == instr->definitions[0].regClass())
            default: return false;
            instr->operands[index].setTemp(temp);
      }
      /* This expects the DPP modifier to be removed. */
   bool
   can_apply_sgprs(opt_ctx& ctx, aco_ptr<Instruction>& instr)
   {
      assert(instr->isVALU());
   if (instr->isSDWA() && ctx.program->gfx_level < GFX9)
         return instr->opcode != aco_opcode::v_readfirstlane_b32 &&
         instr->opcode != aco_opcode::v_readlane_b32 &&
   instr->opcode != aco_opcode::v_readlane_b32_e64 &&
   instr->opcode != aco_opcode::v_writelane_b32 &&
   instr->opcode != aco_opcode::v_writelane_b32_e64 &&
   instr->opcode != aco_opcode::v_permlane16_b32 &&
   instr->opcode != aco_opcode::v_permlanex16_b32 &&
   instr->opcode != aco_opcode::v_interp_p1_f32 &&
   instr->opcode != aco_opcode::v_interp_p2_f32 &&
   instr->opcode != aco_opcode::v_interp_mov_f32 &&
   instr->opcode != aco_opcode::v_interp_p1ll_f16 &&
   instr->opcode != aco_opcode::v_interp_p1lv_f16 &&
   instr->opcode != aco_opcode::v_interp_p2_legacy_f16 &&
   instr->opcode != aco_opcode::v_interp_p2_f16 &&
   instr->opcode != aco_opcode::v_interp_p10_f32_inreg &&
   instr->opcode != aco_opcode::v_interp_p2_f32_inreg &&
   instr->opcode != aco_opcode::v_interp_p10_f16_f32_inreg &&
   instr->opcode != aco_opcode::v_interp_p2_f16_f32_inreg &&
   instr->opcode != aco_opcode::v_interp_p10_rtz_f16_f32_inreg &&
   instr->opcode != aco_opcode::v_interp_p2_rtz_f16_f32_inreg &&
   instr->opcode != aco_opcode::v_wmma_f32_16x16x16_f16 &&
   instr->opcode != aco_opcode::v_wmma_f32_16x16x16_bf16 &&
   instr->opcode != aco_opcode::v_wmma_f16_16x16x16_f16 &&
   instr->opcode != aco_opcode::v_wmma_bf16_16x16x16_bf16 &&
      }
      bool
   is_operand_vgpr(Operand op)
   {
         }
      /* only covers special cases */
   bool
   alu_can_accept_constant(const aco_ptr<Instruction>& instr, unsigned operand)
   {
      /* Fixed operands can't accept constants because we need them
   * to be in their fixed register.
   */
   assert(instr->operands.size() > operand);
   if (instr->operands[operand].isFixed())
            /* SOPP instructions can't use constants. */
   if (instr->isSOPP())
            switch (instr->opcode) {
   case aco_opcode::v_mac_f32:
   case aco_opcode::v_writelane_b32:
   case aco_opcode::v_writelane_b32_e64:
   case aco_opcode::v_cndmask_b32: return operand != 2;
   case aco_opcode::s_addk_i32:
   case aco_opcode::s_mulk_i32:
   case aco_opcode::p_extract_vector:
   case aco_opcode::p_split_vector:
   case aco_opcode::v_readlane_b32:
   case aco_opcode::v_readlane_b32_e64:
   case aco_opcode::v_readfirstlane_b32:
   case aco_opcode::p_extract:
   case aco_opcode::p_insert: return operand != 0;
   case aco_opcode::p_bpermute_readlane:
   case aco_opcode::p_bpermute_shared_vgpr:
   case aco_opcode::p_bpermute_permlane:
   case aco_opcode::p_interp_gfx11:
   case aco_opcode::p_dual_src_export_gfx11:
   case aco_opcode::v_interp_p1_f32:
   case aco_opcode::v_interp_p2_f32:
   case aco_opcode::v_interp_mov_f32:
   case aco_opcode::v_interp_p1ll_f16:
   case aco_opcode::v_interp_p1lv_f16:
   case aco_opcode::v_interp_p2_legacy_f16:
   case aco_opcode::v_interp_p10_f32_inreg:
   case aco_opcode::v_interp_p2_f32_inreg:
   case aco_opcode::v_interp_p10_f16_f32_inreg:
   case aco_opcode::v_interp_p2_f16_f32_inreg:
   case aco_opcode::v_interp_p10_rtz_f16_f32_inreg:
   case aco_opcode::v_interp_p2_rtz_f16_f32_inreg:
   case aco_opcode::v_wmma_f32_16x16x16_f16:
   case aco_opcode::v_wmma_f32_16x16x16_bf16:
   case aco_opcode::v_wmma_f16_16x16x16_f16:
   case aco_opcode::v_wmma_bf16_16x16x16_bf16:
   case aco_opcode::v_wmma_i32_16x16x16_iu8:
   case aco_opcode::v_wmma_i32_16x16x16_iu4: return false;
   default: return true;
      }
      bool
   valu_can_accept_vgpr(aco_ptr<Instruction>& instr, unsigned operand)
   {
      if (instr->opcode == aco_opcode::v_readlane_b32 ||
      instr->opcode == aco_opcode::v_readlane_b32_e64 ||
   instr->opcode == aco_opcode::v_writelane_b32 ||
   instr->opcode == aco_opcode::v_writelane_b32_e64)
      if (instr->opcode == aco_opcode::v_permlane16_b32 ||
      instr->opcode == aco_opcode::v_permlanex16_b32)
         }
      /* check constant bus and literal limitations */
   bool
   check_vop3_operands(opt_ctx& ctx, unsigned num_operands, Operand* operands)
   {
      int limit = ctx.program->gfx_level >= GFX10 ? 2 : 1;
   Operand literal32(s1);
   Operand literal64(s2);
   unsigned num_sgprs = 0;
            for (unsigned i = 0; i < num_operands; i++) {
               if (op.hasRegClass() && op.regClass().type() == RegType::sgpr) {
      /* two reads of the same SGPR count as 1 to the limit */
   if (op.tempId() != sgpr[0] && op.tempId() != sgpr[1]) {
      if (num_sgprs < 2)
         limit--;
   if (limit < 0)
         } else if (op.isLiteral()) {
                     if (!literal32.isUndefined() && literal32.constantValue() != op.constantValue())
                        /* Any number of 32-bit literals counts as only 1 to the limit. Same
   * (but separately) for 64-bit literals. */
   if (op.size() == 1 && literal32.isUndefined()) {
      limit--;
      } else if (op.size() == 2 && literal64.isUndefined()) {
      limit--;
               if (limit < 0)
                     }
      bool
   parse_base_offset(opt_ctx& ctx, Instruction* instr, unsigned op_index, Temp* base, uint32_t* offset,
         {
               if (!op.isTemp())
         Temp tmp = op.getTemp();
   if (!ctx.info[tmp.id()].is_add_sub())
                     unsigned mask = 0x3;
   bool is_sub = false;
   switch (add_instr->opcode) {
   case aco_opcode::v_add_u32:
   case aco_opcode::v_add_co_u32:
   case aco_opcode::v_add_co_u32_e64:
   case aco_opcode::s_add_i32:
   case aco_opcode::s_add_u32: break;
   case aco_opcode::v_sub_u32:
   case aco_opcode::v_sub_i32:
   case aco_opcode::v_sub_co_u32:
   case aco_opcode::v_sub_co_u32_e64:
   case aco_opcode::s_sub_u32:
   case aco_opcode::s_sub_i32:
      mask = 0x2;
   is_sub = true;
      case aco_opcode::v_subrev_u32:
   case aco_opcode::v_subrev_co_u32:
   case aco_opcode::v_subrev_co_u32_e64:
      mask = 0x1;
   is_sub = true;
      default: return false;
   }
   if (prevent_overflow && !add_instr->definitions[0].isNUW())
            if (add_instr->usesModifiers())
            u_foreach_bit (i, mask) {
      if (add_instr->operands[i].isConstant()) {
         } else if (add_instr->operands[i].isTemp() &&
               } else {
         }
   if (!add_instr->operands[!i].isTemp())
            uint32_t offset2 = 0;
   if (parse_base_offset(ctx, add_instr, !i, base, &offset2, prevent_overflow)) {
         } else {
         }
                  }
      void
   skip_smem_offset_align(opt_ctx& ctx, SMEM_instruction* smem)
   {
      bool soe = smem->operands.size() >= (!smem->definitions.empty() ? 3 : 4);
   if (soe && !smem->operands[1].isConstant())
         /* We don't need to check the constant offset because the address seems to be calculated with
   * (offset&-4 + const_offset&-4), not (offset+const_offset)&-4.
            Operand& op = smem->operands[soe ? smem->operands.size() - 1 : 1];
   if (!op.isTemp() || !ctx.info[op.tempId()].is_bitwise())
            Instruction* bitwise_instr = ctx.info[op.tempId()].instr;
   if (bitwise_instr->opcode != aco_opcode::s_and_b32)
            if (bitwise_instr->operands[0].constantEquals(-4) &&
      bitwise_instr->operands[1].isOfType(op.regClass().type()))
      else if (bitwise_instr->operands[1].constantEquals(-4) &&
            }
      void
   smem_combine(opt_ctx& ctx, aco_ptr<Instruction>& instr)
   {
      /* skip &-4 before offset additions: load((a + 16) & -4, 0) */
   if (!instr->operands.empty())
            /* propagate constants and combine additions */
   if (!instr->operands.empty() && instr->operands[1].isTemp()) {
      SMEM_instruction& smem = instr->smem();
            Temp base;
   uint32_t offset;
   if (info.is_constant_or_literal(32) &&
      ((ctx.program->gfx_level == GFX6 && info.val <= 0x3FF) ||
   (ctx.program->gfx_level == GFX7 && info.val <= 0xFFFFFFFF) ||
   (ctx.program->gfx_level >= GFX8 && info.val <= 0xFFFFF))) {
      } else if (parse_base_offset(ctx, instr.get(), 1, &base, &offset, true) &&
            base.regClass() == s1 && offset <= 0xFFFFF && ctx.program->gfx_level >= GFX9 &&
   bool soe = smem.operands.size() >= (!smem.definitions.empty() ? 3 : 4);
   if (soe) {
      if (ctx.info[smem.operands.back().tempId()].is_constant_or_literal(32) &&
      ctx.info[smem.operands.back().tempId()].val == 0) {
   smem.operands[1] = Operand::c32(offset);
         } else {
      SMEM_instruction* new_instr = create_instruction<SMEM_instruction>(
         new_instr->operands[0] = smem.operands[0];
   new_instr->operands[1] = Operand::c32(offset);
   if (smem.definitions.empty())
         new_instr->operands.back() = Operand(base);
   if (!smem.definitions.empty())
         new_instr->sync = smem.sync;
   new_instr->glc = smem.glc;
   new_instr->dlc = smem.dlc;
   new_instr->nv = smem.nv;
   new_instr->disable_wqm = smem.disable_wqm;
                     /* skip &-4 after offset additions: load(a & -4, 16) */
   if (!instr->operands.empty())
      }
      Operand
   get_constant_op(opt_ctx& ctx, ssa_info info, uint32_t bits)
   {
      if (bits == 64)
            }
      void
   propagate_constants_vop3p(opt_ctx& ctx, aco_ptr<Instruction>& instr, ssa_info& info, unsigned i)
   {
      if (!info.is_constant_or_literal(32))
            assert(instr->operands[i].isTemp());
   unsigned bits = get_operand_size(instr, i);
   if (info.is_constant(bits)) {
      instr->operands[i] = get_constant_op(ctx, info, bits);
               /* The accumulation operand of dot product instructions ignores opsel. */
   bool cannot_use_opsel =
      (instr->opcode == aco_opcode::v_dot4_i32_i8 || instr->opcode == aco_opcode::v_dot2_i32_i16 ||
   instr->opcode == aco_opcode::v_dot4_i32_iu8 || instr->opcode == aco_opcode::v_dot4_u32_u8 ||
   instr->opcode == aco_opcode::v_dot2_u32_u16) &&
      if (cannot_use_opsel)
            /* try to fold inline constants */
   VALU_instruction* vop3p = &instr->valu();
   bool opsel_lo = vop3p->opsel_lo[i];
            Operand const_op[2];
   bool const_opsel[2] = {false, false};
   for (unsigned j = 0; j < 2; j++) {
      if ((unsigned)opsel_lo != j && (unsigned)opsel_hi != j)
            uint16_t val = info.val >> (j ? 16 : 0);
   Operand op = Operand::get_const(ctx.program->gfx_level, val, bits / 8u);
   if (bits == 32 && op.isLiteral()) /* try sign extension */
         if (bits == 32 && op.isLiteral()) { /* try shifting left */
      op = Operand::get_const(ctx.program->gfx_level, val << 16, 4);
      }
   if (op.isLiteral())
                     Operand const_lo = const_op[0];
   Operand const_hi = const_op[1];
   bool const_lo_opsel = const_opsel[0];
            if (opsel_lo == opsel_hi) {
      /* use the single 16bit value */
            /* opsel must point the same for both halves */
   opsel_lo = opsel_lo ? const_hi_opsel : const_lo_opsel;
      } else if (const_lo == const_hi) {
      /* both constants are the same */
            /* opsel must point the same for both halves */
   opsel_lo = const_lo_opsel;
      } else if (const_lo.constantValue16(const_lo_opsel) ==
                     /* redirect opsel selection */
   opsel_lo = opsel_lo ? const_hi_opsel : !const_hi_opsel;
      } else if (const_hi.constantValue16(const_hi_opsel) ==
                     /* redirect opsel selection */
   opsel_lo = opsel_lo ? !const_lo_opsel : const_lo_opsel;
      } else if (bits == 16 && const_lo.constantValue() == (const_hi.constantValue() ^ (1 << 15))) {
               /* const_lo == -const_hi */
   if (!can_use_input_modifiers(ctx.program->gfx_level, instr->opcode, i))
            instr->operands[i] = Operand::c16(const_lo.constantValue() & 0x7FFF);
   bool neg_lo = const_lo.constantValue() & (1 << 15);
   vop3p->neg_lo[i] ^= opsel_lo ^ neg_lo;
            /* opsel must point to lo for both operands */
   opsel_lo = false;
               vop3p->opsel_lo[i] = opsel_lo;
      }
      bool
   fixed_to_exec(Operand op)
   {
         }
      SubdwordSel
   parse_extract(Instruction* instr)
   {
      if (instr->opcode == aco_opcode::p_extract) {
      unsigned size = instr->operands[2].constantValue() / 8;
   unsigned offset = instr->operands[1].constantValue() * size;
   bool sext = instr->operands[3].constantEquals(1);
      } else if (instr->opcode == aco_opcode::p_insert && instr->operands[1].constantEquals(0)) {
         } else if (instr->opcode == aco_opcode::p_extract_vector) {
      unsigned size = instr->definitions[0].bytes();
   unsigned offset = instr->operands[1].constantValue() * size;
   if (size <= 2)
      } else if (instr->opcode == aco_opcode::p_split_vector) {
      assert(instr->operands[0].bytes() == 4 && instr->definitions[1].bytes() == 2);
                  }
      SubdwordSel
   parse_insert(Instruction* instr)
   {
      if (instr->opcode == aco_opcode::p_extract && instr->operands[3].constantEquals(0) &&
      instr->operands[1].constantEquals(0)) {
      } else if (instr->opcode == aco_opcode::p_insert) {
      unsigned size = instr->operands[2].constantValue() / 8;
   unsigned offset = instr->operands[1].constantValue() * size;
      } else {
            }
      bool
   can_apply_extract(opt_ctx& ctx, aco_ptr<Instruction>& instr, unsigned idx, ssa_info& info)
   {
      Temp tmp = info.instr->operands[0].getTemp();
            if (!sel) {
         } else if (sel.size() == 4) {
         } else if ((instr->opcode == aco_opcode::v_cvt_f32_u32 ||
                     } else if (instr->opcode == aco_opcode::v_lshlrev_b32 && instr->operands[0].isConstant() &&
            sel.offset() == 0 &&
   ((sel.size() == 2 && instr->operands[0].constantValue() >= 16u) ||
      } else if (instr->opcode == aco_opcode::v_mul_u32_u24 && ctx.program->gfx_level >= GFX10 &&
            !instr->usesModifiers() && sel.size() == 2 && !sel.sign_extend() &&
   (instr->operands[!idx].is16bit() ||
      } else if (idx < 2 && can_use_SDWA(ctx.program->gfx_level, instr, true) &&
            if (instr->isSDWA() && instr->sdwa().sel[idx] != SubdwordSel::dword)
            } else if (instr->isVALU() && sel.size() == 2 && !instr->valu().opsel[idx] &&
               } else if (instr->opcode == aco_opcode::p_extract) {
               /* the outer offset must be within extracted range */
   if (instrSel.offset() >= sel.size())
            /* don't remove the sign-extension when increasing the size further */
   if (instrSel.size() > sel.size() && !instrSel.sign_extend() && sel.sign_extend())
                           }
      /* Combine an p_extract (or p_insert, in some cases) instruction with instr.
   * instr(p_extract(...)) -> instr()
   */
   void
   apply_extract(opt_ctx& ctx, aco_ptr<Instruction>& instr, unsigned idx, ssa_info& info)
   {
      Temp tmp = info.instr->operands[0].getTemp();
   SubdwordSel sel = parse_extract(info.instr);
            instr->operands[idx].set16bit(false);
                     if (sel.size() == 4) {
         } else if ((instr->opcode == aco_opcode::v_cvt_f32_u32 ||
                  switch (sel.offset()) {
   case 0: instr->opcode = aco_opcode::v_cvt_f32_ubyte0; break;
   case 1: instr->opcode = aco_opcode::v_cvt_f32_ubyte1; break;
   case 2: instr->opcode = aco_opcode::v_cvt_f32_ubyte2; break;
   case 3: instr->opcode = aco_opcode::v_cvt_f32_ubyte3; break;
      } else if (instr->opcode == aco_opcode::v_lshlrev_b32 && instr->operands[0].isConstant() &&
            sel.offset() == 0 &&
   ((sel.size() == 2 && instr->operands[0].constantValue() >= 16u) ||
   /* The undesirable upper bits are already shifted out. */
      } else if (instr->opcode == aco_opcode::v_mul_u32_u24 && ctx.program->gfx_level >= GFX10 &&
            !instr->usesModifiers() && sel.size() == 2 && !sel.sign_extend() &&
   (instr->operands[!idx].is16bit() ||
   Instruction* mad =
         mad->definitions[0] = instr->definitions[0];
   mad->operands[0] = instr->operands[0];
   mad->operands[1] = instr->operands[1];
   mad->operands[2] = Operand::zero();
   mad->valu().opsel[idx] = sel.offset();
   mad->pass_flags = instr->pass_flags;
      } else if (can_use_SDWA(ctx.program->gfx_level, instr, true) &&
            convert_to_SDWA(ctx.program->gfx_level, instr);
      } else if (instr->isVALU()) {
      if (sel.offset()) {
               /* VOP12C cannot use opsel with SGPRs. */
   if (!instr->isVOP3() && !instr->isVINTERP_INREG() &&
      !info.instr->operands[0].isOfType(RegType::vgpr))
      } else if (instr->opcode == aco_opcode::p_extract) {
               unsigned size = std::min(sel.size(), instrSel.size());
   unsigned offset = sel.offset() + instrSel.offset();
   unsigned sign_extend =
            instr->operands[1] = Operand::c32(offset / size);
   instr->operands[2] = Operand::c32(size * 8u);
   instr->operands[3] = Operand::c32(sign_extend);
               /* These are the only labels worth keeping at the moment. */
   for (Definition& def : instr->definitions) {
      ctx.info[def.tempId()].label &=
         if (ctx.info[def.tempId()].label & instr_usedef_labels)
         }
      void
   check_sdwa_extract(opt_ctx& ctx, aco_ptr<Instruction>& instr)
   {
      for (unsigned i = 0; i < instr->operands.size(); i++) {
      Operand op = instr->operands[i];
   if (!op.isTemp())
         ssa_info& info = ctx.info[op.tempId()];
   if (info.is_extract() && (info.instr->operands[0].getTemp().type() == RegType::vgpr ||
            if (!can_apply_extract(ctx, instr, i, info))
            }
      bool
   does_fp_op_flush_denorms(opt_ctx& ctx, aco_opcode op)
   {
      switch (op) {
   case aco_opcode::v_min_f32:
   case aco_opcode::v_max_f32:
   case aco_opcode::v_med3_f32:
   case aco_opcode::v_min3_f32:
   case aco_opcode::v_max3_f32:
   case aco_opcode::v_min_f16:
   case aco_opcode::v_max_f16: return ctx.program->gfx_level > GFX8;
   case aco_opcode::v_cndmask_b32:
   case aco_opcode::v_cndmask_b16:
   case aco_opcode::v_mov_b32:
   case aco_opcode::v_mov_b16: return false;
   default: return true;
      }
      bool
   can_eliminate_fcanonicalize(opt_ctx& ctx, aco_ptr<Instruction>& instr, Temp tmp, unsigned idx)
   {
      float_mode* fp = &ctx.fp_mode;
   if (ctx.info[tmp.id()].is_canonicalized() ||
      (tmp.bytes() == 4 ? fp->denorm32 : fp->denorm16_64) == fp_denorm_keep)
         aco_opcode op = instr->opcode;
   return can_use_input_modifiers(ctx.program->gfx_level, instr->opcode, idx) &&
      }
      bool
   can_eliminate_and_exec(opt_ctx& ctx, Temp tmp, unsigned pass_flags)
   {
      if (ctx.info[tmp.id()].is_vopc()) {
      Instruction* vopc_instr = ctx.info[tmp.id()].instr;
   /* Remove superfluous s_and when the VOPC instruction uses the same exec and thus
   * already produces the same result */
      }
   if (ctx.info[tmp.id()].is_bitwise()) {
      Instruction* instr = ctx.info[tmp.id()].instr;
   if (instr->operands.size() != 2 || instr->pass_flags != pass_flags)
         if (!(instr->operands[0].isTemp() && instr->operands[1].isTemp()))
         if (instr->opcode == aco_opcode::s_and_b32 || instr->opcode == aco_opcode::s_and_b64) {
      return can_eliminate_and_exec(ctx, instr->operands[0].getTemp(), pass_flags) ||
      } else {
      return can_eliminate_and_exec(ctx, instr->operands[0].getTemp(), pass_flags) &&
         }
      }
      bool
   is_copy_label(opt_ctx& ctx, aco_ptr<Instruction>& instr, ssa_info& info, unsigned idx)
   {
      return info.is_temp() ||
      }
      bool
   is_op_canonicalized(opt_ctx& ctx, Operand op)
   {
      float_mode* fp = &ctx.fp_mode;
   if ((op.isTemp() && ctx.info[op.tempId()].is_canonicalized()) ||
      (op.bytes() == 4 ? fp->denorm32 : fp->denorm16_64) == fp_denorm_keep)
         if (op.isConstant() || (op.isTemp() && ctx.info[op.tempId()].is_constant_or_literal(32))) {
      uint32_t val = op.isTemp() ? ctx.info[op.tempId()].val : op.constantValue();
   if (op.bytes() == 2)
         else if (op.bytes() == 4)
      }
      }
      bool
   is_scratch_offset_valid(opt_ctx& ctx, Instruction* instr, int64_t offset0, int64_t offset1)
   {
      bool negative_unaligned_scratch_offset_bug = ctx.program->gfx_level == GFX10;
   int32_t min = ctx.program->dev.scratch_global_offset_min;
                     bool has_vgpr_offset = instr && !instr->operands[0].isUndefined();
   if (negative_unaligned_scratch_offset_bug && has_vgpr_offset && offset < 0 && offset % 4)
               }
      bool
   detect_clamp(Instruction* instr, unsigned* clamped_idx)
   {
      VALU_instruction& valu = instr->valu();
   if (valu.omod != 0 || valu.opsel != 0)
            unsigned idx = 0;
   bool found_zero = false, found_one = false;
   bool is_fp16 = instr->opcode == aco_opcode::v_med3_f16;
   for (unsigned i = 0; i < 3; i++) {
      if (!valu.neg[i] && instr->operands[i].constantEquals(0))
         else if (!valu.neg[i] &&
               else
      }
   if (found_zero && found_one && instr->operands[idx].isTemp()) {
      *clamped_idx = idx;
      } else {
            }
      void
   label_instruction(opt_ctx& ctx, aco_ptr<Instruction>& instr)
   {
      if (instr->isSALU() || instr->isVALU() || instr->isPseudo()) {
      ASSERTED bool all_const = false;
   for (Operand& op : instr->operands)
      all_const =
               ASSERTED bool is_copy = instr->opcode == aco_opcode::s_mov_b32 ||
               perfwarn(ctx.program, is_copy && !instr->usesModifiers(), "Use p_parallelcopy instead",
               if (instr->isSMEM())
            for (unsigned i = 0; i < instr->operands.size(); i++) {
      if (!instr->operands[i].isTemp())
            ssa_info info = ctx.info[instr->operands[i].tempId()];
   /* propagate undef */
   if (info.is_undefined() && is_phi(instr))
         /* propagate reg->reg of same type */
   while (info.is_temp() && info.temp.regClass() == instr->operands[i].getTemp().regClass()) {
      instr->operands[i].setTemp(ctx.info[instr->operands[i].tempId()].temp);
               /* PSEUDO: propagate temporaries */
   if (instr->isPseudo()) {
      while (info.is_temp()) {
      pseudo_propagate_temp(ctx, instr, info.temp, i);
                  /* SALU / PSEUDO: propagate inline constants */
   if (instr->isSALU() || instr->isPseudo()) {
      unsigned bits = get_operand_size(instr, i);
   if ((info.is_constant(bits) || (info.is_literal(bits) && instr->isPseudo())) &&
      alu_can_accept_constant(instr, i)) {
   instr->operands[i] = get_constant_op(ctx, info, bits);
                  /* VALU: propagate neg, abs & inline constants */
   else if (instr->isVALU()) {
      if (is_copy_label(ctx, instr, info, i) && info.temp.type() == RegType::vgpr &&
      valu_can_accept_vgpr(instr, i)) {
   instr->operands[i].setTemp(info.temp);
      }
   /* applying SGPRs to VOP1 doesn't increase code size and DCE is helped by doing it earlier */
   if (info.is_temp() && info.temp.type() == RegType::sgpr && can_apply_sgprs(ctx, instr) &&
      instr->operands.size() == 1) {
   instr->format = withoutDPP(instr->format);
   instr->operands[i].setTemp(info.temp);
               /* for instructions other than v_cndmask_b32, the size of the instruction should match the
   * operand size */
   unsigned can_use_mod =
                        if (instr->isSDWA())
                                       if (info.is_neg() && instr->opcode == aco_opcode::v_add_f32 && mod_bitsize_compat) {
      instr->opcode = i ? aco_opcode::v_sub_f32 : aco_opcode::v_subrev_f32;
      } else if (info.is_neg() && instr->opcode == aco_opcode::v_add_f16 && mod_bitsize_compat) {
      instr->opcode = i ? aco_opcode::v_sub_f16 : aco_opcode::v_subrev_f16;
      } else if (info.is_neg() && can_use_mod && mod_bitsize_compat &&
            if (!instr->isDPP16() && can_use_VOP3(ctx, instr))
         instr->operands[i].setTemp(info.temp);
   if (!instr->valu().abs[i])
      }
   if (info.is_abs() && can_use_mod && mod_bitsize_compat &&
      can_eliminate_fcanonicalize(ctx, instr, info.temp, i)) {
   if (!instr->isDPP16() && can_use_VOP3(ctx, instr))
         instr->operands[i] = Operand(info.temp);
   instr->valu().abs[i] = true;
               if (instr->isVOP3P()) {
      propagate_constants_vop3p(ctx, instr, info, i);
               if (info.is_constant(bits) && alu_can_accept_constant(instr, i) &&
      (!instr->isSDWA() || ctx.program->gfx_level >= GFX9) && (!instr->isDPP() || i != 1)) {
   Operand op = get_constant_op(ctx, info, bits);
   perfwarn(ctx.program, instr->opcode == aco_opcode::v_cndmask_b32 && i == 2,
         if (i == 0 || instr->isSDWA() || instr->opcode == aco_opcode::v_readlane_b32 ||
      instr->opcode == aco_opcode::v_writelane_b32) {
   instr->format = withoutDPP(instr->format);
   instr->operands[i] = op;
      } else if (!instr->isVOP3() && can_swap_operands(instr, &instr->opcode)) {
      instr->operands[i] = op;
   instr->valu().swapOperands(0, i);
      } else if (can_use_VOP3(ctx, instr)) {
      instr->format = asVOP3(instr->format);
   instr->operands[i] = op;
                     /* MUBUF: propagate constants and combine additions */
   else if (instr->isMUBUF()) {
      MUBUF_instruction& mubuf = instr->mubuf();
   Temp base;
   uint32_t offset;
                  /* According to AMDGPUDAGToDAGISel::SelectMUBUFScratchOffen(), vaddr
   * overflow for scratch accesses works only on GFX9+ and saddr overflow
   * never works. Since swizzling is the only thing that separates
   * scratch accesses and other accesses and swizzling changing how
   * addressing works significantly, this probably applies to swizzled
                  if (mubuf.offen && mubuf.idxen && i == 1 && info.is_vec() &&
      info.instr->operands.size() == 2 && info.instr->operands[0].isTemp() &&
   info.instr->operands[0].regClass() == v1 && info.instr->operands[1].isConstant() &&
   mubuf.offset + info.instr->operands[1].constantValue() < 4096) {
   instr->operands[1] = info.instr->operands[0];
   mubuf.offset += info.instr->operands[1].constantValue();
   mubuf.offen = false;
      } else if (mubuf.offen && i == 1 && info.is_constant_or_literal(32) &&
            assert(!mubuf.idxen);
   instr->operands[1] = Operand(v1);
   mubuf.offset += info.val;
   mubuf.offen = false;
      } else if (i == 2 && info.is_constant_or_literal(32) && mubuf.offset + info.val < 4096) {
      instr->operands[2] = Operand::c32(0);
   mubuf.offset += info.val;
      } else if (mubuf.offen && i == 1 &&
            parse_base_offset(ctx, instr.get(), i, &base, &offset,
         assert(!mubuf.idxen);
   instr->operands[1].setTemp(base);
   mubuf.offset += offset;
      } else if (i == 2 && parse_base_offset(ctx, instr.get(), i, &base, &offset, true) &&
            instr->operands[i].setTemp(base);
   mubuf.offset += offset;
                  else if (instr->isMTBUF()) {
      MTBUF_instruction& mtbuf = instr->mtbuf();
                  if (mtbuf.offen && mtbuf.idxen && i == 1 && info.is_vec() &&
      info.instr->operands.size() == 2 && info.instr->operands[0].isTemp() &&
   info.instr->operands[0].regClass() == v1 && info.instr->operands[1].isConstant() &&
   mtbuf.offset + info.instr->operands[1].constantValue() < 4096) {
   instr->operands[1] = info.instr->operands[0];
   mtbuf.offset += info.instr->operands[1].constantValue();
   mtbuf.offen = false;
                  /* SCRATCH: propagate constants and combine additions */
   else if (instr->isScratch()) {
      FLAT_instruction& scratch = instr->scratch();
   Temp base;
   uint32_t offset;
                  /* The hardware probably does: 'scratch_base + u2u64(saddr) + i2i64(offset)'. This means
   * we can't combine the addition if the unsigned addition overflows and offset is
   * positive. In theory, there is also issues if
   * 'ilt(offset, 0) && ige(saddr, 0) && ilt(saddr + offset, 0)', but that just
   * replaces an already out-of-bounds access with a larger one since 'saddr + offset'
   * would be larger than INT32_MAX.
   */
   if (i <= 1 && parse_base_offset(ctx, instr.get(), i, &base, &offset, true) &&
      base.regClass() == instr->operands[i].regClass() &&
   is_scratch_offset_valid(ctx, instr.get(), scratch.offset, (int32_t)offset)) {
   instr->operands[i].setTemp(base);
   scratch.offset += (int32_t)offset;
      } else if (i <= 1 && parse_base_offset(ctx, instr.get(), i, &base, &offset, false) &&
            base.regClass() == instr->operands[i].regClass() && (int32_t)offset < 0 &&
   instr->operands[i].setTemp(base);
   scratch.offset += (int32_t)offset;
      } else if (i <= 1 && info.is_constant_or_literal(32) &&
            ctx.program->gfx_level >= GFX10_3 &&
   /* GFX10.3+ can disable both SADDR and ADDR. */
   instr->operands[i] = Operand(instr->operands[i].regClass());
   scratch.offset += (int32_t)info.val;
                  /* DS: combine additions */
               DS_instruction& ds = instr->ds();
   Temp base;
   uint32_t offset;
   bool has_usable_ds_offset = ctx.program->gfx_level >= GFX7;
   if (has_usable_ds_offset && i == 0 &&
      parse_base_offset(ctx, instr.get(), i, &base, &offset, false) &&
   base.regClass() == instr->operands[i].regClass() &&
   instr->opcode != aco_opcode::ds_swizzle_b32) {
   if (instr->opcode == aco_opcode::ds_write2_b32 ||
      instr->opcode == aco_opcode::ds_read2_b32 ||
   instr->opcode == aco_opcode::ds_write2_b64 ||
   instr->opcode == aco_opcode::ds_read2_b64 ||
   instr->opcode == aco_opcode::ds_write2st64_b32 ||
   instr->opcode == aco_opcode::ds_read2st64_b32 ||
   instr->opcode == aco_opcode::ds_write2st64_b64 ||
   instr->opcode == aco_opcode::ds_read2st64_b64) {
   bool is64bit = instr->opcode == aco_opcode::ds_write2_b64 ||
                     bool st64 = instr->opcode == aco_opcode::ds_write2st64_b32 ||
                                    if ((offset & mask) == 0 && ds.offset0 + (offset >> shifts) <= 255 &&
      ds.offset1 + (offset >> shifts) <= 255) {
   instr->operands[i].setTemp(base);
   ds.offset0 += offset >> shifts;
         } else {
      if (ds.offset0 + offset <= 65535) {
      instr->operands[i].setTemp(base);
                        else if (instr->isBranch()) {
      if (ctx.info[instr->operands[0].tempId()].is_scc_invert()) {
      /* Flip the branch instruction to get rid of the scc_invert instruction */
   instr->opcode = instr->opcode == aco_opcode::p_cbranch_z ? aco_opcode::p_cbranch_nz
                           /* if this instruction doesn't define anything, return */
   if (instr->definitions.empty()) {
      check_sdwa_extract(ctx, instr);
               if (instr->isVALU() || instr->isVINTRP()) {
      if (instr_info.can_use_output_modifiers[(int)instr->opcode] || instr->isVINTRP() ||
      instr->opcode == aco_opcode::v_cndmask_b32) {
   bool canonicalized = true;
   if (!does_fp_op_flush_denorms(ctx, instr->opcode)) {
      unsigned ops = instr->opcode == aco_opcode::v_cndmask_b32 ? 2 : instr->operands.size();
   for (unsigned i = 0; canonicalized && (i < ops); i++)
      }
   if (canonicalized)
               if (instr->isVOPC()) {
      ctx.info[instr->definitions[0].tempId()].set_vopc(instr.get());
   check_sdwa_extract(ctx, instr);
      }
   if (instr->isVOP3P()) {
      ctx.info[instr->definitions[0].tempId()].set_vop3p(instr.get());
                  switch (instr->opcode) {
   case aco_opcode::p_create_vector: {
      bool copy_prop = instr->operands.size() == 1 && instr->operands[0].isTemp() &&
         if (copy_prop) {
      ctx.info[instr->definitions[0].tempId()].set_temp(instr->operands[0].getTemp());
               /* expand vector operands */
   std::vector<Operand> ops;
   unsigned offset = 0;
   for (const Operand& op : instr->operands) {
      /* ensure that any expanded operands are properly aligned */
   bool aligned = offset % 4 == 0 || op.bytes() < 4;
   offset += op.bytes();
   if (aligned && op.isTemp() && ctx.info[op.tempId()].is_vec()) {
      Instruction* vec = ctx.info[op.tempId()].instr;
   for (const Operand& vec_op : vec->operands)
      } else {
                     /* combine expanded operands to new vector */
   if (ops.size() != instr->operands.size()) {
      assert(ops.size() > instr->operands.size());
   Definition def = instr->definitions[0];
   instr.reset(create_instruction<Pseudo_instruction>(aco_opcode::p_create_vector,
         for (unsigned i = 0; i < ops.size(); i++) {
      if (ops[i].isTemp() && ctx.info[ops[i].tempId()].is_temp() &&
      ops[i].regClass() == ctx.info[ops[i].tempId()].temp.regClass())
         }
      } else {
      for (unsigned i = 0; i < ops.size(); i++) {
            }
            if (instr->operands.size() == 2) {
      /* check if this is created from split_vector */
   if (instr->operands[1].isTemp() && ctx.info[instr->operands[1].tempId()].is_split()) {
      Instruction* split = ctx.info[instr->operands[1].tempId()].instr;
   if (instr->operands[0].isTemp() &&
      instr->operands[0].getTemp() == split->definitions[0].getTemp())
      }
      }
   case aco_opcode::p_split_vector: {
               if (info.is_constant_or_literal(32)) {
      uint64_t val = info.val;
   for (Definition def : instr->definitions) {
      uint32_t mask = u_bit_consecutive(0, def.bytes() * 8u);
   ctx.info[def.tempId()].set_constant(ctx.program->gfx_level, val & mask);
      }
      } else if (!info.is_vec()) {
      if (instr->definitions.size() == 2 && instr->operands[0].isTemp() &&
      instr->definitions[0].bytes() == instr->definitions[1].bytes()) {
   ctx.info[instr->definitions[1].tempId()].set_split(instr.get());
   if (instr->operands[0].bytes() == 4) {
      /* D16 subdword split */
   ctx.info[instr->definitions[0].tempId()].set_temp(instr->operands[0].getTemp());
         }
               Instruction* vec = ctx.info[instr->operands[0].tempId()].instr;
   unsigned split_offset = 0;
   unsigned vec_offset = 0;
   unsigned vec_index = 0;
   for (unsigned i = 0; i < instr->definitions.size();
      split_offset += instr->definitions[i++].bytes()) {
                  if (vec_offset != split_offset ||
                  Operand vec_op = vec->operands[vec_index];
   if (vec_op.isConstant()) {
      ctx.info[instr->definitions[i].tempId()].set_constant(ctx.program->gfx_level,
      } else if (vec_op.isUndefined()) {
         } else {
      assert(vec_op.isTemp());
         }
      }
   case aco_opcode::p_extract_vector: { /* mov */
      ssa_info& info = ctx.info[instr->operands[0].tempId()];
   const unsigned index = instr->operands[1].constantValue();
            if (info.is_vec()) {
      /* check if we index directly into a vector element */
                  for (const Operand& op : vec->operands) {
      if (offset < dst_offset) {
      offset += op.bytes();
      } else if (offset != dst_offset || op.bytes() != instr->definitions[0].bytes()) {
         }
   instr->operands[0] = op;
         } else if (info.is_constant_or_literal(32)) {
      /* propagate constants */
   uint32_t mask = u_bit_consecutive(0, instr->definitions[0].bytes() * 8u);
   uint32_t val = (info.val >> (dst_offset * 8u)) & mask;
   instr->operands[0] =
                     if (instr->operands[0].bytes() != instr->definitions[0].bytes()) {
                     if (index == 0)
         else
                     /* convert this extract into a copy instruction */
   instr->opcode = aco_opcode::p_parallelcopy;
   instr->operands.pop_back();
      }
   case aco_opcode::p_parallelcopy: /* propagate */
      if (instr->operands[0].isTemp() && ctx.info[instr->operands[0].tempId()].is_vec() &&
      instr->operands[0].regClass() != instr->definitions[0].regClass()) {
   /* We might not be able to copy-propagate if it's a SGPR->VGPR copy, so
   * duplicate the vector instead.
   */
                  instr.reset(create_instruction<Pseudo_instruction>(
         instr->definitions[0] = old_copy->definitions[0];
   std::copy(vec->operands.begin(), vec->operands.end(), instr->operands.begin());
   for (unsigned i = 0; i < vec->operands.size(); i++) {
      Operand& op = instr->operands[i];
   if (op.isTemp() && ctx.info[op.tempId()].is_temp() &&
      ctx.info[op.tempId()].temp.type() == instr->definitions[0].regClass().type())
   }
   ctx.info[instr->definitions[0].tempId()].set_vec(instr.get());
      }
      case aco_opcode::p_as_uniform:
      if (instr->definitions[0].isFixed()) {
         } else if (instr->operands[0].isConstant()) {
      ctx.info[instr->definitions[0].tempId()].set_constant(
      } else if (instr->operands[0].isTemp()) {
      ctx.info[instr->definitions[0].tempId()].set_temp(instr->operands[0].getTemp());
   if (ctx.info[instr->operands[0].tempId()].is_canonicalized())
      } else {
         }
      case aco_opcode::v_mov_b32:
      if (instr->isDPP16()) {
      /* anything else doesn't make sense in SSA */
   assert(instr->dpp16().row_mask == 0xf && instr->dpp16().bank_mask == 0xf);
      } else if (instr->isDPP8()) {
         }
      case aco_opcode::p_is_helper:
      if (!ctx.program->needs_wqm)
            case aco_opcode::v_mul_f64: ctx.info[instr->definitions[0].tempId()].set_mul(instr.get()); break;
   case aco_opcode::v_mul_f16:
   case aco_opcode::v_mul_f32:
   case aco_opcode::v_mul_legacy_f32: { /* omod */
               /* TODO: try to move the negate/abs modifier to the consumer instead */
   bool uses_mods = instr->usesModifiers();
            for (unsigned i = 0; i < 2; i++) {
      if (instr->operands[!i].isConstant() && instr->operands[i].isTemp()) {
      if (!instr->isDPP() && !instr->isSDWA() && !instr->valu().opsel &&
                                                               Temp other = instr->operands[i].getTemp();
   if (abs && neg && other.type() == RegType::vgpr)
         else if (abs && !neg && other.type() == RegType::vgpr)
         else if (!abs && neg && other.type() == RegType::vgpr)
         else if (!abs && !neg)
      } else if (uses_mods || ((fp16 ? ctx.fp_mode.preserve_signed_zero_inf_nan16_64
                     } else if (instr->operands[!i].constantValue() == 0u) { /* 0.0 */
         } else if ((fp16 ? ctx.fp_mode.denorm16_64 : ctx.fp_mode.denorm32) != fp_denorm_flush) {
      /* omod has no effect if denormals are enabled. */
      } else if (instr->operands[!i].constantValue() ==
               } else if (instr->operands[!i].constantValue() ==
               } else if (instr->operands[!i].constantValue() ==
               } else {
         }
         }
      }
   case aco_opcode::v_mul_lo_u16:
   case aco_opcode::v_mul_lo_u16_e64:
   case aco_opcode::v_mul_u32_u24:
      ctx.info[instr->definitions[0].tempId()].set_usedef(instr.get());
      case aco_opcode::v_med3_f16:
   case aco_opcode::v_med3_f32: { /* clamp */
      unsigned idx;
   if (detect_clamp(instr.get(), &idx) && !instr->valu().abs && !instr->valu().neg)
            }
   case aco_opcode::v_cndmask_b32:
      if (instr->operands[0].constantEquals(0) && instr->operands[1].constantEquals(0xFFFFFFFF))
         else if (instr->operands[0].constantEquals(0) &&
               else if (instr->operands[0].constantEquals(0) && instr->operands[1].constantEquals(1))
               case aco_opcode::v_cmp_lg_u32:
      if (instr->format == Format::VOPC && /* don't optimize VOP3 / SDWA / DPP */
      instr->operands[0].constantEquals(0) && instr->operands[1].isTemp() &&
   ctx.info[instr->operands[1].tempId()].is_vcc())
   ctx.info[instr->definitions[0].tempId()].set_temp(
         case aco_opcode::p_linear_phi: {
      /* lower_bool_phis() can create phis like this */
   bool all_same_temp = instr->operands[0].isTemp();
   /* this check is needed when moving uniform loop counters out of a divergent loop */
   if (all_same_temp)
         for (unsigned i = 1; all_same_temp && (i < instr->operands.size()); i++) {
      if (!instr->operands[i].isTemp() ||
      instr->operands[i].tempId() != instr->operands[0].tempId())
   }
   if (all_same_temp) {
         } else {
      bool all_undef = instr->operands[0].isUndefined();
   for (unsigned i = 1; all_undef && (i < instr->operands.size()); i++) {
      if (!instr->operands[i].isUndefined())
      }
   if (all_undef)
      }
      }
   case aco_opcode::v_add_u32:
   case aco_opcode::v_add_co_u32:
   case aco_opcode::v_add_co_u32_e64:
   case aco_opcode::s_add_i32:
   case aco_opcode::s_add_u32:
   case aco_opcode::v_subbrev_co_u32:
   case aco_opcode::v_sub_u32:
   case aco_opcode::v_sub_i32:
   case aco_opcode::v_sub_co_u32:
   case aco_opcode::v_sub_co_u32_e64:
   case aco_opcode::s_sub_u32:
   case aco_opcode::s_sub_i32:
   case aco_opcode::v_subrev_u32:
   case aco_opcode::v_subrev_co_u32:
   case aco_opcode::v_subrev_co_u32_e64:
      ctx.info[instr->definitions[0].tempId()].set_add_sub(instr.get());
      case aco_opcode::s_not_b32:
   case aco_opcode::s_not_b64:
      if (!instr->operands[0].isTemp()) {
   } else if (ctx.info[instr->operands[0].tempId()].is_uniform_bool()) {
      ctx.info[instr->definitions[0].tempId()].set_uniform_bitwise();
   ctx.info[instr->definitions[1].tempId()].set_scc_invert(
      } else if (ctx.info[instr->operands[0].tempId()].is_uniform_bitwise()) {
      ctx.info[instr->definitions[0].tempId()].set_uniform_bitwise();
   ctx.info[instr->definitions[1].tempId()].set_scc_invert(
      }
   ctx.info[instr->definitions[0].tempId()].set_bitwise(instr.get());
      case aco_opcode::s_and_b32:
   case aco_opcode::s_and_b64:
      if (fixed_to_exec(instr->operands[1]) && instr->operands[0].isTemp()) {
      if (ctx.info[instr->operands[0].tempId()].is_uniform_bool()) {
      /* Try to get rid of the superfluous s_cselect + s_and_b64 that comes from turning a
   * uniform bool into divergent */
   ctx.info[instr->definitions[1].tempId()].set_temp(
         ctx.info[instr->definitions[0].tempId()].set_uniform_bool(
            } else if (ctx.info[instr->operands[0].tempId()].is_uniform_bitwise()) {
      /* Try to get rid of the superfluous s_and_b64, since the uniform bitwise instruction
   * already produces the same SCC */
   ctx.info[instr->definitions[1].tempId()].set_temp(
         ctx.info[instr->definitions[0].tempId()].set_uniform_bool(
            } else if ((ctx.program->stage.num_sw_stages() > 1 ||
                  /* In case of merged shaders, pass_flags=1 means that all lanes are active (exec=-1), so
   * s_and is unnecessary. */
   ctx.info[instr->definitions[0].tempId()].set_temp(instr->operands[0].getTemp());
         }
      case aco_opcode::s_or_b32:
   case aco_opcode::s_or_b64:
   case aco_opcode::s_xor_b32:
   case aco_opcode::s_xor_b64:
      if (std::all_of(instr->operands.begin(), instr->operands.end(),
                  [&ctx](const Operand& op)
   {
            }
   ctx.info[instr->definitions[0].tempId()].set_bitwise(instr.get());
      case aco_opcode::s_lshl_b32:
   case aco_opcode::v_or_b32:
   case aco_opcode::v_lshlrev_b32:
   case aco_opcode::v_bcnt_u32_b32:
   case aco_opcode::v_and_b32:
   case aco_opcode::v_xor_b32:
   case aco_opcode::v_not_b32:
      ctx.info[instr->definitions[0].tempId()].set_usedef(instr.get());
      case aco_opcode::v_min_f32:
   case aco_opcode::v_min_f16:
   case aco_opcode::v_min_u32:
   case aco_opcode::v_min_i32:
   case aco_opcode::v_min_u16:
   case aco_opcode::v_min_i16:
   case aco_opcode::v_min_u16_e64:
   case aco_opcode::v_min_i16_e64:
   case aco_opcode::v_max_f32:
   case aco_opcode::v_max_f16:
   case aco_opcode::v_max_u32:
   case aco_opcode::v_max_i32:
   case aco_opcode::v_max_u16:
   case aco_opcode::v_max_i16:
   case aco_opcode::v_max_u16_e64:
   case aco_opcode::v_max_i16_e64:
      ctx.info[instr->definitions[0].tempId()].set_minmax(instr.get());
      case aco_opcode::s_cselect_b64:
   case aco_opcode::s_cselect_b32:
      if (instr->operands[0].constantEquals((unsigned)-1) && instr->operands[1].constantEquals(0)) {
      /* Found a cselect that operates on a uniform bool that comes from eg. s_cmp */
      }
   if (instr->operands[2].isTemp() && ctx.info[instr->operands[2].tempId()].is_scc_invert()) {
      /* Flip the operands to get rid of the scc_invert instruction */
   std::swap(instr->operands[0], instr->operands[1]);
      }
      case aco_opcode::s_mul_i32:
      /* Testing every uint32_t shows that 0x3f800000*n is never a denormal.
   * This pattern is created from a uniform nir_op_b2f. */
   if (instr->operands[0].constantEquals(0x3f800000u))
            case aco_opcode::p_extract: {
      if (instr->definitions[0].bytes() == 4) {
      ctx.info[instr->definitions[0].tempId()].set_extract(instr.get());
   if (instr->operands[0].regClass() == v1 && parse_insert(instr.get()))
      }
      }
   case aco_opcode::p_insert: {
      if (instr->operands[0].bytes() == 4) {
      if (instr->operands[0].regClass() == v1)
         if (parse_extract(instr.get()))
            }
      }
   case aco_opcode::ds_read_u8:
   case aco_opcode::ds_read_u8_d16:
   case aco_opcode::ds_read_u16:
   case aco_opcode::ds_read_u16_d16: {
      ctx.info[instr->definitions[0].tempId()].set_usedef(instr.get());
      }
   case aco_opcode::v_mbcnt_lo_u32_b32: {
      if (instr->operands[0].constantEquals(-1) && instr->operands[1].constantEquals(0)) {
      if (ctx.program->wave_size == 32)
         else
      }
      }
   case aco_opcode::v_mbcnt_hi_u32_b32:
   case aco_opcode::v_mbcnt_hi_u32_b32_e64: {
      if (instr->operands[0].constantEquals(-1) && instr->operands[1].isTemp() &&
      ctx.info[instr->operands[1].tempId()].is_usedef()) {
   Instruction* usedef_instr = ctx.info[instr->operands[1].tempId()].instr;
   if (usedef_instr->opcode == aco_opcode::v_mbcnt_lo_u32_b32 &&
      usedef_instr->operands[0].constantEquals(-1) &&
   usedef_instr->operands[1].constantEquals(0))
   }
      }
   case aco_opcode::v_cvt_f16_f32: {
      if (instr->operands[0].isTemp())
            }
   case aco_opcode::v_cvt_f32_f16: {
      if (instr->operands[0].isTemp())
            }
   default: break;
            /* Don't remove label_extract if we can't apply the extract to
   * neg/abs instructions because we'll likely combine it into another valu. */
   if (!(ctx.info[instr->definitions[0].tempId()].label & (label_neg | label_abs)))
      }
      unsigned
   original_temp_id(opt_ctx& ctx, Temp tmp)
   {
      if (ctx.info[tmp.id()].is_temp())
         else
      }
      void
   decrease_op_uses_if_dead(opt_ctx& ctx, Instruction* instr)
   {
      if (is_dead(ctx.uses, instr)) {
      for (const Operand& op : instr->operands) {
      if (op.isTemp())
            }
      void
   decrease_uses(opt_ctx& ctx, Instruction* instr)
   {
      ctx.uses[instr->definitions[0].tempId()]--;
      }
      Operand
   copy_operand(opt_ctx& ctx, Operand op)
   {
      if (op.isTemp())
            }
      Instruction*
   follow_operand(opt_ctx& ctx, Operand op, bool ignore_uses = false)
   {
      if (!op.isTemp() || !(ctx.info[op.tempId()].label & instr_usedef_labels))
         if (!ignore_uses && ctx.uses[op.tempId()] > 1)
                     if (instr->definitions.size() == 2) {
      assert(instr->definitions[0].isTemp() && instr->definitions[0].tempId() == op.tempId());
   if (instr->definitions[1].isTemp() && ctx.uses[instr->definitions[1].tempId()])
               for (Operand& operand : instr->operands) {
      if (fixed_to_exec(operand))
                  }
      /* s_or_b64(neq(a, a), neq(b, b)) -> v_cmp_u_f32(a, b)
   * s_and_b64(eq(a, a), eq(b, b)) -> v_cmp_o_f32(a, b) */
   bool
   combine_ordering_test(opt_ctx& ctx, aco_ptr<Instruction>& instr)
   {
      if (instr->definitions[0].regClass() != ctx.program->lane_mask)
         if (instr->definitions[1].isTemp() && ctx.uses[instr->definitions[1].tempId()])
                     bitarray8 opsel = 0;
   Instruction* op_instr[2];
            unsigned bitsize = 0;
   for (unsigned i = 0; i < 2; i++) {
      op_instr[i] = follow_operand(ctx, instr->operands[i], true);
   if (!op_instr[i])
            aco_opcode expected_cmp = is_or ? aco_opcode::v_cmp_neq_f32 : aco_opcode::v_cmp_eq_f32;
            if (get_f32_cmp(op_instr[i]->opcode) != expected_cmp)
         if (bitsize && op_bitsize != bitsize)
         if (!op_instr[i]->operands[0].isTemp() || !op_instr[i]->operands[1].isTemp())
            if (op_instr[i]->isSDWA() || op_instr[i]->isDPP())
            VALU_instruction& valu = op_instr[i]->valu();
   if (valu.neg[0] != valu.neg[1] || valu.abs[0] != valu.abs[1] ||
      valu.opsel[0] != valu.opsel[1])
               Temp op0 = op_instr[i]->operands[0].getTemp();
   Temp op1 = op_instr[i]->operands[1].getTemp();
   if (original_temp_id(ctx, op0) != original_temp_id(ctx, op1))
            op[i] = op1;
               if (op[1].type() == RegType::sgpr) {
      std::swap(op[0], op[1]);
      }
   unsigned num_sgprs = (op[0].type() == RegType::sgpr) + (op[1].type() == RegType::sgpr);
   if (num_sgprs > (ctx.program->gfx_level >= GFX10 ? 2 : 1))
            aco_opcode new_op = aco_opcode::num_opcodes;
   switch (bitsize) {
   case 16: new_op = is_or ? aco_opcode::v_cmp_u_f16 : aco_opcode::v_cmp_o_f16; break;
   case 32: new_op = is_or ? aco_opcode::v_cmp_u_f32 : aco_opcode::v_cmp_o_f32; break;
   case 64: new_op = is_or ? aco_opcode::v_cmp_u_f64 : aco_opcode::v_cmp_o_f64; break;
   }
   bool needs_vop3 = num_sgprs > 1 || (opsel[0] && op[0].type() != RegType::vgpr);
   VALU_instruction* new_instr = create_instruction<VALU_instruction>(
            new_instr->opsel = opsel;
   new_instr->operands[0] = copy_operand(ctx, Operand(op[0]));
   new_instr->operands[1] = copy_operand(ctx, Operand(op[1]));
   new_instr->definitions[0] = instr->definitions[0];
            decrease_uses(ctx, op_instr[0]);
            ctx.info[instr->definitions[0].tempId()].label = 0;
                        }
      /* s_or_b64(v_cmp_u_f32(a, b), cmp(a, b)) -> get_unordered(cmp)(a, b)
   * s_and_b64(v_cmp_o_f32(a, b), cmp(a, b)) -> get_ordered(cmp)(a, b) */
   bool
   combine_comparison_ordering(opt_ctx& ctx, aco_ptr<Instruction>& instr)
   {
      if (instr->definitions[0].regClass() != ctx.program->lane_mask)
         if (instr->definitions[1].isTemp() && ctx.uses[instr->definitions[1].tempId()])
            bool is_or = instr->opcode == aco_opcode::s_or_b64 || instr->opcode == aco_opcode::s_or_b32;
            Instruction* nan_test = follow_operand(ctx, instr->operands[0], true);
   Instruction* cmp = follow_operand(ctx, instr->operands[1], true);
   if (!nan_test || !cmp)
         if (nan_test->isSDWA() || cmp->isSDWA())
            if (get_f32_cmp(cmp->opcode) == expected_nan_test)
         else if (get_f32_cmp(nan_test->opcode) != expected_nan_test)
            if (!is_fp_cmp(cmp->opcode) || get_cmp_bitsize(cmp->opcode) != get_cmp_bitsize(nan_test->opcode))
            if (!nan_test->operands[0].isTemp() || !nan_test->operands[1].isTemp())
         if (!cmp->operands[0].isTemp() || !cmp->operands[1].isTemp())
            unsigned prop_cmp0 = original_temp_id(ctx, cmp->operands[0].getTemp());
   unsigned prop_cmp1 = original_temp_id(ctx, cmp->operands[1].getTemp());
   unsigned prop_nan0 = original_temp_id(ctx, nan_test->operands[0].getTemp());
   unsigned prop_nan1 = original_temp_id(ctx, nan_test->operands[1].getTemp());
   VALU_instruction& cmp_valu = cmp->valu();
   VALU_instruction& nan_valu = nan_test->valu();
   if ((prop_cmp0 != prop_nan0 || cmp_valu.opsel[0] != nan_valu.opsel[0]) &&
      (prop_cmp0 != prop_nan1 || cmp_valu.opsel[0] != nan_valu.opsel[1]))
      if ((prop_cmp1 != prop_nan0 || cmp_valu.opsel[1] != nan_valu.opsel[0]) &&
      (prop_cmp1 != prop_nan1 || cmp_valu.opsel[1] != nan_valu.opsel[1]))
      if (prop_cmp0 == prop_cmp1 && cmp_valu.opsel[0] == cmp_valu.opsel[1])
            aco_opcode new_op = is_or ? get_unordered(cmp->opcode) : get_ordered(cmp->opcode);
   VALU_instruction* new_instr = create_instruction<VALU_instruction>(
         new_instr->neg = cmp_valu.neg;
   new_instr->abs = cmp_valu.abs;
   new_instr->clamp = cmp_valu.clamp;
   new_instr->omod = cmp_valu.omod;
   new_instr->opsel = cmp_valu.opsel;
   new_instr->operands[0] = copy_operand(ctx, cmp->operands[0]);
   new_instr->operands[1] = copy_operand(ctx, cmp->operands[1]);
   new_instr->definitions[0] = instr->definitions[0];
            decrease_uses(ctx, nan_test);
            ctx.info[instr->definitions[0].tempId()].label = 0;
                        }
      /* Optimize v_cmp of constant with subgroup invocation to a constant mask.
   * Ideally, we can trade v_cmp for a constant (or literal).
   * In a less ideal case, we trade v_cmp for a SALU instruction, which is still a win.
   */
   bool
   optimize_cmp_subgroup_invocation(opt_ctx& ctx, aco_ptr<Instruction>& instr)
   {
      /* This optimization only applies to VOPC with 2 operands. */
   if (instr->operands.size() != 2)
            /* Find the constant operand or return early if there isn't one. */
   const int const_op_idx = instr->operands[0].isConstant()   ? 0
               if (const_op_idx == -1)
            /* Find the operand that has the subgroup invocation. */
   const int mbcnt_op_idx = 1 - const_op_idx;
   const Operand mbcnt_op = instr->operands[mbcnt_op_idx];
   if (!mbcnt_op.isTemp() || !ctx.info[mbcnt_op.tempId()].is_subgroup_invocation())
            /* Adjust opcode so we don't have to care about const_op_idx below. */
   const aco_opcode op = const_op_idx == 0 ? get_swapped(instr->opcode) : instr->opcode;
   const unsigned wave_size = ctx.program->wave_size;
            /* Find suitable constant bitmask corresponding to the value. */
   unsigned first_bit = 0, num_bits = 0;
   switch (op) {
   case aco_opcode::v_cmp_eq_u32:
   case aco_opcode::v_cmp_eq_i32:
      first_bit = val;
   num_bits = val >= wave_size ? 0 : 1;
      case aco_opcode::v_cmp_le_u32:
   case aco_opcode::v_cmp_le_i32:
      first_bit = 0;
   num_bits = val >= wave_size ? wave_size : (val + 1);
      case aco_opcode::v_cmp_lt_u32:
   case aco_opcode::v_cmp_lt_i32:
      first_bit = 0;
   num_bits = val >= wave_size ? wave_size : val;
      case aco_opcode::v_cmp_ge_u32:
   case aco_opcode::v_cmp_ge_i32:
      first_bit = val;
   num_bits = val >= wave_size ? 0 : (wave_size - val);
      case aco_opcode::v_cmp_gt_u32:
   case aco_opcode::v_cmp_gt_i32:
      first_bit = val + 1;
   num_bits = val >= wave_size ? 0 : (wave_size - val - 1);
      default: return false;
            Instruction* cpy = NULL;
   const uint64_t mask = BITFIELD64_RANGE(first_bit, num_bits);
   if (wave_size == 64 && mask > 0x7fffffff && mask != -1ull) {
      /* Mask can't be represented as a 64-bit constant or literal, use s_bfm_b64. */
   cpy = create_instruction<SOP2_instruction>(aco_opcode::s_bfm_b64, Format::SOP2, 2, 1);
   cpy->operands[0] = Operand::c32(num_bits);
      } else {
      /* Copy mask as a literal constant. */
   cpy =
                     cpy->definitions[0] = instr->definitions[0];
   ctx.info[instr->definitions[0].tempId()].label = 0;
   decrease_uses(ctx, ctx.info[mbcnt_op.tempId()].instr);
               }
      bool
   is_operand_constant(opt_ctx& ctx, Operand op, unsigned bit_size, uint64_t* value)
   {
      if (op.isConstant()) {
      *value = op.constantValue64();
      } else if (op.isTemp()) {
      unsigned id = original_temp_id(ctx, op.getTemp());
   if (!ctx.info[id].is_constant_or_literal(bit_size))
         *value = get_constant_op(ctx, ctx.info[id], bit_size).constantValue64();
      }
      }
      bool
   is_constant_nan(uint64_t value, unsigned bit_size)
   {
      if (bit_size == 16)
         else if (bit_size == 32)
         else
      }
      /* s_or_b64(v_cmp_neq_f32(a, a), cmp(a, #b)) and b is not NaN -> get_unordered(cmp)(a, b)
   * s_and_b64(v_cmp_eq_f32(a, a), cmp(a, #b)) and b is not NaN -> get_ordered(cmp)(a, b) */
   bool
   combine_constant_comparison_ordering(opt_ctx& ctx, aco_ptr<Instruction>& instr)
   {
      if (instr->definitions[0].regClass() != ctx.program->lane_mask)
         if (instr->definitions[1].isTemp() && ctx.uses[instr->definitions[1].tempId()])
                     Instruction* nan_test = follow_operand(ctx, instr->operands[0], true);
            if (!nan_test || !cmp || nan_test->isSDWA() || cmp->isSDWA() || nan_test->isDPP() ||
      cmp->isDPP())
         aco_opcode expected_nan_test = is_or ? aco_opcode::v_cmp_neq_f32 : aco_opcode::v_cmp_eq_f32;
   if (get_f32_cmp(cmp->opcode) == expected_nan_test)
         else if (get_f32_cmp(nan_test->opcode) != expected_nan_test)
            unsigned bit_size = get_cmp_bitsize(cmp->opcode);
   if (!is_fp_cmp(cmp->opcode) || get_cmp_bitsize(nan_test->opcode) != bit_size)
            if (!nan_test->operands[0].isTemp() || !nan_test->operands[1].isTemp())
         if (!cmp->operands[0].isTemp() && !cmp->operands[1].isTemp())
            unsigned prop_nan0 = original_temp_id(ctx, nan_test->operands[0].getTemp());
   unsigned prop_nan1 = original_temp_id(ctx, nan_test->operands[1].getTemp());
   if (prop_nan0 != prop_nan1)
            VALU_instruction& vop3 = nan_test->valu();
   if (vop3.neg[0] != vop3.neg[1] || vop3.abs[0] != vop3.abs[1] || vop3.opsel[0] != vop3.opsel[1])
            int constant_operand = -1;
   for (unsigned i = 0; i < 2; i++) {
      if (cmp->operands[i].isTemp() &&
      original_temp_id(ctx, cmp->operands[i].getTemp()) == prop_nan0 &&
   cmp->valu().opsel[i] == nan_test->valu().opsel[0]) {
   constant_operand = !i;
         }
   if (constant_operand == -1)
            uint64_t constant_value;
   if (!is_operand_constant(ctx, cmp->operands[constant_operand], bit_size, &constant_value))
         if (is_constant_nan(constant_value >> (cmp->valu().opsel[constant_operand] * 16), bit_size))
            aco_opcode new_op = is_or ? get_unordered(cmp->opcode) : get_ordered(cmp->opcode);
   Instruction* new_instr = create_instruction<VALU_instruction>(new_op, cmp->format, 2, 1);
   new_instr->valu().neg = cmp->valu().neg;
   new_instr->valu().abs = cmp->valu().abs;
   new_instr->valu().clamp = cmp->valu().clamp;
   new_instr->valu().omod = cmp->valu().omod;
   new_instr->valu().opsel = cmp->valu().opsel;
   new_instr->operands[0] = copy_operand(ctx, cmp->operands[0]);
   new_instr->operands[1] = copy_operand(ctx, cmp->operands[1]);
   new_instr->definitions[0] = instr->definitions[0];
            decrease_uses(ctx, nan_test);
            ctx.info[instr->definitions[0].tempId()].label = 0;
                        }
      /* s_not(cmp(a, b)) -> get_inverse(cmp)(a, b) */
   bool
   combine_inverse_comparison(opt_ctx& ctx, aco_ptr<Instruction>& instr)
   {
      if (ctx.uses[instr->definitions[1].tempId()])
         if (!instr->operands[0].isTemp() || ctx.uses[instr->operands[0].tempId()] != 1)
            Instruction* cmp = follow_operand(ctx, instr->operands[0]);
   if (!cmp)
            aco_opcode new_opcode = get_inverse(cmp->opcode);
   if (new_opcode == aco_opcode::num_opcodes)
            /* Invert compare instruction and assign this instruction's definition */
   cmp->opcode = new_opcode;
   ctx.info[instr->definitions[0].tempId()] = ctx.info[cmp->definitions[0].tempId()];
            ctx.uses[instr->operands[0].tempId()]--;
      }
      /* op1(op2(1, 2), 0) if swap = false
   * op1(0, op2(1, 2)) if swap = true */
   bool
   match_op3_for_vop3(opt_ctx& ctx, aco_opcode op1, aco_opcode op2, Instruction* op1_instr, bool swap,
                     {
      /* checks */
   if (op1_instr->opcode != op1)
            Instruction* op2_instr = follow_operand(ctx, op1_instr->operands[swap]);
   if (!op2_instr || op2_instr->opcode != op2)
            VALU_instruction* op1_valu = op1_instr->isVALU() ? &op1_instr->valu() : NULL;
            if (op1_instr->isSDWA() || op2_instr->isSDWA())
         if (op1_instr->isDPP() || op2_instr->isDPP())
            /* don't support inbetween clamp/omod */
   if (op2_valu && (op2_valu->clamp || op2_valu->omod))
            /* get operands and modifiers and check inbetween modifiers */
   *op1_clamp = op1_valu ? (bool)op1_valu->clamp : false;
            if (inbetween_neg)
         else if (op1_valu && op1_valu->neg[swap])
            if (inbetween_abs)
         else if (op1_valu && op1_valu->abs[swap])
            if (inbetween_opsel)
         else if (op1_valu && op1_valu->opsel[swap])
                     int shuffle[3];
   shuffle[shuffle_str[0] - '0'] = 0;
   shuffle[shuffle_str[1] - '0'] = 1;
            operands[shuffle[0]] = op1_instr->operands[!swap];
   neg[shuffle[0]] = op1_valu ? op1_valu->neg[!swap] : false;
   abs[shuffle[0]] = op1_valu ? op1_valu->abs[!swap] : false;
            for (unsigned i = 0; i < 2; i++) {
      operands[shuffle[i + 1]] = op2_instr->operands[i];
   neg[shuffle[i + 1]] = op2_valu ? op2_valu->neg[i] : false;
   abs[shuffle[i + 1]] = op2_valu ? op2_valu->abs[i] : false;
               /* check operands */
   if (!check_vop3_operands(ctx, 3, operands))
               }
      void
   create_vop3_for_op3(opt_ctx& ctx, aco_opcode opcode, aco_ptr<Instruction>& instr,
               {
      VALU_instruction* new_instr = create_instruction<VALU_instruction>(opcode, Format::VOP3, 3, 1);
   new_instr->neg = neg;
   new_instr->abs = abs;
   new_instr->clamp = clamp;
   new_instr->omod = omod;
   new_instr->opsel = opsel;
   new_instr->operands[0] = operands[0];
   new_instr->operands[1] = operands[1];
   new_instr->operands[2] = operands[2];
   new_instr->definitions[0] = instr->definitions[0];
   new_instr->pass_flags = instr->pass_flags;
               }
      bool
   combine_three_valu_op(opt_ctx& ctx, aco_ptr<Instruction>& instr, aco_opcode op2, aco_opcode new_op,
         {
      for (unsigned swap = 0; swap < 2; swap++) {
      if (!((1 << swap) & ops))
            Operand operands[3];
   bool clamp, precise;
   bitarray8 neg = 0, abs = 0, opsel = 0;
   uint8_t omod = 0;
   if (match_op3_for_vop3(ctx, instr->opcode, op2, instr.get(), swap, shuffle, operands, neg,
            ctx.uses[instr->operands[swap].tempId()]--;
   create_vop3_for_op3(ctx, new_op, instr, operands, neg, abs, opsel, clamp, omod);
         }
      }
      /* creates v_lshl_add_u32, v_lshl_or_b32 or v_and_or_b32 */
   bool
   combine_add_or_then_and_lshl(opt_ctx& ctx, aco_ptr<Instruction>& instr)
   {
      bool is_or = instr->opcode == aco_opcode::v_or_b32;
            if (is_or && combine_three_valu_op(ctx, instr, aco_opcode::s_and_b32, aco_opcode::v_and_or_b32,
               if (is_or && combine_three_valu_op(ctx, instr, aco_opcode::v_and_b32, aco_opcode::v_and_or_b32,
               if (combine_three_valu_op(ctx, instr, aco_opcode::s_lshl_b32, new_op_lshl, "120", 1 | 2))
         if (combine_three_valu_op(ctx, instr, aco_opcode::v_lshlrev_b32, new_op_lshl, "210", 1 | 2))
            if (instr->isSDWA() || instr->isDPP())
            /* v_or_b32(p_extract(a, 0, 8/16, 0), b) -> v_and_or_b32(a, 0xff/0xffff, b)
   * v_or_b32(p_insert(a, 0, 8/16), b) -> v_and_or_b32(a, 0xff/0xffff, b)
   * v_or_b32(p_insert(a, 24/16, 8/16), b) -> v_lshl_or_b32(a, 24/16, b)
   * v_add_u32(p_insert(a, 24/16, 8/16), b) -> v_lshl_add_b32(a, 24/16, b)
   */
   for (unsigned i = 0; i < 2; i++) {
      Instruction* extins = follow_operand(ctx, instr->operands[i]);
   if (!extins)
            aco_opcode op;
            if (extins->opcode == aco_opcode::p_insert &&
      (extins->operands[1].constantValue() + 1) * extins->operands[2].constantValue() == 32) {
   op = new_op_lshl;
   operands[1] =
      } else if (is_or &&
            (extins->opcode == aco_opcode::p_insert ||
      (extins->opcode == aco_opcode::p_extract &&
      op = aco_opcode::v_and_or_b32;
      } else {
                  operands[0] = extins->operands[0];
            if (!check_vop3_operands(ctx, 3, operands))
            uint8_t neg = 0, abs = 0, opsel = 0, omod = 0;
   bool clamp = false;
   if (instr->isVOP3())
            ctx.uses[instr->operands[i].tempId()]--;
   create_vop3_for_op3(ctx, op, instr, operands, neg, abs, opsel, clamp, omod);
                  }
      /* v_xor(a, s_not(b)) -> v_xnor(a, b)
   * v_xor(a, v_not(b)) -> v_xnor(a, b)
   */
   bool
   combine_xor_not(opt_ctx& ctx, aco_ptr<Instruction>& instr)
   {
      if (instr->usesModifiers())
            for (unsigned i = 0; i < 2; i++) {
      Instruction* op_instr = follow_operand(ctx, instr->operands[i], true);
   if (!op_instr ||
      (op_instr->opcode != aco_opcode::v_not_b32 &&
   op_instr->opcode != aco_opcode::s_not_b32) ||
               instr->opcode = aco_opcode::v_xnor_b32;
   instr->operands[i] = copy_operand(ctx, op_instr->operands[0]);
   decrease_uses(ctx, op_instr);
   if (instr->operands[0].isOfType(RegType::vgpr))
         if (!instr->operands[1].isOfType(RegType::vgpr))
                           }
      /* v_not(v_xor(a, b)) -> v_xnor(a, b) */
   bool
   combine_not_xor(opt_ctx& ctx, aco_ptr<Instruction>& instr)
   {
      if (instr->usesModifiers())
            Instruction* op_instr = follow_operand(ctx, instr->operands[0]);
   if (!op_instr || op_instr->opcode != aco_opcode::v_xor_b32 || op_instr->isSDWA())
            ctx.uses[instr->operands[0].tempId()]--;
   std::swap(instr->definitions[0], op_instr->definitions[0]);
               }
      bool
   combine_minmax(opt_ctx& ctx, aco_ptr<Instruction>& instr, aco_opcode opposite, aco_opcode op3src,
         {
               /* min(min(a, b), c) -> min3(a, b, c)
   * max(max(a, b), c) -> max3(a, b, c)
   * gfx11: min(-min(a, b), c) -> maxmin(-a, -b, c)
   * gfx11: max(-max(a, b), c) -> minmax(-a, -b, c)
   */
   for (unsigned swap = 0; swap < 2; swap++) {
      Operand operands[3];
   bool clamp, precise;
   bitarray8 opsel = 0, neg = 0, abs = 0;
   uint8_t omod = 0;
   bool inbetween_neg;
   if (match_op3_for_vop3(ctx, instr->opcode, instr->opcode, instr.get(), swap, "120", operands,
                  (!inbetween_neg ||
   (minmax != aco_opcode::num_opcodes && ctx.program->gfx_level >= GFX11))) {
   ctx.uses[instr->operands[swap].tempId()]--;
   if (inbetween_neg) {
      neg[0] = !neg[0];
   neg[1] = !neg[1];
      } else {
         }
                  /* min(-max(a, b), c) -> min3(-a, -b, c)
   * max(-min(a, b), c) -> max3(-a, -b, c)
   * gfx11: min(max(a, b), c) -> maxmin(a, b, c)
   * gfx11: max(min(a, b), c) -> minmax(a, b, c)
   */
   for (unsigned swap = 0; swap < 2; swap++) {
      Operand operands[3];
   bool clamp, precise;
   bitarray8 opsel = 0, neg = 0, abs = 0;
   uint8_t omod = 0;
   bool inbetween_neg;
   if (match_op3_for_vop3(ctx, instr->opcode, opposite, instr.get(), swap, "120", operands, neg,
            (inbetween_neg ||
   (minmax != aco_opcode::num_opcodes && ctx.program->gfx_level >= GFX11))) {
   ctx.uses[instr->operands[swap].tempId()]--;
   if (inbetween_neg) {
      neg[0] = !neg[0];
   neg[1] = !neg[1];
      } else {
         }
         }
      }
      /* s_not_b32(s_and_b32(a, b)) -> s_nand_b32(a, b)
   * s_not_b32(s_or_b32(a, b)) -> s_nor_b32(a, b)
   * s_not_b32(s_xor_b32(a, b)) -> s_xnor_b32(a, b)
   * s_not_b64(s_and_b64(a, b)) -> s_nand_b64(a, b)
   * s_not_b64(s_or_b64(a, b)) -> s_nor_b64(a, b)
   * s_not_b64(s_xor_b64(a, b)) -> s_xnor_b64(a, b) */
   bool
   combine_salu_not_bitwise(opt_ctx& ctx, aco_ptr<Instruction>& instr)
   {
      /* checks */
   if (!instr->operands[0].isTemp())
         if (instr->definitions[1].isTemp() && ctx.uses[instr->definitions[1].tempId()])
            Instruction* op2_instr = follow_operand(ctx, instr->operands[0]);
   if (!op2_instr)
         switch (op2_instr->opcode) {
   case aco_opcode::s_and_b32:
   case aco_opcode::s_or_b32:
   case aco_opcode::s_xor_b32:
   case aco_opcode::s_and_b64:
   case aco_opcode::s_or_b64:
   case aco_opcode::s_xor_b64: break;
   default: return false;
            /* create instruction */
   std::swap(instr->definitions[0], op2_instr->definitions[0]);
   std::swap(instr->definitions[1], op2_instr->definitions[1]);
   ctx.uses[instr->operands[0].tempId()]--;
            switch (op2_instr->opcode) {
   case aco_opcode::s_and_b32: op2_instr->opcode = aco_opcode::s_nand_b32; break;
   case aco_opcode::s_or_b32: op2_instr->opcode = aco_opcode::s_nor_b32; break;
   case aco_opcode::s_xor_b32: op2_instr->opcode = aco_opcode::s_xnor_b32; break;
   case aco_opcode::s_and_b64: op2_instr->opcode = aco_opcode::s_nand_b64; break;
   case aco_opcode::s_or_b64: op2_instr->opcode = aco_opcode::s_nor_b64; break;
   case aco_opcode::s_xor_b64: op2_instr->opcode = aco_opcode::s_xnor_b64; break;
   default: break;
               }
      /* s_and_b32(a, s_not_b32(b)) -> s_andn2_b32(a, b)
   * s_or_b32(a, s_not_b32(b)) -> s_orn2_b32(a, b)
   * s_and_b64(a, s_not_b64(b)) -> s_andn2_b64(a, b)
   * s_or_b64(a, s_not_b64(b)) -> s_orn2_b64(a, b) */
   bool
   combine_salu_n2(opt_ctx& ctx, aco_ptr<Instruction>& instr)
   {
      if (instr->definitions[0].isTemp() && ctx.info[instr->definitions[0].tempId()].is_uniform_bool())
            for (unsigned i = 0; i < 2; i++) {
      Instruction* op2_instr = follow_operand(ctx, instr->operands[i]);
   if (!op2_instr || (op2_instr->opcode != aco_opcode::s_not_b32 &&
               if (ctx.uses[op2_instr->definitions[1].tempId()])
            if (instr->operands[!i].isLiteral() && op2_instr->operands[0].isLiteral() &&
                  ctx.uses[instr->operands[i].tempId()]--;
   instr->operands[0] = instr->operands[!i];
   instr->operands[1] = op2_instr->operands[0];
            switch (instr->opcode) {
   case aco_opcode::s_and_b32: instr->opcode = aco_opcode::s_andn2_b32; break;
   case aco_opcode::s_or_b32: instr->opcode = aco_opcode::s_orn2_b32; break;
   case aco_opcode::s_and_b64: instr->opcode = aco_opcode::s_andn2_b64; break;
   case aco_opcode::s_or_b64: instr->opcode = aco_opcode::s_orn2_b64; break;
   default: break;
               }
      }
      /* s_add_{i32,u32}(a, s_lshl_b32(b, <n>)) -> s_lshl<n>_add_u32(a, b) */
   bool
   combine_salu_lshl_add(opt_ctx& ctx, aco_ptr<Instruction>& instr)
   {
      if (instr->opcode == aco_opcode::s_add_i32 && ctx.uses[instr->definitions[1].tempId()])
            for (unsigned i = 0; i < 2; i++) {
      Instruction* op2_instr = follow_operand(ctx, instr->operands[i], true);
   if (!op2_instr || op2_instr->opcode != aco_opcode::s_lshl_b32 ||
      ctx.uses[op2_instr->definitions[1].tempId()])
      if (!op2_instr->operands[1].isConstant())
            uint32_t shift = op2_instr->operands[1].constantValue();
   if (shift < 1 || shift > 4)
            if (instr->operands[!i].isLiteral() && op2_instr->operands[0].isLiteral() &&
                  instr->operands[1] = instr->operands[!i];
   instr->operands[0] = copy_operand(ctx, op2_instr->operands[0]);
   decrease_uses(ctx, op2_instr);
            instr->opcode = std::array<aco_opcode, 4>{
                     }
      }
      /* s_abs_i32(s_sub_[iu]32(a, b)) -> s_absdiff_i32(a, b)
   * s_abs_i32(s_add_[iu]32(a, #b)) -> s_absdiff_i32(a, -b)
   */
   bool
   combine_sabsdiff(opt_ctx& ctx, aco_ptr<Instruction>& instr)
   {
      if (!instr->operands[0].isTemp() || !ctx.info[instr->operands[0].tempId()].is_add_sub())
            Instruction* op_instr = follow_operand(ctx, instr->operands[0], false);
   if (!op_instr)
            if (op_instr->opcode == aco_opcode::s_add_i32 || op_instr->opcode == aco_opcode::s_add_u32) {
      for (unsigned i = 0; i < 2; i++) {
      uint64_t constant;
   if (op_instr->operands[!i].isLiteral() ||
                  if (op_instr->operands[i].isTemp())
         op_instr->operands[0] = op_instr->operands[!i];
   op_instr->operands[1] = Operand::c32(-int32_t(constant));
      }
            use_absdiff:
      op_instr->opcode = aco_opcode::s_absdiff_i32;
   std::swap(instr->definitions[0], op_instr->definitions[0]);
   std::swap(instr->definitions[1], op_instr->definitions[1]);
               }
      bool
   combine_add_sub_b2i(opt_ctx& ctx, aco_ptr<Instruction>& instr, aco_opcode new_op, uint8_t ops)
   {
      if (instr->usesModifiers())
            for (unsigned i = 0; i < 2; i++) {
      if (!((1 << i) & ops))
         if (instr->operands[i].isTemp() && ctx.info[instr->operands[i].tempId()].is_b2i() &&
               aco_ptr<Instruction> new_instr;
   if (instr->operands[!i].isTemp() &&
      instr->operands[!i].getTemp().type() == RegType::vgpr) {
      } else if (ctx.program->gfx_level >= GFX10 ||
            new_instr.reset(
      } else {
         }
   ctx.uses[instr->operands[i].tempId()]--;
   new_instr->definitions[0] = instr->definitions[0];
   if (instr->definitions.size() == 2) {
         } else {
      new_instr->definitions[1] =
         /* Make sure the uses vector is large enough and the number of
   * uses properly initialized to 0.
   */
      }
   new_instr->operands[0] = Operand::zero();
   new_instr->operands[1] = instr->operands[!i];
   new_instr->operands[2] = Operand(ctx.info[instr->operands[i].tempId()].temp);
   new_instr->pass_flags = instr->pass_flags;
   instr = std::move(new_instr);
   ctx.info[instr->definitions[0].tempId()].set_add_sub(instr.get());
                     }
      bool
   combine_add_bcnt(opt_ctx& ctx, aco_ptr<Instruction>& instr)
   {
      if (instr->usesModifiers())
            for (unsigned i = 0; i < 2; i++) {
      Instruction* op_instr = follow_operand(ctx, instr->operands[i]);
   if (op_instr && op_instr->opcode == aco_opcode::v_bcnt_u32_b32 &&
      !op_instr->usesModifiers() && op_instr->operands[0].isTemp() &&
   op_instr->operands[0].getTemp().type() == RegType::vgpr &&
   op_instr->operands[1].constantEquals(0)) {
   aco_ptr<Instruction> new_instr{
         ctx.uses[instr->operands[i].tempId()]--;
   new_instr->operands[0] = op_instr->operands[0];
   new_instr->operands[1] = instr->operands[!i];
   new_instr->definitions[0] = instr->definitions[0];
   new_instr->pass_flags = instr->pass_flags;
                                    }
      bool
   get_minmax_info(aco_opcode op, aco_opcode* min, aco_opcode* max, aco_opcode* min3, aco_opcode* max3,
         {
         #define MINMAX(type, gfx9)                                                                         \
      case aco_opcode::v_min_##type:                                                                  \
   case aco_opcode::v_max_##type:                                                                  \
      *min = aco_opcode::v_min_##type;                                                             \
   *max = aco_opcode::v_max_##type;                                                             \
   *med3 = aco_opcode::v_med3_##type;                                                           \
   *min3 = aco_opcode::v_min3_##type;                                                           \
   *max3 = aco_opcode::v_max3_##type;                                                           \
   *minmax = op == *min ? aco_opcode::v_maxmin_##type : aco_opcode::v_minmax_##type;            \
   *some_gfx9_only = gfx9;                                                                      \
   #define MINMAX_INT16(type, gfx9)                                                                   \
      case aco_opcode::v_min_##type:                                                                  \
   case aco_opcode::v_max_##type:                                                                  \
      *min = aco_opcode::v_min_##type;                                                             \
   *max = aco_opcode::v_max_##type;                                                             \
   *med3 = aco_opcode::v_med3_##type;                                                           \
   *min3 = aco_opcode::v_min3_##type;                                                           \
   *max3 = aco_opcode::v_max3_##type;                                                           \
   *minmax = aco_opcode::num_opcodes;                                                           \
   *some_gfx9_only = gfx9;                                                                      \
   #define MINMAX_INT16_E64(type, gfx9)                                                               \
      case aco_opcode::v_min_##type##_e64:                                                            \
   case aco_opcode::v_max_##type##_e64:                                                            \
      *min = aco_opcode::v_min_##type##_e64;                                                       \
   *max = aco_opcode::v_max_##type##_e64;                                                       \
   *med3 = aco_opcode::v_med3_##type;                                                           \
   *min3 = aco_opcode::v_min3_##type;                                                           \
   *max3 = aco_opcode::v_max3_##type;                                                           \
   *minmax = aco_opcode::num_opcodes;                                                           \
   *some_gfx9_only = gfx9;                                                                      \
   return true;
   MINMAX(f32, false)
   MINMAX(u32, false)
   MINMAX(i32, false)
   MINMAX(f16, true)
   MINMAX_INT16(u16, true)
   MINMAX_INT16(i16, true)
   MINMAX_INT16_E64(u16, true)
   #undef MINMAX_INT16_E64
   #undef MINMAX_INT16
   #undef MINMAX
      default: return false;
      }
      /* when ub > lb:
   * v_min_{f,u,i}{16,32}(v_max_{f,u,i}{16,32}(a, lb), ub) -> v_med3_{f,u,i}{16,32}(a, lb, ub)
   * v_max_{f,u,i}{16,32}(v_min_{f,u,i}{16,32}(a, ub), lb) -> v_med3_{f,u,i}{16,32}(a, lb, ub)
   */
   bool
   combine_clamp(opt_ctx& ctx, aco_ptr<Instruction>& instr, aco_opcode min, aco_opcode max,
         {
      /* TODO: GLSL's clamp(x, minVal, maxVal) and SPIR-V's
   * FClamp(x, minVal, maxVal)/NClamp(x, minVal, maxVal) are undefined if
   * minVal > maxVal, which means we can always select it to a v_med3_f32 */
   aco_opcode other_op;
   if (instr->opcode == min)
         else if (instr->opcode == max)
         else
            for (unsigned swap = 0; swap < 2; swap++) {
      Operand operands[3];
   bool clamp, precise;
   bitarray8 opsel = 0, neg = 0, abs = 0;
   uint8_t omod = 0;
   if (match_op3_for_vop3(ctx, instr->opcode, other_op, instr.get(), swap, "012", operands, neg,
            /* max(min(src, upper), lower) returns upper if src is NaN, but
   * med3(src, lower, upper) returns lower.
   */
   if (precise && instr->opcode != min &&
                  int const0_idx = -1, const1_idx = -1;
   uint32_t const0 = 0, const1 = 0;
   for (int i = 0; i < 3; i++) {
      uint32_t val;
   bool hi16 = opsel & (1 << i);
   if (operands[i].isConstant()) {
         } else if (operands[i].isTemp() &&
               } else {
         }
   if (const0_idx >= 0) {
      const1_idx = i;
      } else {
      const0_idx = i;
         }
                  int lower_idx = const0_idx;
   switch (min) {
   case aco_opcode::v_min_f32:
   case aco_opcode::v_min_f16: {
      float const0_f, const1_f;
   if (min == aco_opcode::v_min_f32) {
      memcpy(&const0_f, &const0, 4);
      } else {
      const0_f = _mesa_half_to_float(const0);
      }
   if (abs[const0_idx])
         if (abs[const1_idx])
         if (neg[const0_idx])
         if (neg[const1_idx])
         lower_idx = const0_f < const1_f ? const0_idx : const1_idx;
      }
   case aco_opcode::v_min_u32: {
      lower_idx = const0 < const1 ? const0_idx : const1_idx;
      }
   case aco_opcode::v_min_u16:
   case aco_opcode::v_min_u16_e64: {
      lower_idx = (uint16_t)const0 < (uint16_t)const1 ? const0_idx : const1_idx;
      }
   case aco_opcode::v_min_i32: {
      int32_t const0_i =
         int32_t const1_i =
         lower_idx = const0_i < const1_i ? const0_idx : const1_idx;
      }
   case aco_opcode::v_min_i16:
   case aco_opcode::v_min_i16_e64: {
      int16_t const0_i = const0 & 0x8000u ? -32768 + (int16_t)(const0 & 0x7fffu) : const0;
   int16_t const1_i = const1 & 0x8000u ? -32768 + (int16_t)(const1 & 0x7fffu) : const1;
   lower_idx = const0_i < const1_i ? const0_idx : const1_idx;
      }
   default: break;
                  if (instr->opcode == min) {
      if (upper_idx != 0 || lower_idx == 0)
      } else {
      if (upper_idx == 0 || lower_idx != 0)
                                                }
      void
   apply_sgprs(opt_ctx& ctx, aco_ptr<Instruction>& instr)
   {
      bool is_shift64 = instr->opcode == aco_opcode::v_lshlrev_b64 ||
                  /* find candidates and create the set of sgprs already read */
   unsigned sgpr_ids[2] = {0, 0};
   uint32_t operand_mask = 0;
   bool has_literal = false;
   for (unsigned i = 0; i < instr->operands.size(); i++) {
      if (instr->operands[i].isLiteral())
         if (!instr->operands[i].isTemp())
         if (instr->operands[i].getTemp().type() == RegType::sgpr) {
      if (instr->operands[i].tempId() != sgpr_ids[0])
      }
   ssa_info& info = ctx.info[instr->operands[i].tempId()];
   if (is_copy_label(ctx, instr, info, i) && info.temp.type() == RegType::sgpr)
         if (info.is_extract() && info.instr->operands[0].getTemp().type() == RegType::sgpr)
      }
   unsigned max_sgprs = 1;
   if (ctx.program->gfx_level >= GFX10 && !is_shift64)
         if (has_literal)
                     /* keep on applying sgprs until there is nothing left to be done */
   while (operand_mask) {
      uint32_t sgpr_idx = 0;
   uint32_t sgpr_info_id = 0;
   uint32_t mask = operand_mask;
   /* choose a sgpr */
   while (mask) {
      unsigned i = u_bit_scan(&mask);
   uint16_t uses = ctx.uses[instr->operands[i].tempId()];
   if (sgpr_info_id == 0 || uses < ctx.uses[sgpr_info_id]) {
      sgpr_idx = i;
         }
                     /* Applying two sgprs require making it VOP3, so don't do it unless it's
   * definitively beneficial.
   * TODO: this is too conservative because later the use count could be reduced to 1 */
   if (!info.is_extract() && num_sgprs && ctx.uses[sgpr_info_id] > 1 && !instr->isVOP3() &&
                  Temp sgpr = info.is_extract() ? info.instr->operands[0].getTemp() : info.temp;
   bool new_sgpr = sgpr.id() != sgpr_ids[0] && sgpr.id() != sgpr_ids[1];
   if (new_sgpr && num_sgprs >= max_sgprs)
            if (sgpr_idx == 0)
            if (sgpr_idx == 1 && instr->isDPP())
            if (sgpr_idx == 0 || instr->isVOP3() || instr->isSDWA() || instr->isVOP3P() ||
      info.is_extract()) {
   /* can_apply_extract() checks SGPR encoding restrictions */
   if (info.is_extract() && can_apply_extract(ctx, instr, sgpr_idx, info))
         else if (info.is_extract())
            } else if (can_swap_operands(instr, &instr->opcode) && !instr->valu().opsel[sgpr_idx]) {
      instr->operands[sgpr_idx] = instr->operands[0];
   instr->operands[0] = Operand(sgpr);
   instr->valu().opsel[0].swap(instr->valu().opsel[sgpr_idx]);
   /* swap bits using a 4-entry LUT */
   uint32_t swapped = (0x3120 >> (operand_mask & 0x3)) & 0xf;
      } else if (can_use_VOP3(ctx, instr) && !info.is_extract()) {
      instr->format = asVOP3(instr->format);
      } else {
                  if (new_sgpr)
         ctx.uses[sgpr_info_id]--;
            /* TODO: handle when it's a VGPR */
   if ((ctx.info[sgpr.id()].label & (label_extract | label_temp)) &&
      ctx.info[sgpr.id()].temp.type() == RegType::sgpr)
      }
      void
   interp_p2_f32_inreg_to_fma_dpp(aco_ptr<Instruction>& instr)
   {
      static_assert(sizeof(DPP16_instruction) == sizeof(VINTERP_inreg_instruction),
         instr->format = asVOP3(Format::DPP16);
   instr->opcode = aco_opcode::v_fma_f32;
   instr->dpp16().dpp_ctrl = dpp_quad_perm(2, 2, 2, 2);
   instr->dpp16().row_mask = 0xf;
   instr->dpp16().bank_mask = 0xf;
   instr->dpp16().bound_ctrl = 0;
      }
      /* apply omod / clamp modifiers if the def is used only once and the instruction can have modifiers */
   bool
   apply_omod_clamp(opt_ctx& ctx, aco_ptr<Instruction>& instr)
   {
      if (instr->definitions.empty() || ctx.uses[instr->definitions[0].tempId()] != 1 ||
      !instr_info.can_use_output_modifiers[(int)instr->opcode])
         bool can_vop3 = can_use_VOP3(ctx, instr);
   bool is_mad_mix =
         bool needs_vop3 = !instr->isSDWA() && !instr->isVINTERP_INREG() && !is_mad_mix;
   if (needs_vop3 && !can_vop3)
            /* SDWA omod is GFX9+. */
   bool can_use_omod =
      (can_vop3 || ctx.program->gfx_level >= GFX9) && !instr->isVOP3P() &&
                  uint64_t omod_labels = label_omod2 | label_omod4 | label_omod5;
   if (!def_info.is_clamp() && !(can_use_omod && (def_info.label & omod_labels)))
         /* if the omod/clamp instruction is dead, then the single user of this
   * instruction is a different instruction */
   if (!ctx.uses[def_info.instr->definitions[0].tempId()])
            if (def_info.instr->definitions[0].bytes() != instr->definitions[0].bytes())
            /* MADs/FMAs are created later, so we don't have to update the original add */
            if (!def_info.is_clamp() && (instr->valu().clamp || instr->valu().omod))
            if (needs_vop3)
            if (!def_info.is_clamp() && instr->opcode == aco_opcode::v_interp_p2_f32_inreg)
            if (def_info.is_omod2())
         else if (def_info.is_omod4())
         else if (def_info.is_omod5())
         else if (def_info.is_clamp())
            instr->definitions[0].swapTemp(def_info.instr->definitions[0]);
   ctx.info[instr->definitions[0].tempId()].label &= label_clamp | label_insert | label_f2f16;
               }
      /* Combine an p_insert (or p_extract, in some cases) instruction with instr.
   * p_insert(instr(...)) -> instr_insert().
   */
   bool
   apply_insert(opt_ctx& ctx, aco_ptr<Instruction>& instr)
   {
      if (instr->definitions.empty() || ctx.uses[instr->definitions[0].tempId()] != 1)
            ssa_info& def_info = ctx.info[instr->definitions[0].tempId()];
   if (!def_info.is_insert())
         /* if the insert instruction is dead, then the single user of this
   * instruction is a different instruction */
   if (!ctx.uses[def_info.instr->definitions[0].tempId()])
            /* MADs/FMAs are created later, so we don't have to update the original add */
            SubdwordSel sel = parse_insert(def_info.instr);
            if (!can_use_SDWA(ctx.program->gfx_level, instr, true))
            convert_to_SDWA(ctx.program->gfx_level, instr);
   if (instr->sdwa().dst_sel.size() != 4)
                  instr->definitions[0].swapTemp(def_info.instr->definitions[0]);
   ctx.info[instr->definitions[0].tempId()].label = 0;
               }
      /* Remove superfluous extract after ds_read like so:
   * p_extract(ds_read_uN(), 0, N, 0) -> ds_read_uN()
   */
   bool
   apply_ds_extract(opt_ctx& ctx, aco_ptr<Instruction>& extract)
   {
      /* Check if p_extract has a usedef operand and is the only user. */
   if (!ctx.info[extract->operands[0].tempId()].is_usedef() ||
      ctx.uses[extract->operands[0].tempId()] > 1)
         /* Check if the usedef is a DS instruction. */
   Instruction* ds = ctx.info[extract->operands[0].tempId()].instr;
   if (ds->format != Format::DS)
            unsigned extract_idx = extract->operands[1].constantValue();
   unsigned bits_extracted = extract->operands[2].constantValue();
   unsigned sign_ext = extract->operands[3].constantValue();
            /* TODO: These are doable, but probably don't occur too often. */
   if (extract_idx || sign_ext || dst_bitsize != 32)
            unsigned bits_loaded = 0;
   if (ds->opcode == aco_opcode::ds_read_u8 || ds->opcode == aco_opcode::ds_read_u8_d16)
         else if (ds->opcode == aco_opcode::ds_read_u16 || ds->opcode == aco_opcode::ds_read_u16_d16)
         else
            /* Shrink the DS load if the extracted bit size is smaller. */
            /* Change the DS opcode so it writes the full register. */
   if (bits_loaded == 8)
         else if (bits_loaded == 16)
         else
            /* The DS now produces the exact same thing as the extract, remove the extract. */
   std::swap(ds->definitions[0], extract->definitions[0]);
   ctx.uses[extract->definitions[0].tempId()] = 0;
   ctx.info[ds->definitions[0].tempId()].label = 0;
      }
      /* v_and(a, v_subbrev_co(0, 0, vcc)) -> v_cndmask(0, a, vcc) */
   bool
   combine_and_subbrev(opt_ctx& ctx, aco_ptr<Instruction>& instr)
   {
      if (instr->usesModifiers())
            for (unsigned i = 0; i < 2; i++) {
      Instruction* op_instr = follow_operand(ctx, instr->operands[i], true);
   if (op_instr && op_instr->opcode == aco_opcode::v_subbrev_co_u32 &&
                     aco_ptr<Instruction> new_instr;
   if (instr->operands[!i].isTemp() &&
      instr->operands[!i].getTemp().type() == RegType::vgpr) {
   new_instr.reset(
      } else if (ctx.program->gfx_level >= GFX10 ||
            new_instr.reset(create_instruction<VALU_instruction>(aco_opcode::v_cndmask_b32,
      } else {
                  new_instr->operands[0] = Operand::zero();
   new_instr->operands[1] = instr->operands[!i];
   new_instr->operands[2] = copy_operand(ctx, op_instr->operands[2]);
   new_instr->definitions[0] = instr->definitions[0];
   new_instr->pass_flags = instr->pass_flags;
   instr = std::move(new_instr);
   decrease_uses(ctx, op_instr);
   ctx.info[instr->definitions[0].tempId()].label = 0;
                     }
      /* v_and(a, not(b)) -> v_bfi_b32(b, 0, a)
   * v_or(a, not(b)) -> v_bfi_b32(b, a, -1)
   */
   bool
   combine_v_andor_not(opt_ctx& ctx, aco_ptr<Instruction>& instr)
   {
      if (instr->usesModifiers())
            for (unsigned i = 0; i < 2; i++) {
      Instruction* op_instr = follow_operand(ctx, instr->operands[i], true);
   if (op_instr && !op_instr->usesModifiers() &&
                     Operand ops[3] = {
      op_instr->operands[0],
   Operand::zero(),
      };
   if (instr->opcode == aco_opcode::v_or_b32) {
      ops[1] = instr->operands[!i];
      }
                                 if (op_instr->operands[0].isTemp())
         for (unsigned j = 0; j < 3; j++)
         new_instr->definitions[0] = instr->definitions[0];
   new_instr->pass_flags = instr->pass_flags;
   instr.reset(new_instr);
   decrease_uses(ctx, op_instr);
   ctx.info[instr->definitions[0].tempId()].label = 0;
                     }
      /* v_add_co(c, s_lshl(a, b)) -> v_mad_u32_u24(a, 1<<b, c)
   * v_add_co(c, v_lshlrev(a, b)) -> v_mad_u32_u24(b, 1<<a, c)
   * v_sub(c, s_lshl(a, b)) -> v_mad_i32_i24(a, -(1<<b), c)
   * v_sub(c, v_lshlrev(a, b)) -> v_mad_i32_i24(b, -(1<<a), c)
   */
   bool
   combine_add_lshl(opt_ctx& ctx, aco_ptr<Instruction>& instr, bool is_sub)
   {
      if (instr->usesModifiers())
            /* Substractions: start at operand 1 to avoid mixup such as
   * turning v_sub(v_lshlrev(a, b), c) into v_mad_i32_i24(b, -(1<<a), c)
   */
            /* Don't allow 24-bit operands on subtraction because
   * v_mad_i32_i24 applies a sign extension.
   */
            for (unsigned i = start_op_idx; i < 2; i++) {
      Instruction* op_instr = follow_operand(ctx, instr->operands[i]);
   if (!op_instr)
            if (op_instr->opcode != aco_opcode::s_lshl_b32 &&
                           if (op_instr->operands[shift_op_idx].isConstant() &&
      ((allow_24bit && op_instr->operands[!shift_op_idx].is24bit()) ||
   op_instr->operands[!shift_op_idx].is16bit())) {
   uint32_t multiplier = 1 << (op_instr->operands[shift_op_idx].constantValue() % 32u);
   if (is_sub)
                        Operand ops[3] = {
      op_instr->operands[!shift_op_idx],
   Operand::c32(multiplier),
      };
                           aco_opcode mad_op = is_sub ? aco_opcode::v_mad_i32_i24 : aco_opcode::v_mad_u32_u24;
   aco_ptr<VALU_instruction> new_instr{
         for (unsigned op_idx = 0; op_idx < 3; ++op_idx)
         new_instr->definitions[0] = instr->definitions[0];
   new_instr->pass_flags = instr->pass_flags;
   instr = std::move(new_instr);
   ctx.info[instr->definitions[0].tempId()].label = 0;
                     }
      void
   propagate_swizzles(VALU_instruction* instr, bool opsel_lo, bool opsel_hi)
   {
      /* propagate swizzles which apply to a result down to the instruction's operands:
   * result = a.xy + b.xx -> result.yx = a.yx + b.xx */
   uint8_t tmp_lo = instr->opsel_lo;
   uint8_t tmp_hi = instr->opsel_hi;
   uint8_t neg_lo = instr->neg_lo;
   uint8_t neg_hi = instr->neg_hi;
   if (opsel_lo == 1) {
      instr->opsel_lo = tmp_hi;
      }
   if (opsel_hi == 0) {
      instr->opsel_hi = tmp_lo;
         }
      void
   combine_vop3p(opt_ctx& ctx, aco_ptr<Instruction>& instr)
   {
               /* apply clamp */
   if (instr->opcode == aco_opcode::v_pk_mul_f16 && instr->operands[1].constantEquals(0x3C00) &&
      vop3p->clamp && instr->operands[0].isTemp() && ctx.uses[instr->operands[0].tempId()] == 1 &&
            ssa_info& info = ctx.info[instr->operands[0].tempId()];
   if (info.is_vop3p() && instr_info.can_use_output_modifiers[(int)info.instr->opcode]) {
      VALU_instruction* candidate = &ctx.info[instr->operands[0].tempId()].instr->valu();
   candidate->clamp = true;
   propagate_swizzles(candidate, vop3p->opsel_lo[0], vop3p->opsel_hi[0]);
   instr->definitions[0].swapTemp(candidate->definitions[0]);
   ctx.info[candidate->definitions[0].tempId()].instr = candidate;
   ctx.uses[instr->definitions[0].tempId()]--;
                  /* check for fneg modifiers */
   for (unsigned i = 0; i < instr->operands.size(); i++) {
      if (!can_use_input_modifiers(ctx.program->gfx_level, instr->opcode, i))
         Operand& op = instr->operands[i];
   if (!op.isTemp())
            ssa_info& info = ctx.info[op.tempId()];
   if (info.is_vop3p() && info.instr->opcode == aco_opcode::v_pk_mul_f16 &&
                                       Operand ops[3];
   for (unsigned j = 0; j < instr->operands.size(); j++)
         ops[i] = info.instr->operands[0];
                  if (fneg->clamp)
                  /* opsel_lo/hi is either 0 or 1:
   * if 0 - pick selection from fneg->lo
   * if 1 - pick selection from fneg->hi
   */
   bool opsel_lo = vop3p->opsel_lo[i];
   bool opsel_hi = vop3p->opsel_hi[i];
   bool neg_lo = fneg->neg_lo[0] ^ fneg->neg_lo[1];
   bool neg_hi = fneg->neg_hi[0] ^ fneg->neg_hi[1];
   vop3p->neg_lo[i] ^= opsel_lo ? neg_hi : neg_lo;
   vop3p->neg_hi[i] ^= opsel_hi ? neg_hi : neg_lo;
                  if (--ctx.uses[fneg->definitions[0].tempId()])
                  if (instr->opcode == aco_opcode::v_pk_add_f16 || instr->opcode == aco_opcode::v_pk_add_u16) {
      bool fadd = instr->opcode == aco_opcode::v_pk_add_f16;
   if (fadd && instr->definitions[0].isPrecise())
            Instruction* mul_instr = nullptr;
   unsigned add_op_idx = 0;
   bitarray8 mul_neg_lo = 0, mul_neg_hi = 0, mul_opsel_lo = 0, mul_opsel_hi = 0;
            /* find the 'best' mul instruction to combine with the add */
   for (unsigned i = 0; i < 2; i++) {
      Instruction* op_instr = follow_operand(ctx, instr->operands[i], true);
                  if (ctx.info[instr->operands[i].tempId()].is_vop3p()) {
      if (fadd) {
      if (op_instr->opcode != aco_opcode::v_pk_mul_f16 ||
      op_instr->definitions[0].isPrecise())
   } else {
                        Operand op[3] = {op_instr->operands[0], op_instr->operands[1], instr->operands[1 - i]};
                  /* no clamp allowed between mul and add */
                  mul_instr = op_instr;
   add_op_idx = 1 - i;
   uses = ctx.uses[instr->operands[i].tempId()];
   mul_neg_lo = mul_instr->valu().neg_lo;
   mul_neg_hi = mul_instr->valu().neg_hi;
   mul_opsel_lo = mul_instr->valu().opsel_lo;
      } else if (instr->operands[i].bytes() == 2) {
      if ((fadd && (op_instr->opcode != aco_opcode::v_mul_f16 ||
                                             if (op_instr->isDPP() || (op_instr->isSDWA() && (op_instr->sdwa().sel[0].size() < 2 ||
                  Operand op[3] = {op_instr->operands[0], op_instr->operands[1], instr->operands[1 - i]};
                  mul_instr = op_instr;
   add_op_idx = 1 - i;
   uses = ctx.uses[instr->operands[i].tempId()];
   mul_neg_lo = mul_instr->valu().neg;
   mul_neg_hi = mul_instr->valu().neg;
   if (mul_instr->isSDWA()) {
      for (unsigned j = 0; j < 2; j++)
      } else {
         }
                  if (!mul_instr)
            /* turn mul + packed add into v_pk_fma_f16 */
   aco_opcode mad = fadd ? aco_opcode::v_pk_fma_f16 : aco_opcode::v_pk_mad_u16;
   aco_ptr<VALU_instruction> fma{create_instruction<VALU_instruction>(mad, Format::VOP3P, 3, 1)};
   fma->operands[0] = copy_operand(ctx, mul_instr->operands[0]);
   fma->operands[1] = copy_operand(ctx, mul_instr->operands[1]);
   fma->operands[2] = instr->operands[add_op_idx];
   fma->clamp = vop3p->clamp;
   fma->neg_lo = mul_neg_lo;
   fma->neg_hi = mul_neg_hi;
   fma->opsel_lo = mul_opsel_lo;
   fma->opsel_hi = mul_opsel_hi;
   propagate_swizzles(fma.get(), vop3p->opsel_lo[1 - add_op_idx],
         fma->opsel_lo[2] = vop3p->opsel_lo[add_op_idx];
   fma->opsel_hi[2] = vop3p->opsel_hi[add_op_idx];
   fma->neg_lo[2] = vop3p->neg_lo[add_op_idx];
   fma->neg_hi[2] = vop3p->neg_hi[add_op_idx];
   fma->neg_lo[1] = fma->neg_lo[1] ^ vop3p->neg_lo[1 - add_op_idx];
   fma->neg_hi[1] = fma->neg_hi[1] ^ vop3p->neg_hi[1 - add_op_idx];
   fma->definitions[0] = instr->definitions[0];
   fma->pass_flags = instr->pass_flags;
   instr = std::move(fma);
   ctx.info[instr->definitions[0].tempId()].set_vop3p(instr.get());
   decrease_uses(ctx, mul_instr);
         }
      bool
   can_use_mad_mix(opt_ctx& ctx, aco_ptr<Instruction>& instr)
   {
      if (ctx.program->gfx_level < GFX9)
            /* v_mad_mix* on GFX9 always flushes denormals for 16-bit inputs/outputs */
   if (ctx.program->gfx_level == GFX9 && ctx.fp_mode.denorm16_64)
            if (instr->valu().omod)
            switch (instr->opcode) {
   case aco_opcode::v_add_f32:
   case aco_opcode::v_sub_f32:
   case aco_opcode::v_subrev_f32:
   case aco_opcode::v_mul_f32: return !instr->isSDWA() && !instr->isDPP();
   case aco_opcode::v_fma_f32:
         case aco_opcode::v_fma_mix_f32:
   case aco_opcode::v_fma_mixlo_f16: return true;
   default: return false;
      }
      void
   to_mad_mix(opt_ctx& ctx, aco_ptr<Instruction>& instr)
   {
               if (instr->opcode == aco_opcode::v_fma_f32) {
      instr->format = (Format)((uint32_t)withoutVOP3(instr->format) | (uint32_t)(Format::VOP3P));
   instr->opcode = aco_opcode::v_fma_mix_f32;
                        aco_ptr<VALU_instruction> vop3p{
            for (unsigned i = 0; i < instr->operands.size(); i++) {
      vop3p->operands[is_add + i] = instr->operands[i];
   vop3p->neg_lo[is_add + i] = instr->valu().neg[i];
      }
   if (instr->opcode == aco_opcode::v_mul_f32) {
      vop3p->operands[2] = Operand::zero();
      } else if (is_add) {
      vop3p->operands[0] = Operand::c32(0x3f800000);
   if (instr->opcode == aco_opcode::v_sub_f32)
         else if (instr->opcode == aco_opcode::v_subrev_f32)
      }
   vop3p->definitions[0] = instr->definitions[0];
   vop3p->clamp = instr->valu().clamp;
   vop3p->pass_flags = instr->pass_flags;
            if (ctx.info[instr->definitions[0].tempId()].label & label_mul)
      }
      bool
   combine_output_conversion(opt_ctx& ctx, aco_ptr<Instruction>& instr)
   {
      ssa_info& def_info = ctx.info[instr->definitions[0].tempId()];
   if (!def_info.is_f2f16())
                  if (!ctx.uses[conv->definitions[0].tempId()] || ctx.uses[instr->definitions[0].tempId()] != 1)
            if (conv->usesModifiers())
            if (instr->opcode == aco_opcode::v_interp_p2_f32_inreg)
            if (!can_use_mad_mix(ctx, instr))
            if (!instr->isVOP3P())
            instr->opcode = aco_opcode::v_fma_mixlo_f16;
   instr->definitions[0].swapTemp(conv->definitions[0]);
   if (conv->definitions[0].isPrecise())
         ctx.info[instr->definitions[0].tempId()].label &= label_clamp;
               }
      void
   combine_mad_mix(opt_ctx& ctx, aco_ptr<Instruction>& instr)
   {
      if (!can_use_mad_mix(ctx, instr))
            for (unsigned i = 0; i < instr->operands.size(); i++) {
      if (!instr->operands[i].isTemp())
         Temp tmp = instr->operands[i].getTemp();
   if (!ctx.info[tmp.id()].is_f2f32())
            Instruction* conv = ctx.info[tmp.id()].instr;
   if (conv->valu().clamp || conv->valu().omod) {
         } else if (conv->isSDWA() &&
               } else if (conv->isDPP()) {
                  if (get_operand_size(instr, i) != 32)
            /* Conversion to VOP3P will add inline constant operands, but that shouldn't affect
   * check_vop3_operands(). */
   Operand op[3];
   for (unsigned j = 0; j < instr->operands.size(); j++)
         op[i] = conv->operands[0];
   if (!check_vop3_operands(ctx, instr->operands.size(), op))
         if (!conv->operands[0].isOfType(RegType::vgpr) && instr->isDPP())
            if (!instr->isVOP3P()) {
      bool is_add =
         to_mad_mix(ctx, instr);
               if (--ctx.uses[tmp.id()])
         instr->operands[i].setTemp(conv->operands[0].getTemp());
   if (conv->definitions[0].isPrecise())
         instr->valu().opsel_hi[i] = true;
   if (conv->isSDWA() && conv->sdwa().sel[0].offset() == 2)
         else
         bool neg = conv->valu().neg[0];
   bool abs = conv->valu().abs[0];
   if (!instr->valu().abs[i]) {
      instr->valu().neg[i] ^= neg;
            }
      // TODO: we could possibly move the whole label_instruction pass to combine_instruction:
   // this would mean that we'd have to fix the instruction uses while value propagation
      /* also returns true for inf */
   bool
   is_pow_of_two(opt_ctx& ctx, Operand op)
   {
      if (op.isTemp() && ctx.info[op.tempId()].is_constant_or_literal(op.bytes() * 8))
         else if (!op.isConstant())
                     if (op.bytes() == 4) {
      uint32_t exponent = (val & 0x7f800000) >> 23;
   uint32_t fraction = val & 0x007fffff;
      } else if (op.bytes() == 2) {
      uint32_t exponent = (val & 0x7c00) >> 10;
   uint32_t fraction = val & 0x03ff;
      } else {
      assert(op.bytes() == 8);
   uint64_t exponent = (val & UINT64_C(0x7ff0000000000000)) >> 52;
   uint64_t fraction = val & UINT64_C(0x000fffffffffffff);
         }
      void
   combine_instruction(opt_ctx& ctx, aco_ptr<Instruction>& instr)
   {
      if (instr->definitions.empty() || is_dead(ctx.uses, instr.get()))
            if (instr->isVALU()) {
      /* Apply SDWA. Do this after label_instruction() so it can remove
   * label_extract if not all instructions can take SDWA. */
   for (unsigned i = 0; i < instr->operands.size(); i++) {
      Operand& op = instr->operands[i];
   if (!op.isTemp())
         ssa_info& info = ctx.info[op.tempId()];
   if (!info.is_extract())
         /* if there are that many uses, there are likely better combinations */
   // TODO: delay applying extract to a point where we know better
   if (ctx.uses[op.tempId()] > 4) {
      info.label &= ~label_extract;
      }
   if (info.is_extract() &&
      (info.instr->operands[0].getTemp().type() == RegType::vgpr ||
   instr->operands[i].getTemp().type() == RegType::sgpr) &&
   can_apply_extract(ctx, instr, i, info)) {
   /* Increase use count of the extract's operand if the extract still has uses. */
   apply_extract(ctx, instr, i, info);
   if (--ctx.uses[instr->operands[i].tempId()])
                        if (can_apply_sgprs(ctx, instr))
         combine_mad_mix(ctx, instr);
   while (apply_omod_clamp(ctx, instr) || combine_output_conversion(ctx, instr))
                     if (instr->isVOP3P() && instr->opcode != aco_opcode::v_fma_mix_f32 &&
      instr->opcode != aco_opcode::v_fma_mixlo_f16)
         if (instr->isSDWA() || instr->isDPP())
            if (instr->opcode == aco_opcode::p_extract) {
      ssa_info& info = ctx.info[instr->operands[0].tempId()];
   if (info.is_extract() && can_apply_extract(ctx, instr, 0, info)) {
      apply_extract(ctx, instr, 0, info);
   if (--ctx.uses[instr->operands[0].tempId()])
                                 if (instr->isVOPC()) {
      if (optimize_cmp_subgroup_invocation(ctx, instr))
               /* TODO: There are still some peephole optimizations that could be done:
   * - abs(a - b) -> s_absdiff_i32
   * - various patterns for s_bitcmp{0,1}_b32 and s_bitset{0,1}_b32
   * - patterns for v_alignbit_b32 and v_alignbyte_b32
   * These aren't probably too interesting though.
   * There are also patterns for v_cmp_class_f{16,32,64}. This is difficult but
   * probably more useful than the previously mentioned optimizations.
   * The various comparison optimizations also currently only work with 32-bit
            /* neg(mul(a, b)) -> mul(neg(a), b), abs(mul(a, b)) -> mul(abs(a), abs(b)) */
   if ((ctx.info[instr->definitions[0].tempId()].label & (label_neg | label_abs)) &&
      ctx.uses[instr->operands[1].tempId()] == 1) {
            if (!ctx.info[val.id()].is_mul())
                     if (mul_instr->operands[0].isLiteral())
         if (mul_instr->valu().clamp)
         if (mul_instr->isSDWA() || mul_instr->isDPP())
         if (mul_instr->opcode == aco_opcode::v_mul_legacy_f32 &&
      ctx.fp_mode.preserve_signed_zero_inf_nan32)
      if (mul_instr->definitions[0].bytes() != instr->definitions[0].bytes())
            /* convert to mul(neg(a), b), mul(abs(a), abs(b)) or mul(neg(abs(a)), abs(b)) */
   ctx.uses[mul_instr->definitions[0].tempId()]--;
   Definition def = instr->definitions[0];
   bool is_neg = ctx.info[instr->definitions[0].tempId()].is_neg();
   bool is_abs = ctx.info[instr->definitions[0].tempId()].is_abs();
   uint32_t pass_flags = instr->pass_flags;
   Format format = mul_instr->format == Format::VOP2 ? asVOP3(Format::VOP2) : mul_instr->format;
   instr.reset(create_instruction<VALU_instruction>(mul_instr->opcode, format,
         std::copy(mul_instr->operands.cbegin(), mul_instr->operands.cend(), instr->operands.begin());
   instr->pass_flags = pass_flags;
   instr->definitions[0] = def;
   VALU_instruction& new_mul = instr->valu();
   VALU_instruction& mul = mul_instr->valu();
   new_mul.neg = mul.neg;
   new_mul.abs = mul.abs;
   new_mul.omod = mul.omod;
   new_mul.opsel = mul.opsel;
   new_mul.opsel_lo = mul.opsel_lo;
   new_mul.opsel_hi = mul.opsel_hi;
   if (is_abs) {
      new_mul.neg[0] = new_mul.neg[1] = false;
      }
   new_mul.neg[0] ^= is_neg;
            ctx.info[instr->definitions[0].tempId()].set_mul(instr.get());
               /* combine mul+add -> mad */
   bool is_add_mix =
      (instr->opcode == aco_opcode::v_fma_mix_f32 ||
   instr->opcode == aco_opcode::v_fma_mixlo_f16) &&
   !instr->valu().neg_lo[0] &&
   ((instr->operands[0].constantEquals(0x3f800000) && !instr->valu().opsel_hi[0]) ||
   (instr->operands[0].constantEquals(0x3C00) && instr->valu().opsel_hi[0] &&
      bool mad32 = instr->opcode == aco_opcode::v_add_f32 || instr->opcode == aco_opcode::v_sub_f32 ||
         bool mad16 = instr->opcode == aco_opcode::v_add_f16 || instr->opcode == aco_opcode::v_sub_f16 ||
         bool mad64 = instr->opcode == aco_opcode::v_add_f64;
   if (is_add_mix || mad16 || mad32 || mad64) {
      Instruction* mul_instr = nullptr;
   unsigned add_op_idx = 0;
   uint32_t uses = UINT32_MAX;
   bool emit_fma = false;
   /* find the 'best' mul instruction to combine with the add */
   for (unsigned i = is_add_mix ? 1 : 0; i < instr->operands.size(); i++) {
      if (!instr->operands[i].isTemp() || !ctx.info[instr->operands[i].tempId()].is_mul())
                  /* no clamp/omod allowed between mul and add */
   if (info.instr->isVOP3() && (info.instr->valu().clamp || info.instr->valu().omod))
         if (info.instr->isVOP3P() && info.instr->valu().clamp)
         /* v_fma_mix_f32/etc can't do omod */
   if (info.instr->isVOP3P() && instr->isVOP3() && instr->valu().omod)
         /* don't promote fp16 to fp32 or remove fp32->fp16->fp32 conversions */
                                                /* Multiplication by power-of-two should never need rounding. 1/power-of-two also works,
   * but using fma removes denormal flushing (0xfffffe * 0.5 + 0x810001a2).
   */
                  bool has_fma = mad16 || mad64 || (legacy && ctx.program->gfx_level >= GFX10_3) ||
               bool has_mad = mad_mix ? !ctx.program->dev.fused_mad_mix
               bool can_use_fma =
      has_fma &&
   (!(info.instr->definitions[0].isPrecise() || instr->definitions[0].isPrecise()) ||
      bool can_use_mad =
         if (mad_mix && legacy)
                        unsigned candidate_add_op_idx = is_add_mix ? (3 - i) : (1 - i);
   Operand op[3] = {info.instr->operands[0], info.instr->operands[1],
         if (info.instr->isSDWA() || info.instr->isDPP() || !check_vop3_operands(ctx, 3, op) ||
                  if (ctx.uses[instr->operands[i].tempId()] == uses) {
      unsigned cur_idx = mul_instr->definitions[0].tempId();
   unsigned new_idx = info.instr->definitions[0].tempId();
   if (cur_idx > new_idx)
               mul_instr = info.instr;
   add_op_idx = candidate_add_op_idx;
   uses = ctx.uses[instr->operands[i].tempId()];
               if (mul_instr) {
      /* turn mul+add into v_mad/v_fma */
   Operand op[3] = {mul_instr->operands[0], mul_instr->operands[1],
         ctx.uses[mul_instr->definitions[0].tempId()]--;
   if (ctx.uses[mul_instr->definitions[0].tempId()]) {
      if (op[0].isTemp())
         if (op[1].isTemp())
               bool neg[3] = {false, false, false};
   bool abs[3] = {false, false, false};
   unsigned omod = 0;
   bool clamp = false;
   bitarray8 opsel_lo = 0;
   bitarray8 opsel_hi = 0;
                  VALU_instruction& valu_mul = mul_instr->valu();
   neg[0] = valu_mul.neg[0];
   neg[1] = valu_mul.neg[1];
   abs[0] = valu_mul.abs[0];
   abs[1] = valu_mul.abs[1];
   opsel_lo = valu_mul.opsel_lo & 0x3;
                  VALU_instruction& valu = instr->valu();
   neg[2] = valu.neg[add_op_idx];
   abs[2] = valu.abs[add_op_idx];
   opsel_lo[2] = valu.opsel_lo[add_op_idx];
   opsel_hi[2] = valu.opsel_hi[add_op_idx];
   opsel[2] = valu.opsel[add_op_idx];
   opsel[3] = valu.opsel[3];
   omod = valu.omod;
   clamp = valu.clamp;
   /* abs of the multiplication result */
   if (valu.abs[mul_op_idx]) {
      neg[0] = false;
   neg[1] = false;
   abs[0] = true;
      }
                  if (instr->opcode == aco_opcode::v_sub_f32 || instr->opcode == aco_opcode::v_sub_f16)
         else if (instr->opcode == aco_opcode::v_subrev_f32 ||
                  aco_ptr<Instruction> add_instr = std::move(instr);
   aco_ptr<VALU_instruction> mad;
   if (add_instr->isVOP3P() || mul_instr->isVOP3P()) {
                     aco_opcode mad_op = add_instr->definitions[0].bytes() == 2 ? aco_opcode::v_fma_mixlo_f16
            } else {
                     aco_opcode mad_op = emit_fma ? aco_opcode::v_fma_f32 : aco_opcode::v_mad_f32;
   if (mul_instr->opcode == aco_opcode::v_mul_legacy_f32) {
      assert(emit_fma == (ctx.program->gfx_level >= GFX10_3));
      } else if (mad16) {
      mad_op = emit_fma ? (ctx.program->gfx_level == GFX8 ? aco_opcode::v_fma_legacy_f16
                  } else if (mad64) {
                              for (unsigned i = 0; i < 3; i++) {
      mad->operands[i] = op[i];
   mad->neg[i] = neg[i];
      }
   mad->omod = omod;
   mad->clamp = clamp;
   mad->opsel_lo = opsel_lo;
   mad->opsel_hi = opsel_hi;
   mad->opsel = opsel;
   mad->definitions[0] = add_instr->definitions[0];
   mad->definitions[0].setPrecise(add_instr->definitions[0].isPrecise() ||
                           /* mark this ssa_def to be re-checked for profitability and literals */
   ctx.mad_infos.emplace_back(std::move(add_instr), mul_instr->definitions[0].tempId());
   ctx.info[instr->definitions[0].tempId()].set_mad(ctx.mad_infos.size() - 1);
         }
   /* v_mul_f32(v_cndmask_b32(0, 1.0, cond), a) -> v_cndmask_b32(0, a, cond) */
   else if (((instr->opcode == aco_opcode::v_mul_f32 &&
            !ctx.fp_mode.preserve_signed_zero_inf_nan32) ||
   instr->opcode == aco_opcode::v_mul_legacy_f32) &&
   for (unsigned i = 0; i < 2; i++) {
      if (instr->operands[i].isTemp() && ctx.info[instr->operands[i].tempId()].is_b2f() &&
      ctx.uses[instr->operands[i].tempId()] == 1 && instr->operands[!i].isTemp() &&
   instr->operands[!i].getTemp().type() == RegType::vgpr) {
                  aco_ptr<VALU_instruction> new_instr{
         new_instr->operands[0] = Operand::zero();
   new_instr->operands[1] = instr->operands[!i];
   new_instr->operands[2] = Operand(ctx.info[instr->operands[i].tempId()].temp);
   new_instr->definitions[0] = instr->definitions[0];
   new_instr->pass_flags = instr->pass_flags;
   instr = std::move(new_instr);
   ctx.info[instr->definitions[0].tempId()].label = 0;
            } else if (instr->opcode == aco_opcode::v_or_b32 && ctx.program->gfx_level >= GFX9) {
      if (combine_three_valu_op(ctx, instr, aco_opcode::s_or_b32, aco_opcode::v_or3_b32, "012",
         } else if (combine_three_valu_op(ctx, instr, aco_opcode::v_or_b32, aco_opcode::v_or3_b32,
         } else if (combine_add_or_then_and_lshl(ctx, instr)) {
   } else if (combine_v_andor_not(ctx, instr)) {
      } else if (instr->opcode == aco_opcode::v_xor_b32 && ctx.program->gfx_level >= GFX10) {
      if (combine_three_valu_op(ctx, instr, aco_opcode::v_xor_b32, aco_opcode::v_xor3_b32, "012",
         } else if (combine_three_valu_op(ctx, instr, aco_opcode::s_xor_b32, aco_opcode::v_xor3_b32,
         } else if (combine_xor_not(ctx, instr)) {
      } else if (instr->opcode == aco_opcode::v_not_b32 && ctx.program->gfx_level >= GFX10) {
         } else if (instr->opcode == aco_opcode::v_add_u16) {
      combine_three_valu_op(
      ctx, instr, aco_opcode::v_mul_lo_u16,
   ctx.program->gfx_level == GFX8 ? aco_opcode::v_mad_legacy_u16 : aco_opcode::v_mad_u16,
   } else if (instr->opcode == aco_opcode::v_add_u16_e64) {
      combine_three_valu_op(ctx, instr, aco_opcode::v_mul_lo_u16_e64, aco_opcode::v_mad_u16, "120",
      } else if (instr->opcode == aco_opcode::v_add_u32) {
      if (combine_add_sub_b2i(ctx, instr, aco_opcode::v_addc_co_u32, 1 | 2)) {
   } else if (combine_add_bcnt(ctx, instr)) {
   } else if (combine_three_valu_op(ctx, instr, aco_opcode::v_mul_u32_u24,
         } else if (ctx.program->gfx_level >= GFX9 && !instr->usesModifiers()) {
      if (combine_three_valu_op(ctx, instr, aco_opcode::s_xor_b32, aco_opcode::v_xad_u32, "120",
         } else if (combine_three_valu_op(ctx, instr, aco_opcode::v_xor_b32, aco_opcode::v_xad_u32,
         } else if (combine_three_valu_op(ctx, instr, aco_opcode::s_add_i32, aco_opcode::v_add3_u32,
         } else if (combine_three_valu_op(ctx, instr, aco_opcode::s_add_u32, aco_opcode::v_add3_u32,
         } else if (combine_three_valu_op(ctx, instr, aco_opcode::v_add_u32, aco_opcode::v_add3_u32,
         } else if (combine_add_or_then_and_lshl(ctx, instr)) {
         } else if (instr->opcode == aco_opcode::v_add_co_u32 ||
            bool carry_out = ctx.uses[instr->definitions[1].tempId()] > 0;
   if (combine_add_sub_b2i(ctx, instr, aco_opcode::v_addc_co_u32, 1 | 2)) {
   } else if (!carry_out && combine_add_bcnt(ctx, instr)) {
   } else if (!carry_out && combine_three_valu_op(ctx, instr, aco_opcode::v_mul_u32_u24,
         } else if (!carry_out && combine_add_lshl(ctx, instr, false)) {
      } else if (instr->opcode == aco_opcode::v_sub_u32 || instr->opcode == aco_opcode::v_sub_co_u32 ||
            bool carry_out =
         if (combine_add_sub_b2i(ctx, instr, aco_opcode::v_subbrev_co_u32, 2)) {
   } else if (!carry_out && combine_add_lshl(ctx, instr, true)) {
      } else if (instr->opcode == aco_opcode::v_subrev_u32 ||
            instr->opcode == aco_opcode::v_subrev_co_u32 ||
      } else if (instr->opcode == aco_opcode::v_lshlrev_b32 && ctx.program->gfx_level >= GFX9) {
      combine_three_valu_op(ctx, instr, aco_opcode::v_add_u32, aco_opcode::v_add_lshl_u32, "120",
      } else if ((instr->opcode == aco_opcode::s_add_u32 || instr->opcode == aco_opcode::s_add_i32) &&
               } else if (instr->opcode == aco_opcode::s_not_b32 || instr->opcode == aco_opcode::s_not_b64) {
      if (!combine_salu_not_bitwise(ctx, instr))
      } else if (instr->opcode == aco_opcode::s_and_b32 || instr->opcode == aco_opcode::s_or_b32 ||
            if (combine_ordering_test(ctx, instr)) {
   } else if (combine_comparison_ordering(ctx, instr)) {
   } else if (combine_constant_comparison_ordering(ctx, instr)) {
   } else if (combine_salu_n2(ctx, instr)) {
      } else if (instr->opcode == aco_opcode::s_abs_i32) {
         } else if (instr->opcode == aco_opcode::v_and_b32) {
      if (combine_and_subbrev(ctx, instr)) {
   } else if (combine_v_andor_not(ctx, instr)) {
      } else if (instr->opcode == aco_opcode::v_fma_f32 || instr->opcode == aco_opcode::v_fma_f16) {
      /* set existing v_fma_f32 with label_mad so we can create v_fmamk_f32/v_fmaak_f32.
   * since ctx.uses[mad_info::mul_temp_id] is always 0, we don't have to worry about
   * select_instruction() using mad_info::add_instr.
   */
   ctx.mad_infos.emplace_back(nullptr, 0);
      } else if (instr->opcode == aco_opcode::v_med3_f32 || instr->opcode == aco_opcode::v_med3_f16) {
      unsigned idx;
   if (detect_clamp(instr.get(), &idx)) {
      instr->format = asVOP3(Format::VOP2);
   instr->operands[0] = instr->operands[idx];
   instr->operands[1] = Operand::zero();
   instr->opcode =
         instr->valu().clamp = true;
   instr->valu().abs = (uint8_t)instr->valu().abs[idx];
   instr->valu().neg = (uint8_t)instr->valu().neg[idx];
         } else {
      aco_opcode min, max, min3, max3, med3, minmax;
   bool some_gfx9_only;
   if (get_minmax_info(instr->opcode, &min, &max, &min3, &max3, &med3, &minmax,
            (!some_gfx9_only || ctx.program->gfx_level >= GFX9)) {
   if (combine_minmax(ctx, instr, instr->opcode == min ? max : min,
         } else {
                  }
      bool
   to_uniform_bool_instr(opt_ctx& ctx, aco_ptr<Instruction>& instr)
   {
      /* Check every operand to make sure they are suitable. */
   for (Operand& op : instr->operands) {
      if (!op.isTemp())
         if (!ctx.info[op.tempId()].is_uniform_bool() && !ctx.info[op.tempId()].is_uniform_bitwise())
               switch (instr->opcode) {
   case aco_opcode::s_and_b32:
   case aco_opcode::s_and_b64: instr->opcode = aco_opcode::s_and_b32; break;
   case aco_opcode::s_or_b32:
   case aco_opcode::s_or_b64: instr->opcode = aco_opcode::s_or_b32; break;
   case aco_opcode::s_xor_b32:
   case aco_opcode::s_xor_b64: instr->opcode = aco_opcode::s_absdiff_i32; break;
   default:
      /* Don't transform other instructions. They are very unlikely to appear here. */
               for (Operand& op : instr->operands) {
               if (ctx.info[op.tempId()].is_uniform_bool()) {
      /* Just use the uniform boolean temp. */
      } else if (ctx.info[op.tempId()].is_uniform_bitwise()) {
      /* Use the SCC definition of the predecessor instruction.
   * This allows the predecessor to get picked up by the same optimization (if it has no
   * divergent users), and it also makes sure that the current instruction will keep working
   * even if the predecessor won't be transformed.
   */
   Instruction* pred_instr = ctx.info[op.tempId()].instr;
   assert(pred_instr->definitions.size() >= 2);
   assert(pred_instr->definitions[1].isFixed() &&
            } else {
                              instr->definitions[0].setTemp(Temp(instr->definitions[0].tempId(), s1));
   assert(instr->operands[0].regClass() == s1);
   assert(instr->operands[1].regClass() == s1);
      }
      void
   select_instruction(opt_ctx& ctx, aco_ptr<Instruction>& instr)
   {
               if (is_dead(ctx.uses, instr.get())) {
      instr.reset();
               /* convert split_vector into a copy or extract_vector if only one definition is ever used */
   if (instr->opcode == aco_opcode::p_split_vector) {
      unsigned num_used = 0;
   unsigned idx = 0;
   unsigned split_offset = 0;
   for (unsigned i = 0, offset = 0; i < instr->definitions.size();
      offset += instr->definitions[i++].bytes()) {
   if (ctx.uses[instr->definitions[i].tempId()]) {
      num_used++;
   idx = i;
         }
   bool done = false;
   if (num_used == 1 && ctx.info[instr->operands[0].tempId()].is_vec() &&
                     unsigned off = 0;
   Operand op;
   for (Operand& vec_op : vec->operands) {
      if (off == split_offset) {
      op = vec_op;
      }
      }
   if (off != instr->operands[0].bytes() && op.bytes() == instr->definitions[idx].bytes()) {
      ctx.uses[instr->operands[0].tempId()]--;
   for (Operand& vec_op : vec->operands) {
      if (vec_op.isTemp())
      }
                  aco_ptr<Pseudo_instruction> extract{create_instruction<Pseudo_instruction>(
         extract->operands[0] = op;
                                 if (!done && num_used == 1 &&
      instr->operands[0].bytes() % instr->definitions[idx].bytes() == 0 &&
   split_offset % instr->definitions[idx].bytes() == 0) {
   aco_ptr<Pseudo_instruction> extract{create_instruction<Pseudo_instruction>(
         extract->operands[0] = instr->operands[0];
   extract->operands[1] =
         extract->definitions[0] = instr->definitions[idx];
                  mad_info* mad_info = NULL;
   if (!instr->definitions.empty() && ctx.info[instr->definitions[0].tempId()].is_mad()) {
      mad_info = &ctx.mad_infos[ctx.info[instr->definitions[0].tempId()].val];
   /* re-check mad instructions */
   if (ctx.uses[mad_info->mul_temp_id] && mad_info->add_instr) {
      ctx.uses[mad_info->mul_temp_id]++;
   if (instr->operands[0].isTemp())
         if (instr->operands[1].isTemp())
         instr.swap(mad_info->add_instr);
      }
   /* check literals */
   else if (!instr->isDPP() && !instr->isVOP3P() && instr->opcode != aco_opcode::v_fma_f64 &&
            instr->opcode != aco_opcode::v_mad_legacy_f32 &&
   /* FMA can only take literals on GFX10+ */
   if ((instr->opcode == aco_opcode::v_fma_f32 || instr->opcode == aco_opcode::v_fma_f16) &&
      ctx.program->gfx_level < GFX10)
      /* There are no v_fmaak_legacy_f16/v_fmamk_legacy_f16 and on chips where VOP3 can take
   * literals (GFX10+), these instructions don't exist.
   */
                  uint32_t literal_mask = 0;
   uint32_t fp16_mask = 0;
   uint32_t sgpr_mask = 0;
   uint32_t vgpr_mask = 0;
                  /* Iterate in reverse to prefer v_madak/v_fmaak. */
   for (int i = 2; i >= 0; i--) {
      Operand& op = instr->operands[i];
   if (!op.isTemp())
         if (ctx.info[op.tempId()].is_literal(get_operand_size(instr, i))) {
      uint32_t new_literal = ctx.info[op.tempId()].val;
   float value = uif(new_literal);
   uint16_t fp16_val = _mesa_float_to_half(value);
   bool is_denorm = (fp16_val & 0x7fff) != 0 && (fp16_val & 0x7fff) <= 0x3ff;
                        if (!literal_mask || literal_value == new_literal) {
      literal_value = new_literal;
   literal_uses = MIN2(literal_uses, ctx.uses[op.tempId()]);
   literal_mask |= 1 << i;
         }
   sgpr_mask |= op.isOfType(RegType::sgpr) << i;
               /* The constant bus limitations before GFX10 disallows SGPRs. */
                  /* Encoding needs a vgpr. */
                  /* v_madmk/v_fmamk needs a vgpr in the third source. */
                  /* opsel with GFX11+ is the only modifier supported by fmamk/fmaak*/
   if (instr->valu().abs || instr->valu().neg || instr->valu().omod || instr->valu().clamp ||
                                 /* We can't use three unique fp16 literals */
                  if ((instr->opcode == aco_opcode::v_fma_f32 ||
      (instr->opcode == aco_opcode::v_mad_f32 && !instr->definitions[0].isPrecise())) &&
   !instr->valu().omod && ctx.program->gfx_level >= GFX10 &&
   util_bitcount(fp16_mask) > std::max<uint32_t>(util_bitcount(literal_mask), 1)) {
   assert(ctx.program->dev.fused_mad_mix);
   u_foreach_bit (i, fp16_mask)
         mad_info->fp16_mask = fp16_mask;
               /* Limit the number of literals to apply to not increase the code
   * size too much, but always apply literals for v_mad->v_madak
   * because both instructions are 64-bit and this doesn't increase
   * code size.
   * TODO: try to apply the literals earlier to lower the number of
   * uses below threshold
   */
   if (literal_mask && (literal_uses < threshold || (literal_mask & 0b100))) {
      u_foreach_bit (i, literal_mask)
         mad_info->literal_mask = literal_mask;
                     /* Mark SCC needed, so the uniform boolean transformation won't swap the definitions
   * when it isn't beneficial */
   if (instr->isBranch() && instr->operands.size() && instr->operands[0].isTemp() &&
      instr->operands[0].isFixed() && instr->operands[0].physReg() == scc) {
   ctx.info[instr->operands[0].tempId()].set_scc_needed();
      } else if ((instr->opcode == aco_opcode::s_cselect_b64 ||
                              /* check for literals */
   if (!instr->isSALU() && !instr->isVALU())
            /* Transform uniform bitwise boolean operations to 32-bit when there are no divergent uses. */
   if (instr->definitions.size() && ctx.uses[instr->definitions[0].tempId()] == 0 &&
      ctx.info[instr->definitions[0].tempId()].is_uniform_bitwise()) {
            if (transform_done && !ctx.info[instr->definitions[1].tempId()].is_scc_needed()) {
      /* Swap the two definition IDs in order to avoid overusing the SCC.
   * This reduces extra moves generated by RA. */
   uint32_t def0_id = instr->definitions[0].getTemp().id();
   uint32_t def1_id = instr->definitions[1].getTemp().id();
   instr->definitions[0].setTemp(Temp(def1_id, s1));
                           /* This optimization is done late in order to be able to apply otherwise
   * unsafe optimizations such as the inverse comparison optimization.
   */
   if (instr->opcode == aco_opcode::s_and_b32 || instr->opcode == aco_opcode::s_and_b64) {
      if (instr->operands[0].isTemp() && fixed_to_exec(instr->operands[1]) &&
      ctx.uses[instr->operands[0].tempId()] == 1 &&
   ctx.uses[instr->definitions[1].tempId()] == 0 &&
   can_eliminate_and_exec(ctx, instr->operands[0].getTemp(), instr->pass_flags)) {
   ctx.uses[instr->operands[0].tempId()]--;
   ctx.info[instr->operands[0].tempId()].instr->definitions[0].setTemp(
         instr.reset();
                  /* Combine DPP copies into VALU. This should be done after creating MAD/FMA. */
   if (instr->isVALU() && !instr->isDPP()) {
      for (unsigned i = 0; i < instr->operands.size(); i++) {
      if (!instr->operands[i].isTemp())
                                 /* We won't eliminate the DPP mov if the operand is used twice */
   bool op_used_twice = false;
   for (unsigned j = 0; j < instr->operands.size(); j++)
                        if (i != 0) {
      if (!can_swap_operands(instr, &instr->opcode, 0, i))
                                    bool dpp8 = info.is_dpp8();
   bool input_mods = can_use_input_modifiers(ctx.program->gfx_level, instr->opcode, 0) &&
         bool mov_uses_mods = info.instr->valu().neg[0] || info.instr->valu().abs[0];
                           if (dpp8) {
      DPP8_instruction* dpp = &instr->dpp8();
   dpp->lane_sel = info.instr->dpp8().lane_sel;
   dpp->fetch_inactive = info.instr->dpp8().fetch_inactive;
   if (mov_uses_mods)
      } else {
      DPP16_instruction* dpp = &instr->dpp16();
   dpp->dpp_ctrl = info.instr->dpp16().dpp_ctrl;
   dpp->bound_ctrl = info.instr->dpp16().bound_ctrl;
                              if (--ctx.uses[info.instr->definitions[0].tempId()])
         instr->operands[0].setTemp(info.instr->operands[0].getTemp());
                  /* Use v_fma_mix for f2f32/f2f16 if it has higher throughput.
   * Do this late to not disturb other optimizations.
   */
   if ((instr->opcode == aco_opcode::v_cvt_f32_f16 || instr->opcode == aco_opcode::v_cvt_f16_f32) &&
      ctx.program->gfx_level >= GFX11 && ctx.program->wave_size == 64 && !instr->valu().omod &&
   !instr->isDPP()) {
   bool is_f2f16 = instr->opcode == aco_opcode::v_cvt_f16_f32;
   Instruction* fma = create_instruction<VALU_instruction>(
         fma->definitions[0] = instr->definitions[0];
   fma->operands[0] = instr->operands[0];
   fma->valu().opsel_hi[0] = !is_f2f16;
   fma->valu().opsel_lo[0] = instr->valu().opsel[0];
   fma->valu().clamp = instr->valu().clamp;
   fma->valu().abs[0] = instr->valu().abs[0];
   fma->valu().neg[0] = instr->valu().neg[0];
   fma->operands[1] = Operand::c32(fui(1.0f));
   fma->operands[2] = Operand::zero();
   /* fma_mix is only dual issued if dst and acc type match */
   fma->valu().opsel_hi[2] = is_f2f16;
   fma->valu().neg[2] = true;
   instr.reset(fma);
               if (instr->isSDWA() || (instr->isVOP3() && ctx.program->gfx_level < GFX10) ||
      (instr->isVOP3P() && ctx.program->gfx_level < GFX10))
         /* we do not apply the literals yet as we don't know if it is profitable */
            unsigned literal_id = 0;
   unsigned literal_uses = UINT32_MAX;
   Operand literal(s1);
   unsigned num_operands = 1;
   if (instr->isSALU() || (ctx.program->gfx_level >= GFX10 &&
               /* catch VOP2 with a 3rd SGPR operand (e.g. v_cndmask_b32, v_addc_co_u32) */
   else if (instr->isVALU() && instr->operands.size() >= 3)
            unsigned sgpr_ids[2] = {0, 0};
   bool is_literal_sgpr = false;
            /* choose a literal to apply */
   for (unsigned i = 0; i < num_operands; i++) {
      Operand op = instr->operands[i];
            if (instr->isVALU() && op.isTemp() && op.getTemp().type() == RegType::sgpr &&
                  if (op.isLiteral()) {
      current_literal = op;
      } else if (!op.isTemp() || !ctx.info[op.tempId()].is_literal(bits)) {
                  if (!alu_can_accept_constant(instr, i))
            if (ctx.uses[op.tempId()] < literal_uses) {
      is_literal_sgpr = op.getTemp().type() == RegType::sgpr;
   mask = 0;
   literal = Operand::c32(ctx.info[op.tempId()].val);
   literal_uses = ctx.uses[op.tempId()];
                           /* don't go over the constant bus limit */
   bool is_shift64 = instr->opcode == aco_opcode::v_lshlrev_b64 ||
               unsigned const_bus_limit = instr->isVALU() ? 1 : UINT32_MAX;
   if (ctx.program->gfx_level >= GFX10 && !is_shift64)
            unsigned num_sgprs = !!sgpr_ids[0] + !!sgpr_ids[1];
   if (num_sgprs == const_bus_limit && !is_literal_sgpr)
            if (literal_id && literal_uses < threshold &&
      (current_literal.isUndefined() ||
   (current_literal.size() == literal.size() &&
         /* mark the literal to be applied */
   while (mask) {
      unsigned i = u_bit_scan(&mask);
   if (instr->operands[i].isTemp() && instr->operands[i].tempId() == literal_id)
            }
      static aco_opcode
   sopk_opcode_for_sopc(aco_opcode opcode)
   {
   #define CTOK(op)                                                                                   \
      case aco_opcode::s_cmp_##op##_i32: return aco_opcode::s_cmpk_##op##_i32;                        \
   case aco_opcode::s_cmp_##op##_u32: return aco_opcode::s_cmpk_##op##_u32;
   switch (opcode) {
      CTOK(eq)
   CTOK(lg)
   CTOK(gt)
   CTOK(ge)
   CTOK(lt)
      default: return aco_opcode::num_opcodes;
      #undef CTOK
   }
      static bool
   sopc_is_signed(aco_opcode opcode)
   {
   #define SOPC(op)                                                                                   \
      case aco_opcode::s_cmp_##op##_i32: return true;                                                 \
   case aco_opcode::s_cmp_##op##_u32: return false;
   switch (opcode) {
      SOPC(eq)
   SOPC(lg)
   SOPC(gt)
   SOPC(ge)
   SOPC(lt)
      default: unreachable("Not a valid SOPC instruction.");
      #undef SOPC
   }
      static aco_opcode
   sopc_32_swapped(aco_opcode opcode)
   {
   #define SOPC(op1, op2)                                                                             \
      case aco_opcode::s_cmp_##op1##_i32: return aco_opcode::s_cmp_##op2##_i32;                       \
   case aco_opcode::s_cmp_##op1##_u32: return aco_opcode::s_cmp_##op2##_u32;
   switch (opcode) {
      SOPC(eq, eq)
   SOPC(lg, lg)
   SOPC(gt, lt)
   SOPC(ge, le)
   SOPC(lt, gt)
      default: return aco_opcode::num_opcodes;
      #undef SOPC
   }
      static void
   try_convert_sopc_to_sopk(aco_ptr<Instruction>& instr)
   {
      if (sopk_opcode_for_sopc(instr->opcode) == aco_opcode::num_opcodes)
            if (instr->operands[0].isLiteral()) {
      std::swap(instr->operands[0], instr->operands[1]);
               if (!instr->operands[1].isLiteral())
            if (instr->operands[0].isFixed() && instr->operands[0].physReg() >= 128)
                              bool value_is_i16 = (value & i16_mask) == 0 || (value & i16_mask) == i16_mask;
            if (!value_is_i16 && !value_is_u16)
            if (!value_is_i16 && sopc_is_signed(instr->opcode)) {
      if (instr->opcode == aco_opcode::s_cmp_lg_i32)
         else if (instr->opcode == aco_opcode::s_cmp_eq_i32)
         else
      } else if (!value_is_u16 && !sopc_is_signed(instr->opcode)) {
      if (instr->opcode == aco_opcode::s_cmp_lg_u32)
         else if (instr->opcode == aco_opcode::s_cmp_eq_u32)
         else
               static_assert(sizeof(SOPK_instruction) <= sizeof(SOPC_instruction),
         instr->format = Format::SOPK;
            instr_sopk->imm = instr_sopk->operands[1].constantValue() & 0xffff;
   instr_sopk->opcode = sopk_opcode_for_sopc(instr_sopk->opcode);
      }
      static void
   unswizzle_vop3p_literals(opt_ctx& ctx, aco_ptr<Instruction>& instr)
   {
      /* This opt is only beneficial for v_pk_fma_f16 because we can use v_pk_fmac_f16 if the
   * instruction doesn't use swizzles. */
   if (instr->opcode != aco_opcode::v_pk_fma_f16)
                     unsigned literal_swizzle = ~0u;
   for (unsigned i = 0; i < instr->operands.size(); i++) {
      if (!instr->operands[i].isLiteral())
         unsigned new_swizzle = vop3p.opsel_lo[i] | (vop3p.opsel_hi[i] << 1);
   if (literal_swizzle != ~0u && new_swizzle != literal_swizzle)
                     if (literal_swizzle == 0b10 || literal_swizzle == ~0u)
            for (unsigned i = 0; i < instr->operands.size(); i++) {
      if (!instr->operands[i].isLiteral())
         uint32_t literal = instr->operands[i].constantValue();
   literal = (literal >> (16 * (literal_swizzle & 0x1)) & 0xffff) |
         instr->operands[i] = Operand::literal32(literal);
   vop3p.opsel_lo[i] = false;
         }
      void
   apply_literals(opt_ctx& ctx, aco_ptr<Instruction>& instr)
   {
      /* Cleanup Dead Instructions */
   if (!instr)
            /* apply literals on MAD */
   if (!instr->definitions.empty() && ctx.info[instr->definitions[0].tempId()].is_mad()) {
      mad_info* info = &ctx.mad_infos[ctx.info[instr->definitions[0].tempId()].val];
   const bool madak = (info->literal_mask & 0b100);
   bool has_dead_literal = false;
   u_foreach_bit (i, info->literal_mask | info->fp16_mask)
            if (has_dead_literal && info->fp16_mask) {
                     uint32_t literal = 0;
   bool second = false;
   u_foreach_bit (i, info->fp16_mask) {
      float value = uif(ctx.info[instr->operands[i].tempId()].val);
   literal |= _mesa_float_to_half(value) << (second * 16);
   instr->valu().opsel_lo[i] = second;
   instr->valu().opsel_hi[i] = true;
               for (unsigned i = 0; i < 3; i++) {
      if (info->fp16_mask & (1 << i))
               ctx.instructions.emplace_back(std::move(instr));
               if (has_dead_literal || madak) {
      aco_opcode new_op = madak ? aco_opcode::v_madak_f32 : aco_opcode::v_madmk_f32;
   if (instr->opcode == aco_opcode::v_fma_f32)
         else if (instr->opcode == aco_opcode::v_mad_f16 ||
                              uint32_t literal = ctx.info[instr->operands[ffs(info->literal_mask) - 1].tempId()].val;
   instr->format = Format::VOP2;
   instr->opcode = new_op;
   for (unsigned i = 0; i < 3; i++) {
      if (info->literal_mask & (1 << i))
      }
   if (madak) { /* add literal -> madak */
      if (!instr->operands[1].isOfType(RegType::vgpr))
      } else { /* mul literal -> madmk */
      if (!(info->literal_mask & 0b10))
            }
   ctx.instructions.emplace_back(std::move(instr));
                  /* apply literals on other SALU/VALU */
   if (instr->isSALU() || instr->isVALU()) {
      for (unsigned i = 0; i < instr->operands.size(); i++) {
      Operand op = instr->operands[i];
   unsigned bits = get_operand_size(instr, i);
   if (op.isTemp() && ctx.info[op.tempId()].is_literal(bits) && ctx.uses[op.tempId()] == 0) {
      Operand literal = Operand::literal32(ctx.info[op.tempId()].val);
   instr->format = withoutDPP(instr->format);
   if (instr->isVALU() && i > 0 && instr->format != Format::VOP3P)
                           if (instr->isSOPC())
            /* allow more s_addk_i32 optimizations if carry isn't used */
   if (instr->opcode == aco_opcode::s_add_u32 && ctx.uses[instr->definitions[1].tempId()] == 0 &&
      (instr->operands[0].isLiteral() || instr->operands[1].isLiteral()))
         if (instr->isVOP3P())
               }
      void
   optimize(Program* program)
   {
      opt_ctx ctx;
   ctx.program = program;
   std::vector<ssa_info> info(program->peekAllocationId());
            /* 1. Bottom-Up DAG pass (forward) to label all ssa-defs */
   for (Block& block : program->blocks) {
      ctx.fp_mode = block.fp_mode;
   for (aco_ptr<Instruction>& instr : block.instructions)
                        /* 2. Combine v_mad, omod, clamp and propagate sgpr on VALU instructions */
   for (Block& block : program->blocks) {
      ctx.fp_mode = block.fp_mode;
   for (aco_ptr<Instruction>& instr : block.instructions)
               /* 3. Top-Down DAG pass (backward) to select instructions (includes DCE) */
   for (auto block_rit = program->blocks.rbegin(); block_rit != program->blocks.rend();
      ++block_rit) {
   Block* block = &(*block_rit);
   ctx.fp_mode = block->fp_mode;
   for (auto instr_rit = block->instructions.rbegin(); instr_rit != block->instructions.rend();
      ++instr_rit)
            /* 4. Add literals to instructions */
   for (Block& block : program->blocks) {
      ctx.instructions.reserve(block.instructions.size());
   ctx.fp_mode = block.fp_mode;
   for (aco_ptr<Instruction>& instr : block.instructions)
               }
      } // namespace aco
