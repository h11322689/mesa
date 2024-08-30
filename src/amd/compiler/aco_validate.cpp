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
      #include "aco_ir.h"
      #include "util/memstream.h"
   #include "util/ralloc.h"
      #include <array>
   #include <map>
   #include <set>
   #include <vector>
      namespace aco {
      static void
   aco_log(Program* program, enum aco_compiler_debug_level level, const char* prefix, const char* file,
         {
               if (program->debug.shorten_messages) {
         } else {
      msg = ralloc_strdup(NULL, prefix);
   ralloc_asprintf_append(&msg, "    In file %s:%u\n", file, line);
   ralloc_asprintf_append(&msg, "    ");
               if (program->debug.func)
                        }
      void
   _aco_perfwarn(Program* program, const char* file, unsigned line, const char* fmt, ...)
   {
               va_start(args, fmt);
   aco_log(program, ACO_COMPILER_DEBUG_LEVEL_PERFWARN, "ACO PERFWARN:\n", file, line, fmt, args);
      }
      void
   _aco_err(Program* program, const char* file, unsigned line, const char* fmt, ...)
   {
               va_start(args, fmt);
   aco_log(program, ACO_COMPILER_DEBUG_LEVEL_ERROR, "ACO ERROR:\n", file, line, fmt, args);
      }
      bool
   validate_ir(Program* program)
   {
      bool is_valid = true;
   auto check = [&program, &is_valid](bool success, const char* msg,
         {
      if (!success) {
      char* out;
   size_t outsize;
   struct u_memstream mem;
                  fprintf(memf, "%s: ", msg);
                                                for (Block& block : program->blocks) {
                  /* check base format */
   Format base_format = instr->format;
   base_format = (Format)((uint32_t)base_format & ~(uint32_t)Format::SDWA);
   base_format = (Format)((uint32_t)base_format & ~(uint32_t)Format::DPP16);
   base_format = (Format)((uint32_t)base_format & ~(uint32_t)Format::DPP8);
   if ((uint32_t)base_format & (uint32_t)Format::VOP1)
         else if ((uint32_t)base_format & (uint32_t)Format::VOP2)
         else if ((uint32_t)base_format & (uint32_t)Format::VOPC)
         else if ((uint32_t)base_format & (uint32_t)Format::VINTRP) {
      if (instr->opcode == aco_opcode::v_interp_p1ll_f16 ||
      instr->opcode == aco_opcode::v_interp_p1lv_f16 ||
   instr->opcode == aco_opcode::v_interp_p2_legacy_f16 ||
   instr->opcode == aco_opcode::v_interp_p2_f16) {
   /* v_interp_*_fp16 are considered VINTRP by the compiler but
   * they are emitted as VOP3.
   */
      } else {
            }
                  /* check VOP3 modifiers */
   if (instr->isVOP3() && withoutDPP(instr->format) != Format::VOP3) {
      check(base_format == Format::VOP2 || base_format == Format::VOP1 ||
                     if (instr->isDPP()) {
      check(base_format == Format::VOP2 || base_format == Format::VOP1 ||
            base_format == Format::VOPC || base_format == Format::VOP3 ||
                     bool fi =
         check(!fi || program->gfx_level >= GFX10, "DPP Fetch-Inactive is GFX10+ only",
               /* check SDWA */
   if (instr->isSDWA()) {
      check(base_format == Format::VOP2 || base_format == Format::VOP1 ||
                                 SDWA_instruction& sdwa = instr->sdwa();
   check(sdwa.omod == 0 || program->gfx_level >= GFX9, "SDWA omod only supported on GFX9+",
         if (base_format == Format::VOPC) {
      check(sdwa.clamp == false || program->gfx_level == GFX8,
         check((instr->definitions[0].isFixed() && instr->definitions[0].physReg() == vcc) ||
            } else {
      const Definition& def = instr->definitions[0];
   check(def.bytes() <= 4, "SDWA definitions must not be larger than 4 bytes",
         check(def.bytes() >= sdwa.dst_sel.size() + sdwa.dst_sel.offset(),
         check(
      sdwa.dst_sel.size() == 1 || sdwa.dst_sel.size() == 2 || sdwa.dst_sel.size() == 4,
      check(sdwa.dst_sel.offset() % sdwa.dst_sel.size() == 0, "Invalid selection offset",
         check(def.bytes() == 4 || def.bytes() == sdwa.dst_sel.size(),
         "SDWA dst_sel size must be definition size for subdword definitions",
                     for (unsigned i = 0; i < std::min<unsigned>(2, instr->operands.size()); i++) {
      const Operand& op = instr->operands[i];
   check(op.bytes() <= 4, "SDWA operands must not be larger than 4 bytes", instr.get());
   check(op.bytes() >= sdwa.sel[i].size() + sdwa.sel[i].offset(),
         check(sdwa.sel[i].size() == 1 || sdwa.sel[i].size() == 2 || sdwa.sel[i].size() == 4,
         check(sdwa.sel[i].offset() % sdwa.sel[i].size() == 0, "Invalid selection offset",
      }
   if (instr->operands.size() >= 3) {
      check(instr->operands[2].isFixed() && instr->operands[2].physReg() == vcc,
      }
   if (instr->definitions.size() >= 2) {
                        const bool sdwa_opcodes =
      instr->opcode != aco_opcode::v_fmac_f32 && instr->opcode != aco_opcode::v_fmac_f16 &&
   instr->opcode != aco_opcode::v_fmamk_f32 &&
   instr->opcode != aco_opcode::v_fmaak_f32 &&
   instr->opcode != aco_opcode::v_fmamk_f16 &&
   instr->opcode != aco_opcode::v_fmaak_f16 &&
   instr->opcode != aco_opcode::v_madmk_f32 &&
   instr->opcode != aco_opcode::v_madak_f32 &&
   instr->opcode != aco_opcode::v_madmk_f16 &&
                     const bool feature_mac =
                              /* check opsel */
   if (instr->isVOP3() || instr->isVOP1() || instr->isVOP2() || instr->isVOPC()) {
      VALU_instruction& valu = instr->valu();
   check(valu.opsel == 0 || program->gfx_level >= GFX9, "Opsel is only supported on GFX9+",
                        for (unsigned i = 0; i < 3; i++) {
      if (i >= instr->operands.size() ||
      (!instr->isVOP3() && !instr->operands[i].isOfType(RegType::vgpr)) ||
   (instr->operands[i].hasRegClass() &&
   instr->operands[i].regClass().is_subdword() && !instr->operands[i].isFixed()))
   }
   if (instr->definitions[0].regClass().is_subdword() && !instr->definitions[0].isFixed())
      } else if (instr->opcode == aco_opcode::v_fma_mixlo_f16 ||
            instr->opcode == aco_opcode::v_fma_mixhi_f16 ||
   check(instr->definitions[0].regClass() ==
            } else if (instr->isVOP3P()) {
      VALU_instruction& vop3p = instr->valu();
   for (unsigned i = 0; i < instr->operands.size(); i++) {
      if (instr->operands[i].hasRegClass() &&
      instr->operands[i].regClass().is_subdword() && !instr->operands[i].isFixed())
   check(!vop3p.opsel_lo[i] && !vop3p.opsel_hi[i],
   }
   check(instr->definitions[0].regClass() == v1 ||
                     /* check for undefs */
   for (unsigned i = 0; i < instr->operands.size(); i++) {
      if (instr->operands[i].isUndefined()) {
      bool flat = instr->isFlatLike();
   bool can_be_undef = is_phi(instr) || instr->isEXP() || instr->isReduction() ||
                     instr->opcode == aco_opcode::p_create_vector ||
   instr->opcode == aco_opcode::p_jump_to_epilog ||
   instr->opcode == aco_opcode::p_dual_src_export_gfx11 ||
   instr->opcode == aco_opcode::p_end_with_regs ||
   (instr->opcode == aco_opcode::p_interp_gfx11 && i == 0) ||
   (instr->opcode == aco_opcode::p_bpermute_permlane && i == 0) ||
   (flat && i == 1) || (instr->isMIMG() && (i == 1 || i == 2)) ||
      } else {
      check(instr->operands[i].isFixed() || instr->operands[i].isTemp() ||
                        /* check subdword definitions */
   for (unsigned i = 0; i < instr->definitions.size(); i++) {
      if (instr->definitions[i].regClass().is_subdword())
      check(instr->definitions[i].bytes() <= 4 || instr->isPseudo() || instr->isVMEM(),
                  if ((instr->isSALU() && instr->opcode != aco_opcode::p_constaddr_addlo &&
      instr->opcode != aco_opcode::p_resumeaddr_addlo) ||
   instr->isVALU()) {
   /* check literals */
   Operand literal(s1);
   for (unsigned i = 0; i < instr->operands.size(); i++) {
                           check(!instr->isDPP() && !instr->isSDWA() &&
                        check(literal.isUndefined() || (literal.size() == op.size() &&
               literal = op;
                     /* check num sgprs for VALU */
   if (instr->isVALU()) {
      bool is_shift64 = instr->opcode == aco_opcode::v_lshlrev_b64 ||
                                    uint32_t scalar_mask =
         if (instr->isSDWA())
                        if (instr->isVOPC() || instr->opcode == aco_opcode::v_readfirstlane_b32 ||
      instr->opcode == aco_opcode::v_readlane_b32 ||
   instr->opcode == aco_opcode::v_readlane_b32_e64) {
   check(instr->definitions[0].regClass().type() == RegType::sgpr,
      } else {
                        unsigned num_sgprs = 0;
   unsigned sgpr[] = {0, 0};
   for (unsigned i = 0; i < instr->operands.size(); i++) {
      Operand op = instr->operands[i];
   if (instr->opcode == aco_opcode::v_readfirstlane_b32 ||
      instr->opcode == aco_opcode::v_readlane_b32 ||
   instr->opcode == aco_opcode::v_readlane_b32_e64) {
   check(i != 1 || op.isOfType(RegType::sgpr) || op.isConstant(),
         check(i == 1 || (op.isOfType(RegType::vgpr) && op.bytes() <= 4),
            }
   if (instr->opcode == aco_opcode::v_permlane16_b32 ||
      instr->opcode == aco_opcode::v_permlanex16_b32) {
   check(i != 0 || op.isOfType(RegType::vgpr),
                                 if (instr->opcode == aco_opcode::v_writelane_b32 ||
      instr->opcode == aco_opcode::v_writelane_b32_e64) {
   check(i != 2 || (op.isOfType(RegType::vgpr) && op.bytes() <= 4),
         check(i == 2 || op.isOfType(RegType::sgpr) || op.isConstant(),
            }
                           if (op.tempId() != sgpr[0] && op.tempId() != sgpr[1]) {
                           if (op.isConstant() && !op.isLiteral())
      check(scalar_mask & (1 << i), "Wrong source position for constant argument",
                        /* Validate modifiers. */
   check(!instr->valu().opsel || instr->isVOP3() || instr->isVOP1() ||
               check(!instr->valu().opsel_lo || instr->isVOP3P(),
         check(!instr->valu().opsel_hi || instr->isVOP3P(),
         check(!instr->valu().omod || instr->isVOP3() || instr->isSDWA(),
                              for (bool abs : instr->valu().abs) {
      check(!abs || instr->isVOP3() || instr->isVOP3P() || instr->isSDWA() ||
            }
   for (bool neg : instr->valu().neg) {
      check(!neg || instr->isVOP3() || instr->isVOP3P() || instr->isSDWA() ||
                        if (instr->isSOP1() || instr->isSOP2()) {
      if (!instr->definitions.empty())
      check(instr->definitions[0].regClass().type() == RegType::sgpr,
      for (const Operand& op : instr->operands) {
      check(op.isConstant() || op.isOfType(RegType::sgpr),
                     switch (instr->format) {
   case Format::PSEUDO: {
      if (instr->opcode == aco_opcode::p_create_vector) {
      unsigned size = 0;
   for (const Operand& op : instr->operands) {
      check(op.bytes() < 4 || size % 4 == 0, "Operand is not aligned", instr.get());
      }
   check(size == instr->definitions[0].bytes(),
         if (instr->definitions[0].regClass().type() == RegType::sgpr) {
      for (const Operand& op : instr->operands) {
      check(op.isConstant() || op.regClass().type() == RegType::sgpr,
            } else if (instr->opcode == aco_opcode::p_extract_vector) {
      check(!instr->operands[0].isConstant() && instr->operands[1].isConstant(),
         check((instr->operands[1].constantValue() + 1) * instr->definitions[0].bytes() <=
               check(instr->definitions[0].regClass().type() == RegType::vgpr ||
               check(program->gfx_level >= GFX9 ||
            !instr->definitions[0].regClass().is_subdword() ||
   } else if (instr->opcode == aco_opcode::p_split_vector) {
      check(!instr->operands[0].isConstant(), "Operand must not be constant", instr.get());
   unsigned size = 0;
   for (const Definition& def : instr->definitions) {
         }
   check(size == instr->operands[0].bytes(),
         if (instr->operands[0].isOfType(RegType::vgpr)) {
      for (const Definition& def : instr->definitions)
      check(def.regClass().type() == RegType::vgpr,
   } else {
      for (const Definition& def : instr->definitions)
      check(program->gfx_level >= GFX9 || !def.regClass().is_subdword(),
      } else if (instr->opcode == aco_opcode::p_parallelcopy) {
      check(instr->definitions.size() == instr->operands.size(),
         for (unsigned i = 0; i < instr->operands.size(); i++) {
      check(instr->definitions[i].bytes() == instr->operands[i].bytes(),
         if (instr->operands[i].hasRegClass()) {
      check((instr->definitions[i].regClass().type() ==
         instr->operands[i].regClass().type()) ||
      (instr->definitions[i].regClass().type() == RegType::vgpr &&
      check(instr->definitions[i].regClass().is_linear_vgpr() ==
            } else {
      check(!instr->definitions[i].regClass().is_linear_vgpr(),
                  } else if (instr->opcode == aco_opcode::p_phi) {
      check(instr->operands.size() == block.logical_preds.size(),
         check(instr->definitions[0].regClass().type() == RegType::vgpr,
         for (const Operand& op : instr->operands)
      check(instr->definitions[0].size() == op.size(),
   } else if (instr->opcode == aco_opcode::p_linear_phi) {
      for (const Operand& op : instr->operands) {
      check(!op.isTemp() || op.getTemp().is_linear(), "Wrong Operand type",
         check(instr->definitions[0].size() == op.size(),
      }
   check(instr->operands.size() == block.linear_preds.size(),
      } else if (instr->opcode == aco_opcode::p_extract ||
            check(!instr->operands[0].isConstant(), "Data operand must not be constant",
         check(instr->operands[1].isConstant(), "Index must be constant", instr.get());
                                                                  if (instr->definitions[0].regClass().type() == RegType::sgpr)
                                       if (instr->opcode == aco_opcode::p_insert) {
      check(op_bits == 8 || op_bits == 16, "Size must be 8 or 16", instr.get());
      } else if (instr->opcode == aco_opcode::p_extract) {
      check(op_bits == 8 || op_bits == 16 || op_bits == 32,
                           unsigned comp = data_bits / MAX2(op_bits, 1);
   check(instr->operands[1].constantValue() < comp, "Index must be in-bounds",
      } else if (instr->opcode == aco_opcode::p_jump_to_epilog) {
      check(instr->definitions.size() == 0, "p_jump_to_epilog must have 0 definitions",
         check(instr->operands.size() > 0 && instr->operands[0].isOfType(RegType::sgpr) &&
               for (unsigned i = 1; i < instr->operands.size(); i++) {
      check(instr->operands[i].isOfType(RegType::vgpr) ||
            instr->operands[i].isOfType(RegType::sgpr) ||
            } else if (instr->opcode == aco_opcode::p_dual_src_export_gfx11) {
      check(instr->definitions.size() == 6,
         check(instr->definitions[2].regClass() == program->lane_mask,
         "Third definition of p_dual_src_export_gfx11 must be a lane mask",
   check(instr->definitions[3].regClass() == program->lane_mask,
         "Fourth definition of p_dual_src_export_gfx11 must be a lane mask",
   check(instr->definitions[4].physReg() == vcc,
         check(instr->definitions[5].physReg() == scc,
         check(instr->operands.size() == 8, "p_dual_src_export_gfx11 must have 8 operands",
         for (unsigned i = 0; i < instr->operands.size(); i++) {
      check(
      instr->operands[i].isOfType(RegType::vgpr) || instr->operands[i].isUndefined(),
      } else if (instr->opcode == aco_opcode::p_start_linear_vgpr) {
      check(instr->definitions.size() == 1, "Must have one definition", instr.get());
   check(instr->operands.size() <= 1, "Must have one or zero operands", instr.get());
   if (!instr->definitions.empty())
      check(instr->definitions[0].regClass().is_linear_vgpr(),
      if (!instr->definitions.empty() && !instr->operands.empty())
      check(instr->definitions[0].bytes() == instr->operands[0].bytes(),
   }
      }
   case Format::PSEUDO_REDUCTION: {
      for (const Operand& op : instr->operands)
                        if (instr->opcode == aco_opcode::p_reduce &&
      instr->reduction().cluster_size == program->wave_size)
   check(instr->definitions[0].regClass().type() == RegType::sgpr ||
            else
                           }
   case Format::SMEM: {
      if (instr->operands.size() >= 1)
      check(instr->operands[0].isOfType(RegType::sgpr), "SMEM operands must be sgpr",
      if (instr->operands.size() >= 2)
      check(instr->operands[1].isConstant() || instr->operands[1].isOfType(RegType::sgpr),
      if (!instr->definitions.empty())
      check(instr->definitions[0].regClass().type() == RegType::sgpr,
         }
   case Format::MTBUF:
   case Format::MUBUF: {
      check(instr->operands.size() > 1, "VMEM instructions must have at least one operand",
         check(instr->operands[1].isOfType(RegType::vgpr),
         check(instr->operands[0].isOfType(RegType::sgpr), "VMEM resource constant must be sgpr",
                        const bool d16 =
      instr->opcode ==
         instr->opcode == aco_opcode::buffer_load_ubyte ||
   instr->opcode == aco_opcode::buffer_load_sbyte ||
   instr->opcode == aco_opcode::buffer_load_ushort ||
   instr->opcode == aco_opcode::buffer_load_sshort ||
   instr->opcode == aco_opcode::buffer_load_ubyte_d16 ||
   instr->opcode == aco_opcode::buffer_load_ubyte_d16_hi ||
   instr->opcode == aco_opcode::buffer_load_sbyte_d16 ||
   instr->opcode == aco_opcode::buffer_load_sbyte_d16_hi ||
   instr->opcode == aco_opcode::buffer_load_short_d16 ||
   instr->opcode == aco_opcode::buffer_load_short_d16_hi ||
   instr->opcode == aco_opcode::buffer_load_format_d16_x ||
   instr->opcode == aco_opcode::buffer_load_format_d16_hi_x ||
   instr->opcode == aco_opcode::buffer_load_format_d16_xy ||
   instr->opcode == aco_opcode::buffer_load_format_d16_xyz ||
   instr->opcode == aco_opcode::buffer_load_format_d16_xyzw ||
   instr->opcode == aco_opcode::tbuffer_load_format_d16_x ||
   instr->opcode == aco_opcode::tbuffer_load_format_d16_xy ||
   instr->opcode == aco_opcode::tbuffer_load_format_d16_xyz ||
      if (instr->definitions.size()) {
      check(instr->definitions[0].regClass().type() == RegType::vgpr,
         check(d16 || !instr->definitions[0].regClass().is_subdword(),
         check(instr->definitions[0].bytes() <= 8 || !d16,
      }
      }
   case Format::MIMG: {
      check(instr->operands.size() >= 4, "MIMG instructions must have at least 4 operands",
         check(instr->operands[0].hasRegClass() &&
               if (instr->operands[1].hasRegClass())
      check(instr->operands[1].regClass() == s4,
      if (!instr->operands[2].isUndefined()) {
      bool is_cmpswap = instr->opcode == aco_opcode::image_atomic_cmpswap ||
         check(instr->definitions.empty() ||
            (instr->definitions[0].regClass() == instr->operands[2].regClass() ||
                        if (instr->mimg().strict_wqm) {
                           unsigned total_size = 0;
   for (unsigned i = 4; i < instr->operands.size(); i++) {
      check(instr->operands[i].hasRegClass() && instr->operands[i].regClass() == v1,
            }
   check(total_size <= instr->operands[3].bytes(),
      } else {
      check(instr->operands.size() == 4 || program->gfx_level >= GFX10,
         for (unsigned i = 3; i < instr->operands.size(); i++) {
      check(instr->operands[i].hasRegClass() &&
               if (instr->operands.size() > 4) {
      if (program->gfx_level < GFX11) {
      check(instr->operands[i].regClass() == v1,
      } else {
      if (instr->opcode != aco_opcode::image_bvh_intersect_ray &&
      instr->opcode != aco_opcode::image_bvh64_intersect_ray && i < 7) {
   check(instr->operands[i].regClass() == v1,
                           if (instr->definitions.size()) {
      check(instr->definitions[0].regClass().type() == RegType::vgpr,
         check(instr->mimg().d16 || !instr->definitions[0].regClass().is_subdword(),
         check(instr->definitions[0].bytes() <= 8 || !instr->mimg().d16,
      }
      }
   case Format::DS: {
      for (const Operand& op : instr->operands) {
      check(op.isOfType(RegType::vgpr) || op.physReg() == m0 || op.isUndefined(),
      }
   if (!instr->definitions.empty())
      check(instr->definitions[0].regClass().type() == RegType::vgpr,
         }
   case Format::EXP: {
      for (unsigned i = 0; i < 4; i++)
      check(instr->operands[i].isOfType(RegType::vgpr),
         }
   case Format::FLAT:
      check(instr->operands[1].isUndefined(), "Flat instructions don't support SADDR",
            case Format::GLOBAL:
      check(instr->operands[0].isOfType(RegType::vgpr), "FLAT/GLOBAL address must be vgpr",
            case Format::SCRATCH: {
      check(instr->operands[0].isOfType(RegType::vgpr),
         check(instr->operands[1].isOfType(RegType::sgpr),
         if (instr->format == Format::SCRATCH && program->gfx_level < GFX10_3)
      check(!instr->operands[0].isUndefined() || !instr->operands[1].isUndefined(),
      if (!instr->definitions.empty())
      check(instr->definitions[0].regClass().type() == RegType::vgpr,
      else
      check(instr->operands[2].isOfType(RegType::vgpr),
         }
   case Format::LDSDIR: {
      check(instr->definitions.size() == 1 && instr->definitions[0].regClass() == v1,
         check(instr->operands.size() == 1, "LDSDIR must have an operand", instr.get());
   if (!instr->operands.empty()) {
      check(instr->operands[0].regClass() == s1, "LDSDIR must have an s1 operand",
         check(instr->operands[0].isFixed() && instr->operands[0].physReg() == m0,
      }
      }
   default: break;
                     }
      bool
   validate_cfg(Program* program)
   {
      if (!(debug_flags & DEBUG_VALIDATE_IR))
            bool is_valid = true;
   auto check_block = [&program, &is_valid](bool success, const char* msg,
         {
      if (!success) {
      aco_err(program, "%s: BB%u", msg, block->index);
                  /* validate CFG */
   for (unsigned i = 0; i < program->blocks.size(); i++) {
      Block& block = program->blocks[i];
            /* predecessors/successors should be sorted */
   for (unsigned j = 0; j + 1 < block.linear_preds.size(); j++)
      check_block(block.linear_preds[j] < block.linear_preds[j + 1],
      for (unsigned j = 0; j + 1 < block.logical_preds.size(); j++)
      check_block(block.logical_preds[j] < block.logical_preds[j + 1],
      for (unsigned j = 0; j + 1 < block.linear_succs.size(); j++)
      check_block(block.linear_succs[j] < block.linear_succs[j + 1],
      for (unsigned j = 0; j + 1 < block.logical_succs.size(); j++)
                  /* critical edges are not allowed */
   if (block.linear_preds.size() > 1) {
      for (unsigned pred : block.linear_preds)
      check_block(program->blocks[pred].linear_succs.size() == 1,
      for (unsigned pred : block.logical_preds)
      check_block(program->blocks[pred].logical_succs.size() == 1,
                  }
      /* RA validation */
   namespace {
      struct Location {
               Block* block;
      };
      struct Assignment {
      Location defloc;
   Location firstloc;
   PhysReg reg;
      };
      bool
   ra_fail(Program* program, Location loc, Location loc2, const char* fmt, ...)
   {
      va_list args;
   va_start(args, fmt);
   char msg[1024];
   vsprintf(msg, fmt, args);
            char* out;
   size_t outsize;
   struct u_memstream mem;
   u_memstream_open(&mem, &out, &outsize);
            fprintf(memf, "RA error found at instruction in BB%d:\n", loc.block->index);
   if (loc.instr) {
      aco_print_instr(program->gfx_level, loc.instr, memf);
      } else {
         }
   if (loc2.block) {
      fprintf(memf, " in BB%d:\n", loc2.block->index);
      }
   fprintf(memf, "\n\n");
            aco_err(program, "%s", out);
               }
      bool
   validate_subdword_operand(amd_gfx_level gfx_level, const aco_ptr<Instruction>& instr,
         {
      Operand op = instr->operands[index];
            if (instr->opcode == aco_opcode::p_as_uniform)
         if (instr->isPseudo() && gfx_level >= GFX8)
         if (instr->isSDWA())
      return byte + instr->sdwa().sel[index].offset() + instr->sdwa().sel[index].size() <= 4 &&
      if (instr->isVOP3P()) {
      bool fma_mix = instr->opcode == aco_opcode::v_fma_mixlo_f16 ||
               return instr->valu().opsel_lo[index] == (byte >> 1) &&
      }
   if (byte == 2 && can_use_opsel(gfx_level, instr->opcode, index))
            switch (instr->opcode) {
   case aco_opcode::v_cvt_f32_ubyte1:
      if (byte == 1)
            case aco_opcode::v_cvt_f32_ubyte2:
      if (byte == 2)
            case aco_opcode::v_cvt_f32_ubyte3:
      if (byte == 3)
            case aco_opcode::ds_write_b8_d16_hi:
   case aco_opcode::ds_write_b16_d16_hi:
      if (byte == 2 && index == 1)
            case aco_opcode::buffer_store_byte_d16_hi:
   case aco_opcode::buffer_store_short_d16_hi:
   case aco_opcode::buffer_store_format_d16_hi_x:
      if (byte == 2 && index == 3)
            case aco_opcode::flat_store_byte_d16_hi:
   case aco_opcode::flat_store_short_d16_hi:
   case aco_opcode::scratch_store_byte_d16_hi:
   case aco_opcode::scratch_store_short_d16_hi:
   case aco_opcode::global_store_byte_d16_hi:
   case aco_opcode::global_store_short_d16_hi:
      if (byte == 2 && index == 2)
            default: break;
               }
      bool
   validate_subdword_definition(amd_gfx_level gfx_level, const aco_ptr<Instruction>& instr)
   {
      Definition def = instr->definitions[0];
            if (instr->isPseudo() && gfx_level >= GFX8)
         if (instr->isSDWA())
      return byte + instr->sdwa().dst_sel.offset() + instr->sdwa().dst_sel.size() <= 4 &&
      if (byte == 2 && can_use_opsel(gfx_level, instr->opcode, -1))
            switch (instr->opcode) {
   case aco_opcode::v_fma_mixhi_f16:
   case aco_opcode::buffer_load_ubyte_d16_hi:
   case aco_opcode::buffer_load_sbyte_d16_hi:
   case aco_opcode::buffer_load_short_d16_hi:
   case aco_opcode::buffer_load_format_d16_hi_x:
   case aco_opcode::flat_load_ubyte_d16_hi:
   case aco_opcode::flat_load_short_d16_hi:
   case aco_opcode::scratch_load_ubyte_d16_hi:
   case aco_opcode::scratch_load_short_d16_hi:
   case aco_opcode::global_load_ubyte_d16_hi:
   case aco_opcode::global_load_short_d16_hi:
   case aco_opcode::ds_read_u8_d16_hi:
   case aco_opcode::ds_read_u16_d16_hi: return byte == 2;
   default: break;
               }
      unsigned
   get_subdword_bytes_written(Program* program, const aco_ptr<Instruction>& instr, unsigned index)
   {
      amd_gfx_level gfx_level = program->gfx_level;
            if (instr->isPseudo())
         if (instr->isVALU()) {
      assert(def.bytes() <= 2);
   if (instr->isSDWA())
            if (instr_is_16bit(gfx_level, instr->opcode))
                        if (instr->isMIMG()) {
      assert(instr->mimg().d16);
               switch (instr->opcode) {
   case aco_opcode::buffer_load_ubyte_d16:
   case aco_opcode::buffer_load_sbyte_d16:
   case aco_opcode::buffer_load_short_d16:
   case aco_opcode::buffer_load_format_d16_x:
   case aco_opcode::tbuffer_load_format_d16_x:
   case aco_opcode::flat_load_ubyte_d16:
   case aco_opcode::flat_load_short_d16:
   case aco_opcode::scratch_load_ubyte_d16:
   case aco_opcode::scratch_load_short_d16:
   case aco_opcode::global_load_ubyte_d16:
   case aco_opcode::global_load_short_d16:
   case aco_opcode::ds_read_u8_d16:
   case aco_opcode::ds_read_u16_d16:
   case aco_opcode::buffer_load_ubyte_d16_hi:
   case aco_opcode::buffer_load_sbyte_d16_hi:
   case aco_opcode::buffer_load_short_d16_hi:
   case aco_opcode::buffer_load_format_d16_hi_x:
   case aco_opcode::flat_load_ubyte_d16_hi:
   case aco_opcode::flat_load_short_d16_hi:
   case aco_opcode::scratch_load_ubyte_d16_hi:
   case aco_opcode::scratch_load_short_d16_hi:
   case aco_opcode::global_load_ubyte_d16_hi:
   case aco_opcode::global_load_short_d16_hi:
   case aco_opcode::ds_read_u8_d16_hi:
   case aco_opcode::ds_read_u16_d16_hi: return program->dev.sram_ecc_enabled ? 4 : 2;
   case aco_opcode::buffer_load_format_d16_xyz:
   case aco_opcode::tbuffer_load_format_d16_xyz: return program->dev.sram_ecc_enabled ? 8 : 6;
   default: return def.size() * 4;
      }
      bool
   validate_instr_defs(Program* program, std::array<unsigned, 2048>& regs,
               {
               for (unsigned i = 0; i < instr->definitions.size(); i++) {
      Definition& def = instr->definitions[i];
   if (!def.isTemp())
         Temp tmp = def.getTemp();
   PhysReg reg = assignments[tmp.id()].reg;
   for (unsigned j = 0; j < tmp.bytes(); j++) {
      if (regs[reg.reg_b + j])
      err |=
      ra_fail(program, loc, assignments[regs[reg.reg_b + j]].defloc,
            }
   if (def.regClass().is_subdword() && def.bytes() < 4) {
      unsigned written = get_subdword_bytes_written(program, instr, i);
   /* If written=4, the instruction still might write the upper half. In that case, it's
   * the lower half that isn't preserved */
   for (unsigned j = reg.byte() & ~(written - 1); j < written; j++) {
      unsigned written_reg = reg.reg() * 4u + j;
   if (regs[written_reg] && regs[written_reg] != def.tempId())
      err |= ra_fail(program, loc, assignments[regs[written_reg]].defloc,
                              for (const Definition& def : instr->definitions) {
      if (!def.isTemp())
         if (def.isKill()) {
      for (unsigned j = 0; j < def.getTemp().bytes(); j++)
                     }
      } /* end namespace */
      bool
   validate_ra(Program* program)
   {
      if (!(debug_flags & DEBUG_VALIDATE_RA))
            bool err = false;
   aco::live live_vars = aco::live_var_analysis(program);
   std::vector<std::vector<Temp>> phi_sgpr_ops(program->blocks.size());
            std::vector<Assignment> assignments(program->peekAllocationId());
   for (Block& block : program->blocks) {
      Location loc;
   loc.block = &block;
   for (aco_ptr<Instruction>& instr : block.instructions) {
      if (instr->opcode == aco_opcode::p_phi) {
      for (unsigned i = 0; i < instr->operands.size(); i++) {
      if (instr->operands[i].isTemp() &&
      instr->operands[i].getTemp().type() == RegType::sgpr &&
   instr->operands[i].isFirstKill())
               loc.instr = instr.get();
   for (unsigned i = 0; i < instr->operands.size(); i++) {
      Operand& op = instr->operands[i];
   if (!op.isTemp())
         if (!op.isFixed())
         if (assignments[op.tempId()].valid && assignments[op.tempId()].reg != op.physReg())
      err |=
      ra_fail(program, loc, assignments[op.tempId()].firstloc,
   if ((op.getTemp().type() == RegType::vgpr &&
      op.physReg().reg_b + op.bytes() > (256 + program->config->num_vgprs) * 4) ||
   (op.getTemp().type() == RegType::sgpr &&
   op.physReg() + op.size() > program->config->num_sgprs &&
   op.physReg() < sgpr_limit))
   err |= ra_fail(program, loc, assignments[op.tempId()].firstloc,
      if (op.physReg() == vcc && !program->needs_vcc)
      err |= ra_fail(program, loc, Location(),
      if (op.regClass().is_subdword() &&
      !validate_subdword_operand(program->gfx_level, instr, i))
      if (!assignments[op.tempId()].firstloc.block)
         if (!assignments[op.tempId()].defloc.block) {
      assignments[op.tempId()].reg = op.physReg();
                  for (unsigned i = 0; i < instr->definitions.size(); i++) {
      Definition& def = instr->definitions[i];
   if (!def.isTemp())
         if (!def.isFixed())
      err |=
      if (assignments[def.tempId()].defloc.block)
      err |= ra_fail(program, loc, assignments[def.tempId()].defloc,
      if ((def.getTemp().type() == RegType::vgpr &&
      def.physReg().reg_b + def.bytes() > (256 + program->config->num_vgprs) * 4) ||
   (def.getTemp().type() == RegType::sgpr &&
   def.physReg() + def.size() > program->config->num_sgprs &&
   def.physReg() < sgpr_limit))
   err |= ra_fail(program, loc, assignments[def.tempId()].firstloc,
      if (def.physReg() == vcc && !program->needs_vcc)
      err |= ra_fail(program, loc, Location(),
      if (def.regClass().is_subdword() &&
      !validate_subdword_definition(program->gfx_level, instr))
      if (!assignments[def.tempId()].firstloc.block)
         assignments[def.tempId()].defloc = loc;
   assignments[def.tempId()].reg = def.physReg();
                     for (Block& block : program->blocks) {
      Location loc;
            std::array<unsigned, 2048> regs; /* register file in bytes */
            IDSet live = live_vars.live_out[block.index];
   /* remove killed p_phi sgpr operands */
   for (Temp tmp : phi_sgpr_ops[block.index])
            /* check live out */
   for (unsigned id : live) {
      Temp tmp(id, program->temp_rc[id]);
   PhysReg reg = assignments[id].reg;
   for (unsigned i = 0; i < tmp.bytes(); i++) {
      if (regs[reg.reg_b + i]) {
      err |= ra_fail(program, loc, Location(),
            }
         }
            for (auto it = block.instructions.rbegin(); it != block.instructions.rend(); ++it) {
               /* check killed p_phi sgpr operands */
   if (instr->opcode == aco_opcode::p_logical_end) {
      for (Temp tmp : phi_sgpr_ops[block.index]) {
      PhysReg reg = assignments[tmp.id()].reg;
   for (unsigned i = 0; i < tmp.bytes(); i++) {
      if (regs[reg.reg_b + i])
      err |= ra_fail(
      program, loc, Location(),
      }
                  for (const Definition& def : instr->definitions) {
      if (!def.isTemp())
                     /* don't count phi operands as live-in, since they are actually
   * killed when they are copied at the predecessor */
   if (instr->opcode != aco_opcode::p_phi && instr->opcode != aco_opcode::p_linear_phi) {
      for (const Operand& op : instr->operands) {
      if (!op.isTemp())
                           for (unsigned id : live) {
      Temp tmp(id, program->temp_rc[id]);
   PhysReg reg = assignments[id].reg;
   for (unsigned i = 0; i < tmp.bytes(); i++)
               for (aco_ptr<Instruction>& instr : block.instructions) {
               /* remove killed p_phi operands from regs */
   if (instr->opcode == aco_opcode::p_logical_end) {
      for (Temp tmp : phi_sgpr_ops[block.index]) {
      PhysReg reg = assignments[tmp.id()].reg;
   for (unsigned i = 0; i < tmp.bytes(); i++)
                  if (instr->opcode != aco_opcode::p_phi && instr->opcode != aco_opcode::p_linear_phi) {
      for (const Operand& op : instr->operands) {
      if (!op.isTemp())
         if (op.isFirstKillBeforeDef()) {
      for (unsigned j = 0; j < op.getTemp().bytes(); j++)
                                    if (!is_phi(instr)) {
      for (const Operand& op : instr->operands) {
      if (!op.isTemp())
         if (op.isLateKill() && op.isFirstKill()) {
      for (unsigned j = 0; j < op.getTemp().bytes(); j++)
            } else if (block.linear_preds.size() != 1 ||
            for (unsigned pred : block.linear_preds) {
      aco_ptr<Instruction>& br = program->blocks[pred].instructions.back();
   assert(br->isBranch());
                           }
   } // namespace aco
