   /*
   * Copyright © 2020 Valve Corporation
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
      #include "util/crc32.h"
      #include <algorithm>
   #include <deque>
   #include <set>
   #include <vector>
      namespace aco {
      /* sgpr_presched/vgpr_presched */
   void
   collect_presched_stats(Program* program)
   {
      RegisterDemand presched_demand;
   for (Block& block : program->blocks)
         program->statistics[aco_statistic_sgpr_presched] = presched_demand.sgpr;
      }
      class BlockCycleEstimator {
   public:
      enum resource {
      null = 0,
   scalar,
   branch_sendmsg,
   valu,
   valu_complex,
   lds,
   export_gds,
   vmem,
                                 int32_t cur_cycle = 0;
   int32_t res_available[(int)BlockCycleEstimator::resource_count] = {0};
   unsigned res_usage[(int)BlockCycleEstimator::resource_count] = {0};
   int32_t reg_available[512] = {0};
   std::deque<int32_t> lgkm;
   std::deque<int32_t> exp;
   std::deque<int32_t> vm;
            unsigned predict_cost(aco_ptr<Instruction>& instr);
   void add(aco_ptr<Instruction>& instr);
         private:
      unsigned get_waitcnt_cost(wait_imm imm);
            void use_resources(aco_ptr<Instruction>& instr);
      };
      struct wait_counter_info {
      wait_counter_info(unsigned vm_, unsigned exp_, unsigned lgkm_, unsigned vs_)
                  unsigned vm;
   unsigned exp;
   unsigned lgkm;
      };
      struct perf_info {
               BlockCycleEstimator::resource rsrc0;
            BlockCycleEstimator::resource rsrc1;
      };
      static bool
   is_dual_issue_capable(const Program& program, const Instruction& instr)
   {
      if (program.gfx_level < GFX11 || !instr.isVALU() || instr.isDPP())
            switch (instr.opcode) {
   case aco_opcode::v_fma_f32:
   case aco_opcode::v_fmac_f32:
   case aco_opcode::v_fmaak_f32:
   case aco_opcode::v_fmamk_f32:
   case aco_opcode::v_mul_f32:
   case aco_opcode::v_add_f32:
   case aco_opcode::v_sub_f32:
   case aco_opcode::v_subrev_f32:
   case aco_opcode::v_mul_legacy_f32:
   case aco_opcode::v_fma_legacy_f32:
   case aco_opcode::v_fmac_legacy_f32:
   case aco_opcode::v_fma_f16:
   case aco_opcode::v_fmac_f16:
   case aco_opcode::v_fmaak_f16:
   case aco_opcode::v_fmamk_f16:
   case aco_opcode::v_mul_f16:
   case aco_opcode::v_add_f16:
   case aco_opcode::v_sub_f16:
   case aco_opcode::v_subrev_f16:
   case aco_opcode::v_mov_b32:
   case aco_opcode::v_movreld_b32:
   case aco_opcode::v_movrels_b32:
   case aco_opcode::v_movrelsd_b32:
   case aco_opcode::v_movrelsd_2_b32:
   case aco_opcode::v_cndmask_b32:
   case aco_opcode::v_writelane_b32_e64:
   case aco_opcode::v_mov_b16:
   case aco_opcode::v_cndmask_b16:
   case aco_opcode::v_max_f32:
   case aco_opcode::v_min_f32:
   case aco_opcode::v_max_f16:
   case aco_opcode::v_min_f16:
   case aco_opcode::v_max_i16_e64:
   case aco_opcode::v_min_i16_e64:
   case aco_opcode::v_max_u16_e64:
   case aco_opcode::v_min_u16_e64:
   case aco_opcode::v_add_i16:
   case aco_opcode::v_sub_i16:
   case aco_opcode::v_mad_i16:
   case aco_opcode::v_add_u16_e64:
   case aco_opcode::v_sub_u16_e64:
   case aco_opcode::v_mad_u16:
   case aco_opcode::v_mul_lo_u16_e64:
   case aco_opcode::v_not_b16:
   case aco_opcode::v_and_b16:
   case aco_opcode::v_or_b16:
   case aco_opcode::v_xor_b16:
   case aco_opcode::v_lshrrev_b16_e64:
   case aco_opcode::v_ashrrev_i16_e64:
   case aco_opcode::v_lshlrev_b16_e64:
   case aco_opcode::v_dot2_bf16_bf16:
   case aco_opcode::v_dot2_f32_bf16:
   case aco_opcode::v_dot2_f16_f16:
   case aco_opcode::v_dot2_f32_f16:
   case aco_opcode::v_dot2c_f32_f16: return true;
   case aco_opcode::v_fma_mix_f32:
   case aco_opcode::v_fma_mixlo_f16:
   case aco_opcode::v_fma_mixhi_f16: {
      /* dst and acc type must match */
   if (instr.valu().opsel_hi[2] == (instr.opcode == aco_opcode::v_fma_mix_f32))
            /* If all operands are vgprs, two must be the same. */
   for (unsigned i = 0; i < 3; i++) {
      if (instr.operands[i].isConstant() || instr.operands[i].isOfType(RegType::sgpr))
         for (unsigned j = 0; j < i; j++) {
      if (instr.operands[i].physReg() == instr.operands[j].physReg())
         }
      }
   default: return false;
      }
      static perf_info
   get_perf_info(const Program& program, const Instruction& instr)
   {
            #define WAIT(res)          BlockCycleEstimator::res, 0
   #define WAIT_USE(res, cnt) BlockCycleEstimator::res, cnt
         if (program.gfx_level >= GFX10) {
      /* fp64 might be incorrect */
   switch (cls) {
   case instr_class::valu32:
   case instr_class::valu_convert32:
   case instr_class::valu_fma: return {5, WAIT_USE(valu, 1)};
   case instr_class::valu64: return {6, WAIT_USE(valu, 2), WAIT_USE(valu_complex, 2)};
   case instr_class::valu_quarter_rate32:
         case instr_class::valu_transcendental32:
         case instr_class::valu_double: return {22, WAIT_USE(valu, 16), WAIT_USE(valu_complex, 16)};
   case instr_class::valu_double_add:
         case instr_class::valu_double_convert:
         case instr_class::valu_double_transcendental:
         case instr_class::salu: return {2, WAIT_USE(scalar, 1)};
   case instr_class::smem: return {0, WAIT_USE(scalar, 1)};
   case instr_class::branch:
   case instr_class::sendmsg: return {0, WAIT_USE(branch_sendmsg, 1)};
   case instr_class::ds:
      return instr.isDS() && instr.ds().gds ? perf_info{0, WAIT_USE(export_gds, 1)}
      case instr_class::exp: return {0, WAIT_USE(export_gds, 1)};
   case instr_class::vmem: return {0, WAIT_USE(vmem, 1)};
   case instr_class::wmma: {
      /* int8 and (b)f16 have the same performance. */
   uint8_t cost = instr.opcode == aco_opcode::v_wmma_i32_16x16x16_iu4 ? 16 : 32;
      }
   case instr_class::barrier:
   case instr_class::waitcnt:
   case instr_class::other:
   default: return {0};
      } else {
      switch (cls) {
   case instr_class::valu32: return {4, WAIT_USE(valu, 4)};
   case instr_class::valu_convert32: return {16, WAIT_USE(valu, 16)};
   case instr_class::valu64: return {8, WAIT_USE(valu, 8)};
   case instr_class::valu_quarter_rate32: return {16, WAIT_USE(valu, 16)};
   case instr_class::valu_fma:
      return program.dev.has_fast_fma32 ? perf_info{4, WAIT_USE(valu, 4)}
      case instr_class::valu_transcendental32: return {16, WAIT_USE(valu, 16)};
   case instr_class::valu_double: return {64, WAIT_USE(valu, 64)};
   case instr_class::valu_double_add: return {32, WAIT_USE(valu, 32)};
   case instr_class::valu_double_convert: return {16, WAIT_USE(valu, 16)};
   case instr_class::valu_double_transcendental: return {64, WAIT_USE(valu, 64)};
   case instr_class::salu: return {4, WAIT_USE(scalar, 4)};
   case instr_class::smem: return {4, WAIT_USE(scalar, 4)};
   case instr_class::branch:
      return {8, WAIT_USE(branch_sendmsg, 8)};
      case instr_class::ds:
      return instr.isDS() && instr.ds().gds ? perf_info{4, WAIT_USE(export_gds, 4)}
      case instr_class::exp: return {16, WAIT_USE(export_gds, 16)};
   case instr_class::vmem: return {4, WAIT_USE(vmem, 4)};
   case instr_class::barrier:
   case instr_class::waitcnt:
   case instr_class::other:
   default: return {4};
            #undef WAIT_USE
   #undef WAIT
   }
      void
   BlockCycleEstimator::use_resources(aco_ptr<Instruction>& instr)
   {
               if (perf.rsrc0 != resource_count) {
      res_available[(int)perf.rsrc0] = cur_cycle + perf.cost0;
               if (perf.rsrc1 != resource_count) {
      res_available[(int)perf.rsrc1] = cur_cycle + perf.cost1;
         }
      int32_t
   BlockCycleEstimator::cycles_until_res_available(aco_ptr<Instruction>& instr)
   {
               int32_t cost = 0;
   if (perf.rsrc0 != resource_count)
         if (perf.rsrc1 != resource_count)
               }
      static wait_counter_info
   get_wait_counter_info(aco_ptr<Instruction>& instr)
   {
      /* These numbers are all a bit nonsense. LDS/VMEM/SMEM/EXP performance
            if (instr->isEXP())
            if (instr->isFlatLike()) {
      unsigned lgkm = instr->isFlat() ? 20 : 0;
   if (!instr->definitions.empty())
         else
               if (instr->isSMEM()) {
      if (instr->definitions.empty())
         if (instr->operands.empty()) /* s_memtime and s_memrealtime */
            bool likely_desc_load = instr->operands[0].size() == 2;
   bool soe = instr->operands.size() >= (!instr->definitions.empty() ? 3 : 4);
   bool const_offset =
            if (likely_desc_load || const_offset)
                        if (instr->format == Format::DS)
            if (instr->isVMEM() && !instr->definitions.empty())
            if (instr->isVMEM() && instr->definitions.empty())
               }
      static wait_imm
   get_wait_imm(Program* program, aco_ptr<Instruction>& instr)
   {
      if (instr->opcode == aco_opcode::s_endpgm) {
         } else if (instr->opcode == aco_opcode::s_waitcnt) {
         } else if (instr->opcode == aco_opcode::s_waitcnt_vscnt) {
         } else {
      unsigned max_lgkm_cnt = program->gfx_level >= GFX10 ? 62 : 14;
   unsigned max_exp_cnt = 6;
   unsigned max_vm_cnt = program->gfx_level >= GFX9 ? 62 : 14;
            wait_counter_info wait_info = get_wait_counter_info(instr);
   wait_imm imm;
   imm.lgkm = wait_info.lgkm ? max_lgkm_cnt : wait_imm::unset_counter;
   imm.exp = wait_info.exp ? max_exp_cnt : wait_imm::unset_counter;
   imm.vm = wait_info.vm ? max_vm_cnt : wait_imm::unset_counter;
   imm.vs = wait_info.vs ? max_vs_cnt : wait_imm::unset_counter;
         }
      unsigned
   BlockCycleEstimator::get_dependency_cost(aco_ptr<Instruction>& instr)
   {
               wait_imm imm = get_wait_imm(program, instr);
   if (imm.vm != wait_imm::unset_counter) {
      for (int i = 0; i < (int)vm.size() - imm.vm; i++)
      }
   if (imm.exp != wait_imm::unset_counter) {
      for (int i = 0; i < (int)exp.size() - imm.exp; i++)
      }
   if (imm.lgkm != wait_imm::unset_counter) {
      for (int i = 0; i < (int)lgkm.size() - imm.lgkm; i++)
      }
   if (imm.vs != wait_imm::unset_counter) {
      for (int i = 0; i < (int)vs.size() - imm.vs; i++)
               if (instr->opcode == aco_opcode::s_endpgm) {
      for (unsigned i = 0; i < 512; i++)
      } else if (program->gfx_level >= GFX10) {
      for (Operand& op : instr->operands) {
      if (op.isConstant() || op.isUndefined())
         for (unsigned i = 0; i < op.size(); i++)
                  if (program->gfx_level < GFX10)
               }
      unsigned
   BlockCycleEstimator::predict_cost(aco_ptr<Instruction>& instr)
   {
      int32_t dep = get_dependency_cost(instr);
      }
      static bool
   is_vector(aco_opcode op)
   {
      switch (instr_info.classes[(int)op]) {
   case instr_class::valu32:
   case instr_class::valu_convert32:
   case instr_class::valu_fma:
   case instr_class::valu_double:
   case instr_class::valu_double_add:
   case instr_class::valu_double_convert:
   case instr_class::valu_double_transcendental:
   case instr_class::vmem:
   case instr_class::ds:
   case instr_class::exp:
   case instr_class::valu64:
   case instr_class::valu_quarter_rate32:
   case instr_class::valu_transcendental32: return true;
   default: return false;
      }
      void
   BlockCycleEstimator::add(aco_ptr<Instruction>& instr)
   {
                        unsigned start;
   bool dual_issue = program->gfx_level >= GFX10 && program->wave_size == 64 &&
               for (unsigned i = 0; i < (dual_issue ? 2 : 1); i++) {
               start = cur_cycle;
            /* GCN is in-order and doesn't begin the next instruction until the current one finishes */
               wait_imm imm = get_wait_imm(program, instr);
   while (lgkm.size() > imm.lgkm)
         while (exp.size() > imm.exp)
         while (vm.size() > imm.vm)
         while (vs.size() > imm.vs)
            wait_counter_info wait_info = get_wait_counter_info(instr);
   if (wait_info.exp)
         if (wait_info.lgkm)
         if (wait_info.vm)
         if (wait_info.vs)
            /* This is inaccurate but shouldn't affect anything after waitcnt insertion.
   * Before waitcnt insertion, this is necessary to consider memory operations.
   */
   int latency = MAX3(wait_info.exp, wait_info.lgkm, wait_info.vm);
            for (Definition& def : instr->definitions) {
      int32_t* available = &reg_available[def.physReg().reg()];
   for (unsigned i = 0; i < def.size(); i++)
         }
      static void
   join_queue(std::deque<int32_t>& queue, const std::deque<int32_t>& pred, int cycle_diff)
   {
      for (unsigned i = 0; i < MIN2(queue.size(), pred.size()); i++)
         for (int i = pred.size() - queue.size() - 1; i >= 0; i--)
      }
      void
   BlockCycleEstimator::join(const BlockCycleEstimator& pred)
   {
               for (unsigned i = 0; i < (unsigned)resource_count; i++) {
      assert(res_usage[i] == 0);
               for (unsigned i = 0; i < 512; i++)
            join_queue(lgkm, pred.lgkm, -pred.cur_cycle);
   join_queue(exp, pred.exp, -pred.cur_cycle);
   join_queue(vm, pred.vm, -pred.cur_cycle);
      }
      /* instructions/branches/vmem_clauses/smem_clauses/cycles */
   void
   collect_preasm_stats(Program* program)
   {
      for (Block& block : program->blocks) {
      std::set<Instruction*> vmem_clause;
                     for (aco_ptr<Instruction>& instr : block.instructions) {
                                    if ((instr->isVMEM() || instr->isScratch() || instr->isGlobal()) &&
      !instr->operands.empty()) {
   if (std::none_of(vmem_clause.begin(), vmem_clause.end(),
                        } else {
                  if (instr->isSMEM() && !instr->operands.empty()) {
      if (std::none_of(smem_clause.begin(), smem_clause.end(),
                        } else {
                        double latency = 0;
   double usage[(int)BlockCycleEstimator::resource_count] = {0};
            constexpr const unsigned vmem_latency = 320;
   for (const Definition def : program->args_pending_vmem) {
      blocks[0].vm.push_back(vmem_latency);
   for (unsigned i = 0; i < def.size(); i++)
               for (Block& block : program->blocks) {
      BlockCycleEstimator& block_est = blocks[block.index];
   for (unsigned pred : block.linear_preds)
            for (aco_ptr<Instruction>& instr : block.instructions) {
      unsigned before = block_est.cur_cycle;
   block_est.add(instr);
               /* TODO: it would be nice to be able to consider estimated loop trip
   * counts used for loop unrolling.
            /* TODO: estimate the trip_count of divergent loops (those which break
   * divergent) higher than of uniform loops
            /* Assume loops execute 8-2 times, uniform branches are taken 50% the time,
   * and any lane in the wave takes a side of a divergent branch 75% of the
   * time.
   */
   double iter = 1.0;
   iter *= block.loop_nest_depth > 0 ? 8.0 : 1.0;
   iter *= block.loop_nest_depth > 1 ? 4.0 : 1.0;
   iter *= block.loop_nest_depth > 2 ? pow(2.0, block.loop_nest_depth - 2) : 1.0;
   iter *= pow(0.5, block.uniform_if_depth);
            bool divergent_if_linear_else =
      block.logical_preds.empty() && block.linear_preds.size() == 1 &&
   block.linear_succs.size() == 1 &&
      if (divergent_if_linear_else)
            latency += block_est.cur_cycle * iter;
   for (unsigned i = 0; i < (unsigned)BlockCycleEstimator::resource_count; i++)
               /* This likely exaggerates the effectiveness of parallelism because it
   * ignores instruction ordering. It can assume there might be SALU/VALU/etc
   * work to from other waves while one is idle but that might not be the case
   * because those other waves have not reached such a point yet.
            double parallelism = program->num_waves;
   for (unsigned i = 0; i < (unsigned)BlockCycleEstimator::resource_count; i++) {
      if (usage[i] > 0.0)
      }
   double waves_per_cycle = 1.0 / latency * parallelism;
            double max_utilization = 1.0;
   if (program->workgroup_size != UINT_MAX)
      max_utilization =
               program->statistics[aco_statistic_latency] = round(latency);
            if (debug_flags & DEBUG_PERF_INFO) {
               fprintf(stderr, "num_waves: %u\n", program->num_waves);
   fprintf(stderr, "salu_smem_usage: %f\n", usage[(int)BlockCycleEstimator::scalar]);
   fprintf(stderr, "branch_sendmsg_usage: %f\n",
         fprintf(stderr, "valu_usage: %f\n", usage[(int)BlockCycleEstimator::valu]);
   fprintf(stderr, "valu_complex_usage: %f\n", usage[(int)BlockCycleEstimator::valu_complex]);
   fprintf(stderr, "lds_usage: %f\n", usage[(int)BlockCycleEstimator::lds]);
   fprintf(stderr, "export_gds_usage: %f\n", usage[(int)BlockCycleEstimator::export_gds]);
   fprintf(stderr, "vmem_usage: %f\n", usage[(int)BlockCycleEstimator::vmem]);
   fprintf(stderr, "latency: %f\n", latency);
   fprintf(stderr, "parallelism: %f\n", parallelism);
   fprintf(stderr, "max_utilization: %f\n", max_utilization);
   fprintf(stderr, "wave64_per_cycle: %f\n", wave64_per_cycle);
         }
      void
   collect_postasm_stats(Program* program, const std::vector<uint32_t>& code)
   {
         }
      Instruction_cycle_info
   get_cycle_info(const Program& program, const Instruction& instr)
   {
      perf_info info = get_perf_info(program, instr);
      }
      } // namespace aco
