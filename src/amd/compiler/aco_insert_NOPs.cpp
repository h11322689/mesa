   /*
   * Copyright © 2019 Valve Corporation
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
      #include "util/bitset.h"
      #include <algorithm>
   #include <bitset>
   #include <set>
   #include <stack>
   #include <vector>
      namespace aco {
   namespace {
      struct State {
      Program* program;
   Block* block;
      };
      struct NOP_ctx_gfx6 {
      void join(const NOP_ctx_gfx6& other)
   {
      set_vskip_mode_then_vector =
         valu_wr_vcc_then_vccz = MAX2(valu_wr_vcc_then_vccz, other.valu_wr_vcc_then_vccz);
   valu_wr_exec_then_execz = MAX2(valu_wr_exec_then_execz, other.valu_wr_exec_then_execz);
   valu_wr_vcc_then_div_fmas = MAX2(valu_wr_vcc_then_div_fmas, other.valu_wr_vcc_then_div_fmas);
   salu_wr_m0_then_gds_msg_ttrace =
         valu_wr_exec_then_dpp = MAX2(valu_wr_exec_then_dpp, other.valu_wr_exec_then_dpp);
   salu_wr_m0_then_lds = MAX2(salu_wr_m0_then_lds, other.salu_wr_m0_then_lds);
   salu_wr_m0_then_moverel = MAX2(salu_wr_m0_then_moverel, other.salu_wr_m0_then_moverel);
   setreg_then_getsetreg = MAX2(setreg_then_getsetreg, other.setreg_then_getsetreg);
   vmem_store_then_wr_data |= other.vmem_store_then_wr_data;
   smem_clause |= other.smem_clause;
   smem_write |= other.smem_write;
   for (unsigned i = 0; i < BITSET_WORDS(128); i++) {
      smem_clause_read_write[i] |= other.smem_clause_read_write[i];
                  bool operator==(const NOP_ctx_gfx6& other)
   {
      return set_vskip_mode_then_vector == other.set_vskip_mode_then_vector &&
         valu_wr_vcc_then_vccz == other.valu_wr_vcc_then_vccz &&
   valu_wr_exec_then_execz == other.valu_wr_exec_then_execz &&
   valu_wr_vcc_then_div_fmas == other.valu_wr_vcc_then_div_fmas &&
   vmem_store_then_wr_data == other.vmem_store_then_wr_data &&
   salu_wr_m0_then_gds_msg_ttrace == other.salu_wr_m0_then_gds_msg_ttrace &&
   valu_wr_exec_then_dpp == other.valu_wr_exec_then_dpp &&
   salu_wr_m0_then_lds == other.salu_wr_m0_then_lds &&
   salu_wr_m0_then_moverel == other.salu_wr_m0_then_moverel &&
   setreg_then_getsetreg == other.setreg_then_getsetreg &&
   smem_clause == other.smem_clause && smem_write == other.smem_write &&
               void add_wait_states(unsigned amount)
   {
      if ((set_vskip_mode_then_vector -= amount) < 0)
            if ((valu_wr_vcc_then_vccz -= amount) < 0)
            if ((valu_wr_exec_then_execz -= amount) < 0)
            if ((valu_wr_vcc_then_div_fmas -= amount) < 0)
            if ((salu_wr_m0_then_gds_msg_ttrace -= amount) < 0)
            if ((valu_wr_exec_then_dpp -= amount) < 0)
            if ((salu_wr_m0_then_lds -= amount) < 0)
            if ((salu_wr_m0_then_moverel -= amount) < 0)
            if ((setreg_then_getsetreg -= amount) < 0)
                        /* setting MODE.vskip and then any vector op requires 2 wait states */
            /* VALU writing VCC/EXEC and then a VALU reading VCCZ/EXECZ requires 5 wait states */
   int8_t valu_wr_vcc_then_vccz = 0;
            /* VALU writing VCC followed by v_div_fmas require 4 wait states */
            /* SALU writing M0 followed by GDS, s_sendmsg or s_ttrace_data requires 1 wait state */
            /* VALU writing EXEC followed by DPP requires 5 wait states */
            /* SALU writing M0 followed by some LDS instructions requires 1 wait state on GFX10 */
            /* SALU writing M0 followed by s_moverel requires 1 wait state on GFX9 */
            /* s_setreg followed by a s_getreg/s_setreg of the same register needs 2 wait states
   * currently we don't look at the actual register */
            /* some memory instructions writing >64bit followed by a instructions
   * writing the VGPRs holding the writedata requires 1 wait state */
            /* we break up SMEM clauses that contain stores or overwrite an
   * operand/definition of another instruction in the clause */
   bool smem_clause = false;
   bool smem_write = false;
   BITSET_DECLARE(smem_clause_read_write, 128) = {0};
      };
      struct NOP_ctx_gfx10 {
      bool has_VOPC_write_exec = false;
   bool has_nonVALU_exec_read = false;
   bool has_VMEM = false;
   bool has_branch_after_VMEM = false;
   bool has_DS = false;
   bool has_branch_after_DS = false;
   bool has_NSA_MIMG = false;
   bool has_writelane = false;
   std::bitset<128> sgprs_read_by_VMEM;
   std::bitset<128> sgprs_read_by_VMEM_store;
   std::bitset<128> sgprs_read_by_DS;
            void join(const NOP_ctx_gfx10& other)
   {
      has_VOPC_write_exec |= other.has_VOPC_write_exec;
   has_nonVALU_exec_read |= other.has_nonVALU_exec_read;
   has_VMEM |= other.has_VMEM;
   has_branch_after_VMEM |= other.has_branch_after_VMEM;
   has_DS |= other.has_DS;
   has_branch_after_DS |= other.has_branch_after_DS;
   has_NSA_MIMG |= other.has_NSA_MIMG;
   has_writelane |= other.has_writelane;
   sgprs_read_by_VMEM |= other.sgprs_read_by_VMEM;
   sgprs_read_by_DS |= other.sgprs_read_by_DS;
   sgprs_read_by_VMEM_store |= other.sgprs_read_by_VMEM_store;
               bool operator==(const NOP_ctx_gfx10& other)
   {
      return has_VOPC_write_exec == other.has_VOPC_write_exec &&
         has_nonVALU_exec_read == other.has_nonVALU_exec_read && has_VMEM == other.has_VMEM &&
   has_branch_after_VMEM == other.has_branch_after_VMEM && has_DS == other.has_DS &&
   has_branch_after_DS == other.has_branch_after_DS &&
   has_NSA_MIMG == other.has_NSA_MIMG && has_writelane == other.has_writelane &&
   sgprs_read_by_VMEM == other.sgprs_read_by_VMEM &&
   sgprs_read_by_DS == other.sgprs_read_by_DS &&
         };
      template <int Max> struct VGPRCounterMap {
   public:
      int base = 0;
   BITSET_DECLARE(resident, 256);
            /* Initializes all counters to Max. */
            /* Increase all counters, clamping at Max. */
            /* Set counter to 0. */
   void set(unsigned idx)
   {
      val[idx] = -base;
               void set(PhysReg reg, unsigned bytes)
   {
      if (reg.reg() < 256)
            for (unsigned i = 0; i < DIV_ROUND_UP(bytes, 4); i++)
               /* Reset all counters to Max. */
   void reset()
   {
      base = 0;
               void reset(PhysReg reg, unsigned bytes)
   {
      if (reg.reg() < 256)
            for (unsigned i = 0; i < DIV_ROUND_UP(bytes, 4); i++)
               uint8_t get(unsigned idx)
   {
                  uint8_t get(PhysReg reg, unsigned offset = 0)
   {
      assert(reg.reg() >= 256);
               void join_min(const VGPRCounterMap& other)
   {
      unsigned i;
   BITSET_FOREACH_SET (i, other.resident, 256) {
      if (BITSET_TEST(resident, i))
         else
      }
               bool operator==(const VGPRCounterMap& other) const
   {
      if (!BITSET_EQUAL(resident, other.resident))
            unsigned i;
   BITSET_FOREACH_SET (i, other.resident, 256) {
      if (!BITSET_TEST(resident, i))
         if (val[i] + base != other.val[i] + other.base)
      }
         };
      struct NOP_ctx_gfx11 {
      /* VcmpxPermlaneHazard */
            /* LdsDirectVMEMHazard */
   std::bitset<256> vgpr_used_by_vmem_load;
   std::bitset<256> vgpr_used_by_vmem_store;
            /* VALUTransUseHazard */
   VGPRCounterMap<15> valu_since_wr_by_trans;
            /* VALUMaskWriteHazard */
   std::bitset<128> sgpr_read_by_valu_as_lanemask;
            void join(const NOP_ctx_gfx11& other)
   {
      has_Vcmpx |= other.has_Vcmpx;
   vgpr_used_by_vmem_load |= other.vgpr_used_by_vmem_load;
   vgpr_used_by_vmem_store |= other.vgpr_used_by_vmem_store;
   vgpr_used_by_ds |= other.vgpr_used_by_ds;
   valu_since_wr_by_trans.join_min(other.valu_since_wr_by_trans);
   trans_since_wr_by_trans.join_min(other.trans_since_wr_by_trans);
   sgpr_read_by_valu_as_lanemask |= other.sgpr_read_by_valu_as_lanemask;
   sgpr_read_by_valu_as_lanemask_then_wr_by_salu |=
               bool operator==(const NOP_ctx_gfx11& other)
   {
      return has_Vcmpx == other.has_Vcmpx &&
         vgpr_used_by_vmem_load == other.vgpr_used_by_vmem_load &&
   vgpr_used_by_vmem_store == other.vgpr_used_by_vmem_store &&
   vgpr_used_by_ds == other.vgpr_used_by_ds &&
   valu_since_wr_by_trans == other.valu_since_wr_by_trans &&
   trans_since_wr_by_trans == other.trans_since_wr_by_trans &&
   sgpr_read_by_valu_as_lanemask == other.sgpr_read_by_valu_as_lanemask &&
         };
      int
   get_wait_states(aco_ptr<Instruction>& instr)
   {
      if (instr->opcode == aco_opcode::s_nop)
         else if (instr->opcode == aco_opcode::p_constaddr)
         else
      }
      bool
   regs_intersect(PhysReg a_reg, unsigned a_size, PhysReg b_reg, unsigned b_size)
   {
         }
      template <typename GlobalState, typename BlockState,
               void
   search_backwards_internal(State& state, GlobalState& global_state, BlockState block_state,
         {
      if (block == state.block && start_at_end) {
      /* If it's the current block, block->instructions is incomplete. */
   for (int pred_idx = state.old_instructions.size() - 1; pred_idx >= 0; pred_idx--) {
      aco_ptr<Instruction>& instr = state.old_instructions[pred_idx];
   if (!instr)
         if (instr_cb(global_state, block_state, instr))
                  for (int pred_idx = block->instructions.size() - 1; pred_idx >= 0; pred_idx--) {
      if (instr_cb(global_state, block_state, block->instructions[pred_idx]))
               PRAGMA_DIAGNOSTIC_PUSH
   PRAGMA_DIAGNOSTIC_IGNORED(-Waddress)
   if (block_cb != nullptr && !block_cb(global_state, block_state, block))
                  for (unsigned lin_pred : block->linear_preds) {
      search_backwards_internal<GlobalState, BlockState, block_cb, instr_cb>(
         }
      template <typename GlobalState, typename BlockState,
               void
   search_backwards(State& state, GlobalState& global_state, BlockState& block_state)
   {
      search_backwards_internal<GlobalState, BlockState, block_cb, instr_cb>(
      }
      struct HandleRawHazardGlobalState {
      PhysReg reg;
      };
      struct HandleRawHazardBlockState {
      uint32_t mask;
      };
      template <bool Valu, bool Vintrp, bool Salu>
   bool
   handle_raw_hazard_instr(HandleRawHazardGlobalState& global_state,
         {
               uint32_t writemask = 0;
   for (Definition& def : pred->definitions) {
      if (regs_intersect(global_state.reg, mask_size, def.physReg(), def.size())) {
      unsigned start = def.physReg() > global_state.reg ? def.physReg() - global_state.reg : 0;
   unsigned end = MIN2(mask_size, start + def.size());
                  bool is_hazard = writemask != 0 && ((pred->isVALU() && Valu) || (pred->isVINTRP() && Vintrp) ||
         if (is_hazard) {
      global_state.nops_needed = MAX2(global_state.nops_needed, block_state.nops_needed);
               block_state.mask &= ~writemask;
            if (block_state.mask == 0)
               }
      template <bool Valu, bool Vintrp, bool Salu>
   void
   handle_raw_hazard(State& state, int* NOPs, int min_states, Operand op)
   {
      if (*NOPs >= min_states)
            HandleRawHazardGlobalState global = {op.physReg(), 0};
            /* Loops require branch instructions, which count towards the wait
   * states. So even with loops this should finish unless nops_needed is some
   * huge value. */
   search_backwards<HandleRawHazardGlobalState, HandleRawHazardBlockState, nullptr,
               }
      static auto handle_valu_then_read_hazard = handle_raw_hazard<true, true, false>;
   static auto handle_vintrp_then_read_hazard = handle_raw_hazard<false, true, false>;
   static auto handle_valu_salu_then_read_hazard = handle_raw_hazard<true, true, true>;
      void
   set_bitset_range(BITSET_WORD* words, unsigned start, unsigned size)
   {
      unsigned end = start + size - 1;
   unsigned start_mod = start % BITSET_WORDBITS;
   if (start_mod + size <= BITSET_WORDBITS) {
         } else {
      unsigned first_size = BITSET_WORDBITS - start_mod;
   set_bitset_range(words, start, BITSET_WORDBITS - start_mod);
         }
      bool
   test_bitset_range(BITSET_WORD* words, unsigned start, unsigned size)
   {
      unsigned end = start + size - 1;
   unsigned start_mod = start % BITSET_WORDBITS;
   if (start_mod + size <= BITSET_WORDBITS) {
         } else {
      unsigned first_size = BITSET_WORDBITS - start_mod;
   return test_bitset_range(words, start, BITSET_WORDBITS - start_mod) ||
         }
      /* A SMEM clause is any group of consecutive SMEM instructions. The
   * instructions in this group may return out of order and/or may be replayed.
   *
   * To fix this potential hazard correctly, we have to make sure that when a
   * clause has more than one instruction, no instruction in the clause writes
   * to a register that is read by another instruction in the clause (including
   * itself). In this case, we have to break the SMEM clause by inserting non
   * SMEM instructions.
   *
   * SMEM clauses are only present on GFX8+, and only matter when XNACK is set.
   */
   void
   handle_smem_clause_hazards(Program* program, NOP_ctx_gfx6& ctx, aco_ptr<Instruction>& instr,
         {
      /* break off from previous SMEM clause if needed */
   if (!*NOPs & (ctx.smem_clause || ctx.smem_write)) {
      /* Don't allow clauses with store instructions since the clause's
   * instructions may use the same address. */
   if (ctx.smem_write || instr->definitions.empty() ||
      instr_info.is_atomic[(unsigned)instr->opcode]) {
      } else if (program->dev.xnack_enabled) {
      for (Operand op : instr->operands) {
      if (!op.isConstant() &&
      test_bitset_range(ctx.smem_clause_write, op.physReg(), op.size())) {
   *NOPs = 1;
                  Definition def = instr->definitions[0];
   if (!*NOPs && test_bitset_range(ctx.smem_clause_read_write, def.physReg(), def.size()))
            }
      /* TODO: we don't handle accessing VCC using the actual SGPR instead of using the alias */
   void
   handle_instruction_gfx6(State& state, NOP_ctx_gfx6& ctx, aco_ptr<Instruction>& instr,
         {
      /* check hazards */
            if (instr->isSMEM()) {
      if (state.program->gfx_level == GFX6) {
      /* A read of an SGPR by SMRD instruction requires 4 wait states
   * when the SGPR was written by a VALU instruction. According to LLVM,
   * there is also an undocumented hardware behavior when the buffer
   * descriptor is written by a SALU instruction */
   for (unsigned i = 0; i < instr->operands.size(); i++) {
      Operand op = instr->operands[i];
                  bool is_buffer_desc = i == 0 && op.size() > 2;
   if (is_buffer_desc)
         else
                     } else if (instr->isSALU()) {
      if (instr->opcode == aco_opcode::s_setreg_b32 ||
      instr->opcode == aco_opcode::s_setreg_imm32_b32 ||
   instr->opcode == aco_opcode::s_getreg_b32) {
               if (state.program->gfx_level == GFX9) {
      if (instr->opcode == aco_opcode::s_movrels_b32 ||
      instr->opcode == aco_opcode::s_movrels_b64 ||
   instr->opcode == aco_opcode::s_movreld_b32 ||
   instr->opcode == aco_opcode::s_movreld_b64) {
                  if (instr->opcode == aco_opcode::s_sendmsg || instr->opcode == aco_opcode::s_ttracedata)
      } else if (instr->isDS() && instr->ds().gds) {
         } else if (instr->isVALU() || instr->isVINTRP()) {
      for (Operand op : instr->operands) {
      if (op.physReg() == vccz)
         if (op.physReg() == execz)
               if (instr->isDPP()) {
      NOPs = MAX2(NOPs, ctx.valu_wr_exec_then_dpp);
               for (Definition def : instr->definitions) {
      if (def.regClass().type() != RegType::sgpr) {
      for (unsigned i = 0; i < def.size(); i++)
                  if ((instr->opcode == aco_opcode::v_readlane_b32 ||
      instr->opcode == aco_opcode::v_readlane_b32_e64 ||
   instr->opcode == aco_opcode::v_writelane_b32 ||
   instr->opcode == aco_opcode::v_writelane_b32_e64) &&
   !instr->operands[1].isConstant()) {
               /* It's required to insert 1 wait state if the dst VGPR of any v_interp_*
   * is followed by a read with v_readfirstlane or v_readlane to fix GPU
   * hangs on GFX6. Note that v_writelane_* is apparently not affected.
   * This hazard isn't documented anywhere but AMD confirmed that hazard.
   */
   if (state.program->gfx_level == GFX6 &&
      (instr->opcode == aco_opcode::v_readlane_b32 || /* GFX6 doesn't have v_readlane_b32_e64 */
   instr->opcode == aco_opcode::v_readfirstlane_b32)) {
               if (instr->opcode == aco_opcode::v_div_fmas_f32 ||
      instr->opcode == aco_opcode::v_div_fmas_f64)
   } else if (instr->isVMEM() || instr->isFlatLike()) {
      /* If the VALU writes the SGPR that is used by a VMEM, the user must add five wait states. */
   for (Operand op : instr->operands) {
      if (!op.isConstant() && !op.isUndefined() && op.regClass().type() == RegType::sgpr)
                  if (!instr->isSALU() && instr->format != Format::SMEM)
            if (state.program->gfx_level == GFX9) {
      bool lds_scratch_global = (instr->isScratch() || instr->isGlobal()) && instr->flatlike().lds;
   if (instr->isVINTRP() || lds_scratch_global ||
      instr->opcode == aco_opcode::ds_read_addtid_b32 ||
   instr->opcode == aco_opcode::ds_write_addtid_b32 ||
   instr->opcode == aco_opcode::buffer_store_lds_dword) {
                           // TODO: try to schedule the NOP-causing instruction up to reduce the number of stall cycles
   if (NOPs) {
      /* create NOP */
   aco_ptr<SOPP_instruction> nop{
         nop->imm = NOPs - 1;
   nop->block = -1;
               /* update information to check for later hazards */
   if ((ctx.smem_clause || ctx.smem_write) && (NOPs || instr->format != Format::SMEM)) {
      ctx.smem_clause = false;
            if (state.program->dev.xnack_enabled) {
      BITSET_ZERO(ctx.smem_clause_read_write);
                  if (instr->isSMEM()) {
      if (instr->definitions.empty() || instr_info.is_atomic[(unsigned)instr->opcode]) {
         } else {
               if (state.program->dev.xnack_enabled) {
      for (Operand op : instr->operands) {
      if (!op.isConstant()) {
                     Definition def = instr->definitions[0];
   set_bitset_range(ctx.smem_clause_read_write, def.physReg(), def.size());
            } else if (instr->isVALU()) {
      for (Definition def : instr->definitions) {
      if (def.regClass().type() == RegType::sgpr) {
      if (def.physReg() == vcc || def.physReg() == vcc_hi) {
      ctx.valu_wr_vcc_then_vccz = 5;
      }
   if (def.physReg() == exec || def.physReg() == exec_hi) {
      ctx.valu_wr_exec_then_execz = 5;
               } else if (instr->isSALU()) {
      if (!instr->definitions.empty()) {
      /* all other definitions should be SCC */
   Definition def = instr->definitions[0];
   if (def.physReg() == m0) {
      ctx.salu_wr_m0_then_gds_msg_ttrace = 1;
   ctx.salu_wr_m0_then_lds = 1;
         } else if (instr->opcode == aco_opcode::s_setreg_b32 ||
            SOPK_instruction& sopk = instr->sopk();
   unsigned offset = (sopk.imm >> 6) & 0x1f;
   unsigned size = ((sopk.imm >> 11) & 0x1f) + 1;
                  if (reg == 1 && offset >= 28 && size > (28 - offset))
         } else if (instr->isVMEM() || instr->isFlatLike()) {
      /* >64-bit MUBUF/MTBUF store with a constant in SOFFSET */
   bool consider_buf = (instr->isMUBUF() || instr->isMTBUF()) && instr->operands.size() == 4 &&
         /* MIMG store with a 128-bit T# with more than two bits set in dmask (making it a >64-bit
   * store) */
   bool consider_mimg = instr->isMIMG() &&
               /* FLAT/GLOBAL/SCRATCH store with >64-bit data */
   bool consider_flat =
         if (consider_buf || consider_mimg || consider_flat) {
      PhysReg wrdata = instr->operands[consider_flat ? 2 : 3].physReg();
   unsigned size = instr->operands[consider_flat ? 2 : 3].size();
   for (unsigned i = 0; i < size; i++)
            }
      bool
   is_latest_instr_vintrp(bool& global_state, bool& block_state, aco_ptr<Instruction>& pred)
   {
      if (pred->isVINTRP())
            }
      template <bool Salu, bool Sgpr>
   bool
   handle_wr_hazard_instr(int& global_state, int& block_state, aco_ptr<Instruction>& pred)
   {
      if (Salu ? pred->isSALU() : (pred->isVALU() || pred->isVINTRP())) {
      for (Definition dst : pred->definitions) {
      if ((dst.physReg().reg() < 256) == Sgpr) {
      global_state = MAX2(global_state, block_state);
                     block_state -= get_wait_states(pred);
      }
      template <bool Salu, bool Sgpr>
   void
   handle_wr_hazard(State& state, int* NOPs, int min_states)
   {
      if (*NOPs >= min_states)
            int global = 0;
   int block = min_states;
   search_backwards<int, int, nullptr, handle_wr_hazard_instr<Salu, Sgpr>>(state, global, block);
      }
      void
   resolve_all_gfx6(State& state, NOP_ctx_gfx6& ctx,
         {
               /* SGPR->SMEM hazards */
   if (state.program->gfx_level == GFX6) {
      handle_wr_hazard<true, true>(state, &NOPs, 4);
               /* Break up SMEM clauses */
   if (ctx.smem_clause || ctx.smem_write)
            /* SALU/GDS hazards */
   NOPs = MAX2(NOPs, ctx.setreg_then_getsetreg);
   if (state.program->gfx_level == GFX9)
                  /* VALU hazards */
   NOPs = MAX2(NOPs, ctx.valu_wr_vcc_then_vccz);
   NOPs = MAX2(NOPs, ctx.valu_wr_exec_then_execz);
   NOPs = MAX2(NOPs, ctx.valu_wr_exec_then_dpp);
   if (state.program->gfx_level >= GFX8)
         NOPs = MAX2(NOPs, ctx.vmem_store_then_wr_data.any() ? 1 : 0);
   if (state.program->gfx_level == GFX6) {
      /* VINTRP->v_readlane_b32/etc */
   bool vintrp = false;
   search_backwards<bool, bool, nullptr, is_latest_instr_vintrp>(state, vintrp, vintrp);
   if (vintrp)
      }
            /* VALU(sgpr)->VMEM/v_readlane_b32/etc hazards. v_readlane_b32/etc require only 4 NOPs. */
                     if (state.program->gfx_level == GFX9)
            ctx.add_wait_states(NOPs);
   if (NOPs) {
      Builder bld(state.program, &new_instructions);
         }
      template <std::size_t N>
   bool
   check_written_regs(const aco_ptr<Instruction>& instr, const std::bitset<N>& check_regs)
   {
      return std::any_of(instr->definitions.begin(), instr->definitions.end(),
                     [&check_regs](const Definition& def) -> bool
   {
      bool writes_any = false;
   for (unsigned i = 0; i < def.size(); i++) {
         }
      template <std::size_t N>
   bool
   check_read_regs(const aco_ptr<Instruction>& instr, const std::bitset<N>& check_regs)
   {
      return std::any_of(instr->operands.begin(), instr->operands.end(),
                     [&check_regs](const Operand& op) -> bool
   {
      if (op.isConstant())
         bool writes_any = false;
   for (unsigned i = 0; i < op.size(); i++) {
         }
      template <std::size_t N>
   void
   mark_read_regs(const aco_ptr<Instruction>& instr, std::bitset<N>& reg_reads)
   {
      for (const Operand& op : instr->operands) {
      for (unsigned i = 0; i < op.size(); i++) {
      unsigned reg = op.physReg() + i;
   if (reg < reg_reads.size())
            }
      template <std::size_t N>
   void
   mark_read_regs_exec(State& state, const aco_ptr<Instruction>& instr, std::bitset<N>& reg_reads)
   {
      mark_read_regs(instr, reg_reads);
   reg_reads.set(exec);
   if (state.program->wave_size == 64)
      }
      bool
   VALU_writes_sgpr(aco_ptr<Instruction>& instr)
   {
      if (instr->isVOPC())
         if (instr->isVOP3() && instr->definitions.size() == 2)
         if (instr->opcode == aco_opcode::v_readfirstlane_b32 ||
      instr->opcode == aco_opcode::v_readlane_b32 ||
   instr->opcode == aco_opcode::v_readlane_b32_e64)
         }
      bool
   instr_writes_sgpr(const aco_ptr<Instruction>& instr)
   {
      return std::any_of(instr->definitions.begin(), instr->definitions.end(),
            }
      inline bool
   instr_is_branch(const aco_ptr<Instruction>& instr)
   {
      return instr->opcode == aco_opcode::s_branch || instr->opcode == aco_opcode::s_cbranch_scc0 ||
         instr->opcode == aco_opcode::s_cbranch_scc1 ||
   instr->opcode == aco_opcode::s_cbranch_vccz ||
   instr->opcode == aco_opcode::s_cbranch_vccnz ||
   instr->opcode == aco_opcode::s_cbranch_execz ||
   instr->opcode == aco_opcode::s_cbranch_execnz ||
   instr->opcode == aco_opcode::s_cbranch_cdbgsys ||
   instr->opcode == aco_opcode::s_cbranch_cdbguser ||
   instr->opcode == aco_opcode::s_cbranch_cdbgsys_or_user ||
   instr->opcode == aco_opcode::s_cbranch_cdbgsys_and_user ||
   instr->opcode == aco_opcode::s_subvector_loop_begin ||
   instr->opcode == aco_opcode::s_subvector_loop_end ||
      }
      void
   handle_instruction_gfx10(State& state, NOP_ctx_gfx10& ctx, aco_ptr<Instruction>& instr,
         {
                        unsigned vm_vsrc = 7;
   unsigned sa_sdst = 1;
   if (debug_flags & DEBUG_FORCE_WAITDEPS) {
      bld.sopp(aco_opcode::s_waitcnt_depctr, -1, 0x0000);
   vm_vsrc = 0;
      } else if (instr->opcode == aco_opcode::s_waitcnt_depctr) {
      vm_vsrc = (instr->sopp().imm >> 2) & 0x7;
               /* VMEMtoScalarWriteHazard
   * Handle EXEC/M0/SGPR write following a VMEM/DS instruction without a VALU or "waitcnt vmcnt(0)"
   * in-between.
   */
   if (instr->isVMEM() || instr->isFlatLike() || instr->isDS()) {
      /* Remember all SGPRs that are read by the VMEM/DS instruction */
   if (instr->isVMEM() || instr->isFlatLike())
      mark_read_regs_exec(
      state, instr,
   if (instr->isFlat() || instr->isDS())
      } else if (instr->isSALU() || instr->isSMEM()) {
      if (instr->opcode == aco_opcode::s_waitcnt) {
      wait_imm imm(state.program->gfx_level, instr->sopp().imm);
   if (imm.vm == 0)
         if (imm.lgkm == 0)
      } else if (instr->opcode == aco_opcode::s_waitcnt_vscnt && instr->sopk().imm == 0) {
         } else if (vm_vsrc == 0) {
      ctx.sgprs_read_by_VMEM.reset();
   ctx.sgprs_read_by_DS.reset();
               /* Check if SALU writes an SGPR that was previously read by the VALU */
   if (check_written_regs(instr, ctx.sgprs_read_by_VMEM) ||
      check_written_regs(instr, ctx.sgprs_read_by_DS) ||
   check_written_regs(instr, ctx.sgprs_read_by_VMEM_store)) {
   ctx.sgprs_read_by_VMEM.reset();
                  /* Insert s_waitcnt_depctr instruction with magic imm to mitigate the problem */
         } else if (instr->isVALU()) {
      /* Hazard is mitigated by any VALU instruction */
   ctx.sgprs_read_by_VMEM.reset();
   ctx.sgprs_read_by_DS.reset();
               /* VcmpxPermlaneHazard
   * Handle any permlane following a VOPC instruction writing exec, insert v_mov between them.
   */
   if (instr->isVOPC() && instr->definitions[0].physReg() == exec) {
      /* we only need to check definitions[0] because since GFX10 v_cmpx only writes one dest */
      } else if (ctx.has_VOPC_write_exec && (instr->opcode == aco_opcode::v_permlane16_b32 ||
                     /* v_nop would be discarded by SQ, so use v_mov with the first operand of the permlane */
   bld.vop1(aco_opcode::v_mov_b32, Definition(instr->operands[0].physReg(), v1),
      } else if (instr->isVALU() && instr->opcode != aco_opcode::v_nop) {
                  /* VcmpxExecWARHazard
   * Handle any VALU instruction writing the exec mask after it was read by a non-VALU instruction.
   */
   if (!instr->isVALU() && instr->reads_exec()) {
         } else if (instr->isVALU() && ctx.has_nonVALU_exec_read) {
      if (instr->writes_exec()) {
               /* Insert s_waitcnt_depctr instruction with magic imm to mitigate the problem */
      } else if (instr_writes_sgpr(instr)) {
      /* Any VALU instruction that writes an SGPR mitigates the problem */
         } else if (sa_sdst == 0) {
                  /* SMEMtoVectorWriteHazard
   * Handle any VALU instruction writing an SGPR after an SMEM reads it.
   */
   if (instr->isSMEM()) {
      /* Remember all SGPRs that are read by the SMEM instruction */
      } else if (VALU_writes_sgpr(instr)) {
      /* Check if VALU writes an SGPR that was previously read by SMEM */
   if (check_written_regs(instr, ctx.sgprs_read_by_SMEM)) {
               /* Insert s_mov to mitigate the problem */
         } else if (instr->isSALU()) {
      if (instr->format != Format::SOPP) {
      /* SALU can mitigate the hazard */
      } else {
      /* Reducing lgkmcnt count to 0 always mitigates the hazard. */
   const SOPP_instruction& sopp = instr->sopp();
   if (sopp.opcode == aco_opcode::s_waitcnt_lgkmcnt) {
      if (sopp.imm == 0 && sopp.definitions[0].physReg() == sgpr_null)
      } else if (sopp.opcode == aco_opcode::s_waitcnt) {
      wait_imm imm(state.program->gfx_level, instr->sopp().imm);
   if (imm.lgkm == 0)
                     /* LdsBranchVmemWARHazard
   * Handle VMEM/GLOBAL/SCRATCH->branch->DS and DS->branch->VMEM/GLOBAL/SCRATCH patterns.
   */
   if (instr->isVMEM() || instr->isGlobal() || instr->isScratch()) {
      if (ctx.has_branch_after_DS)
         ctx.has_branch_after_VMEM = ctx.has_branch_after_DS = ctx.has_DS = false;
      } else if (instr->isDS()) {
      if (ctx.has_branch_after_VMEM)
         ctx.has_branch_after_VMEM = ctx.has_branch_after_DS = ctx.has_VMEM = false;
      } else if (instr_is_branch(instr)) {
      ctx.has_branch_after_VMEM |= ctx.has_VMEM;
   ctx.has_branch_after_DS |= ctx.has_DS;
      } else if (instr->opcode == aco_opcode::s_waitcnt_vscnt) {
      /* Only s_waitcnt_vscnt can mitigate the hazard */
   const SOPK_instruction& sopk = instr->sopk();
   if (sopk.definitions[0].physReg() == sgpr_null && sopk.imm == 0)
               /* NSAToVMEMBug
   * Handles NSA MIMG (4 or more dwords) immediately followed by MUBUF/MTBUF (with offset[2:1] !=
   * 0).
   */
   if (instr->isMIMG() && get_mimg_nsa_dwords(instr.get()) > 1) {
         } else if (ctx.has_NSA_MIMG) {
               if (instr->isMUBUF() || instr->isMTBUF()) {
      uint32_t offset = instr->isMUBUF() ? instr->mubuf().offset : instr->mtbuf().offset;
   if (offset & 6)
                  /* waNsaCannotFollowWritelane
   * Handles NSA MIMG immediately following a v_writelane_b32.
   */
   if (instr->opcode == aco_opcode::v_writelane_b32_e64) {
         } else if (ctx.has_writelane) {
      ctx.has_writelane = false;
   if (instr->isMIMG() && get_mimg_nsa_dwords(instr.get()) > 0)
         }
      void
   resolve_all_gfx10(State& state, NOP_ctx_gfx10& ctx,
         {
                        /* VcmpxPermlaneHazard */
   if (ctx.has_VOPC_write_exec) {
      ctx.has_VOPC_write_exec = false;
            /* VALU mitigates VMEMtoScalarWriteHazard. */
   ctx.sgprs_read_by_VMEM.reset();
   ctx.sgprs_read_by_DS.reset();
                        /* VMEMtoScalarWriteHazard */
   if (ctx.sgprs_read_by_VMEM.any() || ctx.sgprs_read_by_DS.any() ||
      ctx.sgprs_read_by_VMEM_store.any()) {
   ctx.sgprs_read_by_VMEM.reset();
   ctx.sgprs_read_by_DS.reset();
   ctx.sgprs_read_by_VMEM_store.reset();
               /* VcmpxExecWARHazard */
   if (ctx.has_nonVALU_exec_read) {
      ctx.has_nonVALU_exec_read = false;
               if (waitcnt_depctr != 0xffff)
            /* SMEMtoVectorWriteHazard */
   if (ctx.sgprs_read_by_SMEM.any()) {
      ctx.sgprs_read_by_SMEM.reset();
               /* LdsBranchVmemWARHazard */
   if (ctx.has_VMEM || ctx.has_branch_after_VMEM || ctx.has_DS || ctx.has_branch_after_DS) {
      bld.sopk(aco_opcode::s_waitcnt_vscnt, Definition(sgpr_null, s1), 0);
               /* NSAToVMEMBug/waNsaCannotFollowWritelane */
   if (ctx.has_NSA_MIMG || ctx.has_writelane) {
      ctx.has_NSA_MIMG = ctx.has_writelane = false;
   /* Any instruction resolves these hazards. */
   if (new_instructions.size() == prev_count)
         }
      void
   fill_vgpr_bitset(std::bitset<256>& set, PhysReg reg, unsigned bytes)
   {
      if (reg.reg() < 256)
         for (unsigned i = 0; i < DIV_ROUND_UP(bytes, 4); i++)
      }
      /* GFX11 */
   unsigned
   parse_vdst_wait(aco_ptr<Instruction>& instr)
   {
      if (instr->isVMEM() || instr->isFlatLike() || instr->isDS() || instr->isEXP())
         else if (instr->isLDSDIR())
         else if (instr->opcode == aco_opcode::s_waitcnt_depctr)
         else
      }
      struct LdsDirectVALUHazardGlobalState {
      unsigned wait_vdst = 15;
   PhysReg vgpr;
      };
      struct LdsDirectVALUHazardBlockState {
      unsigned num_valu = 0;
            unsigned num_instrs = 0;
      };
      bool
   handle_lds_direct_valu_hazard_instr(LdsDirectVALUHazardGlobalState& global_state,
               {
      if (instr->isVALU()) {
               bool uses_vgpr = false;
   for (Definition& def : instr->definitions)
         for (Operand& op : instr->operands) {
      uses_vgpr |=
      }
   if (uses_vgpr) {
      /* Transcendentals execute in parallel to other VALU and va_vdst count becomes unusable */
   global_state.wait_vdst =
                                 if (parse_vdst_wait(instr) == 0)
            block_state.num_instrs++;
   if (block_state.num_instrs > 256 || block_state.num_blocks > 32) {
      /* Exit to limit compile times and set wait_vdst to be safe. */
   global_state.wait_vdst =
                        }
      bool
   handle_lds_direct_valu_hazard_block(LdsDirectVALUHazardGlobalState& global_state,
         {
      if (block->kind & block_kind_loop_header) {
      if (global_state.loop_headers_visited.count(block->index))
                                 }
      unsigned
   handle_lds_direct_valu_hazard(State& state, aco_ptr<Instruction>& instr)
   {
      /* LdsDirectVALUHazard
   * Handle LDSDIR writing a VGPR after it's used by a VALU instruction.
   */
   if (instr->ldsdir().wait_vdst == 0)
            LdsDirectVALUHazardGlobalState global_state;
   global_state.wait_vdst = instr->ldsdir().wait_vdst;
   global_state.vgpr = instr->definitions[0].physReg();
   LdsDirectVALUHazardBlockState block_state;
   search_backwards<LdsDirectVALUHazardGlobalState, LdsDirectVALUHazardBlockState,
                  }
      enum VALUPartialForwardingHazardState : uint8_t {
      nothing_written,
   written_after_exec_write,
      };
      struct VALUPartialForwardingHazardGlobalState {
      bool hazard_found = false;
      };
      struct VALUPartialForwardingHazardBlockState {
      /* initialized by number of VGPRs read by VALU, decrement when encountered to return early */
   uint8_t num_vgprs_read = 0;
   BITSET_DECLARE(vgprs_read, 256) = {0};
   enum VALUPartialForwardingHazardState state = nothing_written;
   unsigned num_valu_since_read = 0;
            unsigned num_instrs = 0;
      };
      bool
   handle_valu_partial_forwarding_hazard_instr(VALUPartialForwardingHazardGlobalState& global_state,
               {
      if (instr->isSALU() && !instr->definitions.empty()) {
      if (block_state.state == written_after_exec_write && instr->writes_exec())
      } else if (instr->isVALU()) {
      bool vgpr_write = false;
   for (Definition& def : instr->definitions) {
                     for (unsigned i = 0; i < def.size(); i++) {
      unsigned reg = def.physReg().reg() - 256 + i;
                  if (block_state.state == exec_written && block_state.num_valu_since_write < 3) {
                        BITSET_CLEAR(block_state.vgprs_read, reg);
   block_state.num_vgprs_read--;
                  if (vgpr_write) {
      /* If the state is nothing_written: the check below should ensure that this write is
   * close enough to the read.
   *
   * If the state is exec_written: the current choice of second write has failed. Reset and
   * try with the current write as the second one, if it's close enough to the read.
   *
   * If the state is written_after_exec_write: a further second write would be better, if
   * it's close enough to the read.
   */
   if (block_state.state == nothing_written || block_state.num_valu_since_read < 5) {
      block_state.state = written_after_exec_write;
      } else {
            } else {
                     } else if (parse_vdst_wait(instr) == 0) {
                  if (block_state.num_valu_since_read >= (block_state.state == nothing_written ? 5 : 8))
         if (block_state.num_vgprs_read == 0)
            block_state.num_instrs++;
   if (block_state.num_instrs > 256 || block_state.num_blocks > 32) {
      /* Exit to limit compile times and set hazard_found=true to be safe. */
   global_state.hazard_found = true;
                  }
      bool
   handle_valu_partial_forwarding_hazard_block(VALUPartialForwardingHazardGlobalState& global_state,
               {
      if (block->kind & block_kind_loop_header) {
      if (global_state.loop_headers_visited.count(block->index))
                                 }
      bool
   handle_valu_partial_forwarding_hazard(State& state, aco_ptr<Instruction>& instr)
   {
      /* VALUPartialForwardingHazard
   * VALU instruction reads two VGPRs: one written before an exec write by SALU and one after.
   * For the hazard, there must be less than 3 VALU between the first and second VGPR writes.
   * There also must be less than 5 VALU between the second VGPR write and the current instruction.
   */
   if (state.program->wave_size != 64 || !instr->isVALU())
            unsigned num_vgprs = 0;
   for (Operand& op : instr->operands)
         if (num_vgprs <= 1)
                     for (unsigned i = 0; i < instr->operands.size(); i++) {
      Operand& op = instr->operands[i];
   if (op.physReg().reg() < 256)
         for (unsigned j = 0; j < op.size(); j++)
      }
            if (block_state.num_vgprs_read <= 1)
            VALUPartialForwardingHazardGlobalState global_state;
   search_backwards<VALUPartialForwardingHazardGlobalState, VALUPartialForwardingHazardBlockState,
                  }
      void
   handle_instruction_gfx11(State& state, NOP_ctx_gfx11& ctx, aco_ptr<Instruction>& instr,
         {
               /* VcmpxPermlaneHazard
   * Handle any permlane following a VOPC instruction writing exec, insert v_mov between them.
   */
   if (instr->isVOPC() && instr->definitions[0].physReg() == exec) {
         } else if (ctx.has_Vcmpx && (instr->opcode == aco_opcode::v_permlane16_b32 ||
                     /* v_nop would be discarded by SQ, so use v_mov with the first operand of the permlane */
   bld.vop1(aco_opcode::v_mov_b32, Definition(instr->operands[0].physReg(), v1),
      } else if (instr->isVALU() && instr->opcode != aco_opcode::v_nop) {
                  unsigned va_vdst = parse_vdst_wait(instr);
   unsigned vm_vsrc = 7;
            if (debug_flags & DEBUG_FORCE_WAITDEPS) {
      bld.sopp(aco_opcode::s_waitcnt_depctr, -1, 0x0000);
   va_vdst = 0;
   vm_vsrc = 0;
      } else if (instr->opcode == aco_opcode::s_waitcnt_depctr) {
      /* va_vdst already obtained through parse_vdst_wait(). */
   vm_vsrc = (instr->sopp().imm >> 2) & 0x7;
               if (instr->isLDSDIR()) {
      unsigned count = handle_lds_direct_valu_hazard(state, instr);
   LDSDIR_instruction* ldsdir = &instr->ldsdir();
   if (count < va_vdst) {
      ldsdir->wait_vdst = MIN2(ldsdir->wait_vdst, count);
                  /* VALUTransUseHazard
   * VALU reads VGPR written by transcendental instruction without 6+ VALU or 2+ transcendental
   * in-between.
   */
   if (va_vdst > 0 && instr->isVALU()) {
      uint8_t num_valu = 15;
   uint8_t num_trans = 15;
   for (Operand& op : instr->operands) {
      if (op.physReg().reg() < 256)
         for (unsigned i = 0; i < op.size(); i++) {
      num_valu = std::min(num_valu, ctx.valu_since_wr_by_trans.get(op.physReg(), i));
         }
   if (num_trans <= 1 && num_valu <= 5) {
      bld.sopp(aco_opcode::s_waitcnt_depctr, -1, 0x0fff);
                  if (va_vdst > 0 && handle_valu_partial_forwarding_hazard(state, instr)) {
      bld.sopp(aco_opcode::s_waitcnt_depctr, -1, 0x0fff);
               /* VALUMaskWriteHazard
   * VALU reads SGPR as a lane mask and later written by SALU cannot safely be read by SALU.
   */
   if (state.program->wave_size == 64 && instr->isSALU() &&
      check_written_regs(instr, ctx.sgpr_read_by_valu_as_lanemask)) {
   ctx.sgpr_read_by_valu_as_lanemask_then_wr_by_salu = ctx.sgpr_read_by_valu_as_lanemask;
      } else if (state.program->wave_size == 64 && instr->isSALU() &&
            bld.sopp(aco_opcode::s_waitcnt_depctr, -1, 0xfffe);
               if (va_vdst == 0) {
      ctx.valu_since_wr_by_trans.reset();
               if (sa_sdst == 0)
            if (instr->isVALU()) {
               ctx.valu_since_wr_by_trans.inc();
   if (is_trans)
            if (is_trans) {
      for (Definition& def : instr->definitions) {
      ctx.valu_since_wr_by_trans.set(def.physReg(), def.bytes());
                  if (state.program->wave_size == 64) {
      for (Operand& op : instr->operands) {
      if (op.isLiteral() || (!op.isConstant() && op.physReg().reg() < 128))
      }
   switch (instr->opcode) {
   case aco_opcode::v_addc_co_u32:
   case aco_opcode::v_subb_co_u32:
   case aco_opcode::v_subbrev_co_u32:
   case aco_opcode::v_cndmask_b16:
   case aco_opcode::v_cndmask_b32:
   case aco_opcode::v_div_fmas_f32:
   case aco_opcode::v_div_fmas_f64:
      if (instr->operands.back().physReg() != exec) {
      ctx.sgpr_read_by_valu_as_lanemask.set(instr->operands.back().physReg().reg());
      }
      default: break;
                  /* LdsDirectVMEMHazard
   * Handle LDSDIR writing a VGPR after it's used by a VMEM/DS instruction.
   */
   if (instr->isVMEM() || instr->isFlatLike()) {
      for (Definition& def : instr->definitions)
         if (instr->definitions.empty()) {
      for (Operand& op : instr->operands)
      } else {
      for (Operand& op : instr->operands)
         }
   if (instr->isDS() || instr->isFlat()) {
      for (Definition& def : instr->definitions)
         for (Operand& op : instr->operands)
      }
   if (instr->isVALU() || instr->isEXP() || vm_vsrc == 0) {
      ctx.vgpr_used_by_vmem_load.reset();
   ctx.vgpr_used_by_vmem_store.reset();
      } else if (instr->opcode == aco_opcode::s_waitcnt) {
      wait_imm imm(GFX11, instr->sopp().imm);
   if (imm.vm == 0)
         if (imm.lgkm == 0)
      } else if (instr->opcode == aco_opcode::s_waitcnt_vscnt && instr->sopk().imm == 0) {
         }
   if (instr->isLDSDIR()) {
      if (ctx.vgpr_used_by_vmem_load[instr->definitions[0].physReg().reg() - 256] ||
      ctx.vgpr_used_by_vmem_store[instr->definitions[0].physReg().reg() - 256] ||
   ctx.vgpr_used_by_ds[instr->definitions[0].physReg().reg() - 256]) {
   bld.sopp(aco_opcode::s_waitcnt_depctr, -1, 0xffe3);
   ctx.vgpr_used_by_vmem_load.reset();
   ctx.vgpr_used_by_vmem_store.reset();
            }
      bool
   has_vdst0_since_valu_instr(bool& global_state, unsigned& block_state, aco_ptr<Instruction>& pred)
   {
      if (parse_vdst_wait(pred) == 0)
            if (--block_state == 0) {
      global_state = false;
               if (pred->isVALU()) {
      bool vgpr_rd_or_wr = false;
   for (Definition def : pred->definitions) {
      if (def.physReg().reg() >= 256)
      }
   for (Operand op : pred->operands) {
      if (op.physReg().reg() >= 256)
      }
   if (vgpr_rd_or_wr) {
      global_state = false;
                     }
      void
   resolve_all_gfx11(State& state, NOP_ctx_gfx11& ctx,
         {
                        /* LdsDirectVALUHazard/VALUPartialForwardingHazard/VALUTransUseHazard */
   bool has_vdst0_since_valu = true;
   unsigned depth = 16;
   search_backwards<bool, unsigned, nullptr, has_vdst0_since_valu_instr>(
         if (!has_vdst0_since_valu) {
      waitcnt_depctr &= 0x0fff;
   ctx.valu_since_wr_by_trans.reset();
               /* VcmpxPermlaneHazard */
   if (ctx.has_Vcmpx) {
      ctx.has_Vcmpx = false;
               /* VALUMaskWriteHazard */
   if (state.program->wave_size == 64 &&
      (ctx.sgpr_read_by_valu_as_lanemask.any() ||
   ctx.sgpr_read_by_valu_as_lanemask_then_wr_by_salu.any())) {
   waitcnt_depctr &= 0xfffe;
   ctx.sgpr_read_by_valu_as_lanemask.reset();
               /* LdsDirectVMEMHazard */
   if (ctx.vgpr_used_by_vmem_load.any() || ctx.vgpr_used_by_vmem_store.any() ||
      ctx.vgpr_used_by_ds.any()) {
   waitcnt_depctr &= 0xffe3;
   ctx.vgpr_used_by_vmem_load.reset();
   ctx.vgpr_used_by_vmem_store.reset();
               if (waitcnt_depctr != 0xffff)
      }
      template <typename Ctx>
   using HandleInstr = void (*)(State& state, Ctx&, aco_ptr<Instruction>&,
            template <typename Ctx>
   using ResolveAll = void (*)(State& state, Ctx&, std::vector<aco_ptr<Instruction>>&);
      template <typename Ctx, HandleInstr<Ctx> Handle, ResolveAll<Ctx> Resolve>
   void
   handle_block(Program* program, Ctx& ctx, Block& block)
   {
      if (block.instructions.empty())
            State state;
   state.program = program;
   state.block = &block;
            block.instructions.clear(); // Silence clang-analyzer-cplusplus.Move warning
            bool found_end = false;
   for (aco_ptr<Instruction>& instr : state.old_instructions) {
               /* Resolve all possible hazards (we don't know what s_setpc_b64 jumps to). */
   if (instr->opcode == aco_opcode::s_setpc_b64) {
               std::vector<aco_ptr<Instruction>> resolve_instrs;
   Resolve(state, ctx, resolve_instrs);
   block.instructions.insert(std::prev(block.instructions.end()),
                  found_end = true;
               found_end |= instr->opcode == aco_opcode::s_endpgm;
               /* Resolve all possible hazards (we don't know what the shader is concatenated with). */
   if (block.linear_succs.empty() && !found_end)
      }
      template <typename Ctx, HandleInstr<Ctx> Handle, ResolveAll<Ctx> Resolve>
   void
   mitigate_hazards(Program* program)
   {
      std::vector<Ctx> all_ctx(program->blocks.size());
            for (unsigned i = 0; i < program->blocks.size(); i++) {
      Block& block = program->blocks[i];
            if (block.kind & block_kind_loop_header) {
         } else if (block.kind & block_kind_loop_exit) {
      /* Go through the whole loop again */
   for (unsigned idx = loop_header_indices.top(); idx < i; idx++) {
      Ctx loop_block_ctx;
                           /* We only need to continue if the loop header context changed */
                                          for (unsigned b : block.linear_preds)
                  }
      } /* end namespace */
      void
   insert_NOPs(Program* program)
   {
      if (program->gfx_level >= GFX11)
         else if (program->gfx_level >= GFX10_3)
         else if (program->gfx_level >= GFX10)
         else
      }
      } // namespace aco
