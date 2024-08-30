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
   #include "aco_util.h"
      #include <unordered_map>
   #include <vector>
      /*
   * Implements the algorithm for dominator-tree value numbering
   * from "Value Numbering" by Briggs, Cooper, and Simpson.
   */
      namespace aco {
   namespace {
      inline uint32_t
   murmur_32_scramble(uint32_t h, uint32_t k)
   {
      k *= 0xcc9e2d51;
   k = (k << 15) | (k >> 17);
   h ^= k * 0x1b873593;
   h = (h << 13) | (h >> 19);
   h = h * 5 + 0xe6546b64;
      }
      template <typename T>
   uint32_t
   hash_murmur_32(Instruction* instr)
   {
               for (const Operand& op : instr->operands)
            /* skip format, opcode and pass_flags */
   for (unsigned i = 2; i < (sizeof(T) >> 2); i++) {
      uint32_t u;
   /* Accesses it though a byte array, so doesn't violate the strict aliasing rule */
   memcpy(&u, reinterpret_cast<uint8_t*>(instr) + i * 4, 4);
               /* Finalize. */
   uint32_t len = instr->operands.size() + instr->definitions.size() + sizeof(T);
   hash ^= len;
   hash ^= hash >> 16;
   hash *= 0x85ebca6b;
   hash ^= hash >> 13;
   hash *= 0xc2b2ae35;
   hash ^= hash >> 16;
      }
      struct InstrHash {
      /* This hash function uses the Murmur3 algorithm written by Austin Appleby
   * https://github.com/aappleby/smhasher/blob/master/src/MurmurHash3.cpp
   *
   * In order to calculate the expression set, only the right-hand-side of an
   * instruction is used for the hash, i.e. everything except the definitions.
   */
   std::size_t operator()(Instruction* instr) const
   {
      if (instr->isDPP16())
            if (instr->isDPP8())
            if (instr->isSDWA())
            if (instr->isVINTERP_INREG())
            if (instr->isVALU())
            switch (instr->format) {
   case Format::SMEM: return hash_murmur_32<SMEM_instruction>(instr);
   case Format::VINTRP: return hash_murmur_32<VINTRP_instruction>(instr);
   case Format::DS: return hash_murmur_32<DS_instruction>(instr);
   case Format::SOPP: return hash_murmur_32<SOPP_instruction>(instr);
   case Format::SOPK: return hash_murmur_32<SOPK_instruction>(instr);
   case Format::EXP: return hash_murmur_32<Export_instruction>(instr);
   case Format::MUBUF: return hash_murmur_32<MUBUF_instruction>(instr);
   case Format::MIMG: return hash_murmur_32<MIMG_instruction>(instr);
   case Format::MTBUF: return hash_murmur_32<MTBUF_instruction>(instr);
   case Format::FLAT: return hash_murmur_32<FLAT_instruction>(instr);
   case Format::PSEUDO_BRANCH: return hash_murmur_32<Pseudo_branch_instruction>(instr);
   case Format::PSEUDO_REDUCTION: return hash_murmur_32<Pseudo_reduction_instruction>(instr);
   default: return hash_murmur_32<Instruction>(instr);
         };
      struct InstrPred {
      bool operator()(Instruction* a, Instruction* b) const
   {
      if (a->format != b->format)
         if (a->opcode != b->opcode)
         if (a->operands.size() != b->operands.size() ||
      a->definitions.size() != b->definitions.size())
      for (unsigned i = 0; i < a->operands.size(); i++) {
      if (a->operands[i].isConstant()) {
      if (!b->operands[i].isConstant())
         if (a->operands[i].constantValue() != b->operands[i].constantValue())
      } else if (a->operands[i].isTemp()) {
      if (!b->operands[i].isTemp())
         if (a->operands[i].tempId() != b->operands[i].tempId())
      } else if (a->operands[i].isUndefined() ^ b->operands[i].isUndefined())
         if (a->operands[i].isFixed()) {
      if (!b->operands[i].isFixed())
         if (a->operands[i].physReg() != b->operands[i].physReg())
         if (a->operands[i].physReg() == exec && a->pass_flags != b->pass_flags)
         }
   for (unsigned i = 0; i < a->definitions.size(); i++) {
      if (a->definitions[i].isTemp()) {
      if (!b->definitions[i].isTemp())
         if (a->definitions[i].regClass() != b->definitions[i].regClass())
      }
   if (a->definitions[i].isFixed()) {
      if (!b->definitions[i].isFixed())
         if (a->definitions[i].physReg() != b->definitions[i].physReg())
         if (a->definitions[i].physReg() == exec)
                  if (a->opcode == aco_opcode::v_readfirstlane_b32)
            if (a->isVALU()) {
      VALU_instruction& aV = a->valu();
   VALU_instruction& bV = b->valu();
   if (aV.abs != bV.abs || aV.neg != bV.neg || aV.clamp != bV.clamp || aV.omod != bV.omod ||
      aV.opsel != bV.opsel || aV.opsel_lo != bV.opsel_lo || aV.opsel_hi != bV.opsel_hi)
   }
   if (a->isDPP16()) {
      DPP16_instruction& aDPP = a->dpp16();
   DPP16_instruction& bDPP = b->dpp16();
   return aDPP.pass_flags == bDPP.pass_flags && aDPP.dpp_ctrl == bDPP.dpp_ctrl &&
            }
   if (a->isDPP8()) {
      DPP8_instruction& aDPP = a->dpp8();
   DPP8_instruction& bDPP = b->dpp8();
   return aDPP.pass_flags == bDPP.pass_flags && aDPP.lane_sel == bDPP.lane_sel &&
      }
   if (a->isSDWA()) {
      SDWA_instruction& aSDWA = a->sdwa();
   SDWA_instruction& bSDWA = b->sdwa();
   return aSDWA.sel[0] == bSDWA.sel[0] && aSDWA.sel[1] == bSDWA.sel[1] &&
               switch (a->format) {
   case Format::SOP1: {
      if (a->opcode == aco_opcode::s_sendmsg_rtn_b32 ||
      a->opcode == aco_opcode::s_sendmsg_rtn_b64)
         }
   case Format::SOPK: {
      if (a->opcode == aco_opcode::s_getreg_b32)
         SOPK_instruction& aK = a->sopk();
   SOPK_instruction& bK = b->sopk();
      }
   case Format::SMEM: {
      SMEM_instruction& aS = a->smem();
   SMEM_instruction& bS = b->smem();
   return aS.sync == bS.sync && aS.glc == bS.glc && aS.dlc == bS.dlc && aS.nv == bS.nv &&
      }
   case Format::VINTRP: {
      VINTRP_instruction& aI = a->vintrp();
   VINTRP_instruction& bI = b->vintrp();
   if (aI.attribute != bI.attribute)
         if (aI.component != bI.component)
            }
   case Format::VINTERP_INREG: {
      VINTERP_inreg_instruction& aI = a->vinterp_inreg();
   VINTERP_inreg_instruction& bI = b->vinterp_inreg();
      }
   case Format::PSEUDO_REDUCTION: {
      Pseudo_reduction_instruction& aR = a->reduction();
   Pseudo_reduction_instruction& bR = b->reduction();
   return aR.pass_flags == bR.pass_flags && aR.reduce_op == bR.reduce_op &&
      }
   case Format::DS: {
      assert(a->opcode == aco_opcode::ds_bpermute_b32 ||
         DS_instruction& aD = a->ds();
   DS_instruction& bD = b->ds();
   return aD.sync == bD.sync && aD.pass_flags == bD.pass_flags && aD.gds == bD.gds &&
      }
   case Format::LDSDIR: {
      LDSDIR_instruction& aD = a->ldsdir();
   LDSDIR_instruction& bD = b->ldsdir();
   return aD.sync == bD.sync && aD.attr == bD.attr && aD.attr_chan == bD.attr_chan &&
      }
   case Format::MTBUF: {
      MTBUF_instruction& aM = a->mtbuf();
   MTBUF_instruction& bM = b->mtbuf();
   return aM.sync == bM.sync && aM.dfmt == bM.dfmt && aM.nfmt == bM.nfmt &&
         aM.offset == bM.offset && aM.offen == bM.offen && aM.idxen == bM.idxen &&
      }
   case Format::MUBUF: {
      MUBUF_instruction& aM = a->mubuf();
   MUBUF_instruction& bM = b->mubuf();
   return aM.sync == bM.sync && aM.offset == bM.offset && aM.offen == bM.offen &&
            }
   case Format::MIMG: {
      MIMG_instruction& aM = a->mimg();
   MIMG_instruction& bM = b->mimg();
   return aM.sync == bM.sync && aM.dmask == bM.dmask && aM.unrm == bM.unrm &&
         aM.glc == bM.glc && aM.slc == bM.slc && aM.tfe == bM.tfe && aM.da == bM.da &&
      }
   case Format::FLAT:
   case Format::GLOBAL:
   case Format::SCRATCH:
   case Format::EXP:
   case Format::SOPP:
   case Format::PSEUDO_BRANCH:
   case Format::PSEUDO_BARRIER: unreachable("unsupported instruction format");
   default: return true;
         };
      using expr_set = aco::unordered_map<Instruction*, uint32_t, InstrHash, InstrPred>;
      struct vn_ctx {
      Program* program;
   monotonic_buffer_resource m;
   expr_set expr_values;
            /* The exec id should be the same on the same level of control flow depth.
   * Together with the check for dominator relations, it is safe to assume
   * that the same exec_id also means the same execution mask.
   * Discards increment the exec_id, so that it won't return to the previous value.
   */
            vn_ctx(Program* program_) : program(program_), m(), expr_values(m), renames(m)
   {
      static_assert(sizeof(Temp) == 4, "Temp must fit in 32bits");
   unsigned size = 0;
   for (Block& block : program->blocks)
               };
      /* dominates() returns true if the parent block dominates the child block and
   * if the parent block is part of the same loop or has a smaller loop nest depth.
   */
   bool
   dominates(vn_ctx& ctx, uint32_t parent, uint32_t child)
   {
      unsigned parent_loop_nest_depth = ctx.program->blocks[parent].loop_nest_depth;
   while (parent < child && parent_loop_nest_depth <= ctx.program->blocks[child].loop_nest_depth)
               }
      /** Returns whether this instruction can safely be removed
   *  and replaced by an equal expression.
   *  This is in particular true for ALU instructions and
   *  read-only memory instructions.
   *
   *  Note that expr_set must not be used with instructions
   *  which cannot be eliminated.
   */
   bool
   can_eliminate(aco_ptr<Instruction>& instr)
   {
      switch (instr->format) {
   case Format::FLAT:
   case Format::GLOBAL:
   case Format::SCRATCH:
   case Format::EXP:
   case Format::SOPP:
   case Format::PSEUDO_BRANCH:
   case Format::PSEUDO_BARRIER: return false;
   case Format::DS:
      return instr->opcode == aco_opcode::ds_bpermute_b32 ||
            case Format::SMEM:
   case Format::MUBUF:
   case Format::MIMG:
   case Format::MTBUF:
      if (!get_sync_info(instr.get()).can_reorder())
            default: break;
            if (instr->definitions.empty() || instr->opcode == aco_opcode::p_phi ||
      instr->opcode == aco_opcode::p_linear_phi ||
   instr->opcode == aco_opcode::p_pops_gfx9_add_exiting_wave_id ||
   instr->definitions[0].isNoCSE())
            }
      void
   process_block(vn_ctx& ctx, Block& block)
   {
      std::vector<aco_ptr<Instruction>> new_instructions;
            for (aco_ptr<Instruction>& instr : block.instructions) {
      /* first, rename operands */
   for (Operand& op : instr->operands) {
      if (!op.isTemp())
         auto it = ctx.renames.find(op.tempId());
   if (it != ctx.renames.end())
               if (instr->opcode == aco_opcode::p_discard_if ||
                  if (!can_eliminate(instr)) {
      new_instructions.emplace_back(std::move(instr));
               /* simple copy-propagation through renaming */
   bool copy_instr =
      instr->opcode == aco_opcode::p_parallelcopy ||
      if (copy_instr && !instr->definitions[0].isFixed() && instr->operands[0].isTemp() &&
      instr->operands[0].regClass() == instr->definitions[0].regClass()) {
   ctx.renames[instr->definitions[0].tempId()] = instr->operands[0].getTemp();
               instr->pass_flags = ctx.exec_id;
            /* if there was already an expression with the same value number */
   if (!res.second) {
      Instruction* orig_instr = res.first->first;
   assert(instr->definitions.size() == orig_instr->definitions.size());
   /* check if the original instruction dominates the current one */
   if (dominates(ctx, res.first->second, block.index) &&
      ctx.program->blocks[res.first->second].fp_mode.canReplace(block.fp_mode)) {
   for (unsigned i = 0; i < instr->definitions.size(); i++) {
      assert(instr->definitions[i].regClass() == orig_instr->definitions[i].regClass());
   assert(instr->definitions[i].isTemp());
   ctx.renames[instr->definitions[i].tempId()] = orig_instr->definitions[i].getTemp();
   if (instr->definitions[i].isPrecise())
         /* SPIR_V spec says that an instruction marked with NUW wrapping
   * around is undefined behaviour, so we can break additions in
   * other contexts.
   */
   if (instr->definitions[i].isNUW())
         } else {
      ctx.expr_values.erase(res.first);
   ctx.expr_values.emplace(instr.get(), block.index);
         } else {
                        }
      void
   rename_phi_operands(Block& block, aco::unordered_map<uint32_t, Temp>& renames)
   {
      for (aco_ptr<Instruction>& phi : block.instructions) {
      if (phi->opcode != aco_opcode::p_phi && phi->opcode != aco_opcode::p_linear_phi)
            for (Operand& op : phi->operands) {
      if (!op.isTemp())
         auto it = renames.find(op.tempId());
   if (it != renames.end())
            }
   } /* end namespace */
      void
   value_numbering(Program* program)
   {
      vn_ctx ctx(program);
            for (Block& block : program->blocks) {
      assert(ctx.exec_id > 0);
   /* decrement exec_id when leaving nested control flow */
   if (block.kind & block_kind_loop_header)
         if (block.kind & block_kind_merge) {
         } else if (block.kind & block_kind_loop_exit) {
      ctx.exec_id -= program->blocks[loop_headers.back()].linear_preds.size();
   ctx.exec_id -= block.linear_preds.size();
               if (block.logical_idom == (int)block.index)
            if (block.logical_idom != -1)
         else
            /* increment exec_id when entering nested control flow */
   if (block.kind & block_kind_branch || block.kind & block_kind_loop_preheader ||
      block.kind & block_kind_break || block.kind & block_kind_continue)
      else if (block.kind & block_kind_continue_or_break)
               /* rename loop header phi operands */
   for (Block& block : program->blocks) {
      if (block.kind & block_kind_loop_header)
         }
      } // namespace aco
