   /*
   * Copyright © 2018 Valve Corporation
   * Copyright © 2018 Google
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
   #include "aco_util.h"
      #include "common/sid.h"
      #include <algorithm>
   #include <cstring>
   #include <map>
   #include <set>
   #include <stack>
   #include <unordered_map>
   #include <unordered_set>
   #include <vector>
      namespace std {
   template <> struct hash<aco::Temp> {
      size_t operator()(aco::Temp temp) const noexcept
   {
      uint32_t v;
   std::memcpy(&v, &temp, sizeof(temp));
         };
   } // namespace std
      /*
   * Implements the spilling algorithm on SSA-form from
   * "Register Spilling and Live-Range Splitting for SSA-Form Programs"
   * by Matthias Braun and Sebastian Hack.
   */
      namespace aco {
      namespace {
      struct remat_info {
         };
      struct spill_ctx {
      RegisterDemand target_pressure;
   Program* program;
            std::vector<std::vector<RegisterDemand>> register_demand;
   std::vector<aco::map<Temp, Temp>> renames;
   std::vector<aco::unordered_map<Temp, uint32_t>> spills_entry;
            std::vector<bool> processed;
            using next_use_distance_startend_type = aco::unordered_map<Temp, std::pair<uint32_t, uint32_t>>;
   std::vector<next_use_distance_startend_type> next_use_distances_start;
   std::vector<next_use_distance_startend_type> next_use_distances_end;
   std::vector<std::vector<std::pair<Temp, uint32_t>>> local_next_use_distance; /* Working buffer */
   std::vector<std::pair<RegClass, std::unordered_set<uint32_t>>> interferences;
   std::vector<std::vector<uint32_t>> affinities;
   std::vector<bool> is_reloaded;
   aco::unordered_map<Temp, remat_info> remat;
   std::set<Instruction*> unused_remats;
            unsigned sgpr_spill_slots;
   unsigned vgpr_spill_slots;
            spill_ctx(const RegisterDemand target_pressure_, Program* program_,
            : target_pressure(target_pressure_), program(program_), memory(),
      register_demand(std::move(register_demand_)),
   renames(program->blocks.size(), aco::map<Temp, Temp>(memory)),
   spills_entry(program->blocks.size(), aco::unordered_map<Temp, uint32_t>(memory)),
   spills_exit(program->blocks.size(), aco::unordered_map<Temp, uint32_t>(memory)),
   processed(program->blocks.size(), false),
   next_use_distances_start(program->blocks.size(), next_use_distance_startend_type(memory)),
   next_use_distances_end(program->blocks.size(), next_use_distance_startend_type(memory)),
            void add_affinity(uint32_t first, uint32_t second)
   {
      unsigned found_first = affinities.size();
   unsigned found_second = affinities.size();
   for (unsigned i = 0; i < affinities.size(); i++) {
      std::vector<uint32_t>& vec = affinities[i];
   for (uint32_t entry : vec) {
      if (entry == first)
         else if (entry == second)
         }
   if (found_first == affinities.size() && found_second == affinities.size()) {
         } else if (found_first < affinities.size() && found_second == affinities.size()) {
         } else if (found_second < affinities.size() && found_first == affinities.size()) {
         } else if (found_first != found_second) {
      /* merge second into first */
   affinities[found_first].insert(affinities[found_first].end(),
                  } else {
                     void add_interference(uint32_t first, uint32_t second)
   {
      if (interferences[first].first.type() != interferences[second].first.type())
            bool inserted = interferences[first].second.insert(second).second;
   if (inserted)
               uint32_t allocate_spill_id(RegClass rc)
   {
      interferences.emplace_back(rc, std::unordered_set<uint32_t>());
   is_reloaded.push_back(false);
                  };
      int32_t
   get_dominator(int idx_a, int idx_b, Program* program, bool is_linear)
   {
         if (idx_a == -1)
         if (idx_b == -1)
         if (is_linear) {
      while (idx_a != idx_b) {
      if (idx_a > idx_b)
         else
         } else {
      while (idx_a != idx_b) {
      if (idx_a > idx_b)
         else
         }
   assert(idx_a != -1);
      }
      void
   next_uses_per_block(spill_ctx& ctx, unsigned block_idx, uint32_t& worklist)
   {
      Block* block = &ctx.program->blocks[block_idx];
   ctx.next_use_distances_start[block_idx] = ctx.next_use_distances_end[block_idx];
            /* to compute the next use distance at the beginning of the block, we have to add the block's
   * size */
   for (std::unordered_map<Temp, std::pair<uint32_t, uint32_t>>::iterator it =
            it != next_use_distances_start.end(); ++it)
         int idx = block->instructions.size() - 1;
   while (idx >= 0) {
               if (instr->opcode == aco_opcode::p_linear_phi || instr->opcode == aco_opcode::p_phi)
            for (const Definition& def : instr->definitions) {
      if (def.isTemp())
               for (const Operand& op : instr->operands) {
      /* omit exec mask */
   if (op.isFixed() && op.physReg() == exec)
         if (op.regClass().type() == RegType::vgpr && op.regClass().is_linear())
         if (op.isTemp())
      }
               assert(block_idx != 0 || next_use_distances_start.empty());
   std::unordered_set<Temp> phi_defs;
   while (idx >= 0) {
      aco_ptr<Instruction>& instr = block->instructions[idx];
                     auto it = instr->definitions[0].isTemp()
               if (it != next_use_distances_start.end() &&
      phi_defs.insert(instr->definitions[0].getTemp()).second) {
               for (unsigned i = 0; i < instr->operands.size(); i++) {
      unsigned pred_idx =
         if (instr->operands[i].isTemp()) {
      auto insert_result = ctx.next_use_distances_end[pred_idx].insert(
         const bool inserted = insert_result.second;
   std::pair<uint32_t, uint32_t>& entry_distance = insert_result.first->second;
   if (inserted || entry_distance != distance)
               }
               /* all remaining live vars must be live-out at the predecessors */
   for (std::pair<const Temp, std::pair<uint32_t, uint32_t>>& pair : next_use_distances_start) {
      Temp temp = pair.first;
   if (phi_defs.count(temp)) {
         }
   uint32_t distance = pair.second.second;
   uint32_t dom = pair.second.first;
   std::vector<unsigned>& preds = temp.is_linear() ? block->linear_preds : block->logical_preds;
   for (unsigned pred_idx : preds) {
      if (ctx.program->blocks[pred_idx].loop_nest_depth > block->loop_nest_depth)
         auto insert_result = ctx.next_use_distances_end[pred_idx].insert(
                        if (!inserted) {
      dom = get_dominator(dom, entry_distance.first, ctx.program, temp.is_linear());
      }
   if (entry_distance != std::pair<uint32_t, uint32_t>{dom, distance}) {
      worklist = std::max(worklist, pred_idx + 1);
               }
      void
   compute_global_next_uses(spill_ctx& ctx)
   {
      uint32_t worklist = ctx.program->blocks.size();
   while (worklist) {
      unsigned block_idx = --worklist;
         }
      bool
   should_rematerialize(aco_ptr<Instruction>& instr)
   {
      /* TODO: rematerialization is only supported for VOP1, SOP1 and PSEUDO */
   if (instr->format != Format::VOP1 && instr->format != Format::SOP1 &&
      instr->format != Format::PSEUDO && instr->format != Format::SOPK)
      /* TODO: pseudo-instruction rematerialization is only supported for
   * p_create_vector/p_parallelcopy */
   if (instr->isPseudo() && instr->opcode != aco_opcode::p_create_vector &&
      instr->opcode != aco_opcode::p_parallelcopy)
      if (instr->isSOPK() && instr->opcode != aco_opcode::s_movk_i32)
            for (const Operand& op : instr->operands) {
      /* TODO: rematerialization using temporaries isn't yet supported */
   if (!op.isConstant())
               /* TODO: rematerialization with multiple definitions isn't yet supported */
   if (instr->definitions.size() > 1)
               }
      aco_ptr<Instruction>
   do_reload(spill_ctx& ctx, Temp tmp, Temp new_name, uint32_t spill_id)
   {
      std::unordered_map<Temp, remat_info>::iterator remat = ctx.remat.find(tmp);
   if (remat != ctx.remat.end()) {
      Instruction* instr = remat->second.instr;
   assert((instr->isVOP1() || instr->isSOP1() || instr->isPseudo() || instr->isSOPK()) &&
         assert((instr->format != Format::PSEUDO || instr->opcode == aco_opcode::p_create_vector ||
         instr->opcode == aco_opcode::p_parallelcopy) &&
            aco_ptr<Instruction> res;
   if (instr->isVOP1()) {
      res.reset(create_instruction<VALU_instruction>(
      } else if (instr->isSOP1()) {
      res.reset(create_instruction<SOP1_instruction>(
      } else if (instr->isPseudo()) {
      res.reset(create_instruction<Pseudo_instruction>(
      } else if (instr->isSOPK()) {
      res.reset(create_instruction<SOPK_instruction>(
            }
   for (unsigned i = 0; i < instr->operands.size(); i++) {
      res->operands[i] = instr->operands[i];
   if (instr->operands[i].isTemp()) {
      assert(false && "unsupported");
   if (ctx.remat.count(instr->operands[i].getTemp()))
         }
   res->definitions[0] = Definition(new_name);
      } else {
      aco_ptr<Pseudo_instruction> reload{
         reload->operands[0] = Operand::c32(spill_id);
   reload->definitions[0] = Definition(new_name);
   ctx.is_reloaded[spill_id] = true;
         }
      void
   get_rematerialize_info(spill_ctx& ctx)
   {
      for (Block& block : ctx.program->blocks) {
      bool logical = false;
   for (aco_ptr<Instruction>& instr : block.instructions) {
      if (instr->opcode == aco_opcode::p_logical_start)
         else if (instr->opcode == aco_opcode::p_logical_end)
         if (logical && should_rematerialize(instr)) {
      for (const Definition& def : instr->definitions) {
      if (def.isTemp()) {
      ctx.remat[def.getTemp()] = remat_info{instr.get()};
                     }
      void
   update_local_next_uses(spill_ctx& ctx, Block* block,
         {
      if (local_next_uses.size() < block->instructions.size()) {
      /* Allocate more next-use-maps. Note that by never reducing the vector size, we enable
   * future calls to this function to re-use already allocated map memory. */
               local_next_uses[block->instructions.size() - 1].clear();
   for (std::pair<const Temp, std::pair<uint32_t, uint32_t>>& pair :
      ctx.next_use_distances_end[block->index]) {
   local_next_uses[block->instructions.size() - 1].push_back(std::make_pair<Temp, uint32_t>(
               for (int idx = block->instructions.size() - 1; idx >= 0; idx--) {
      aco_ptr<Instruction>& instr = block->instructions[idx];
   if (!instr)
         if (instr->opcode == aco_opcode::p_phi || instr->opcode == aco_opcode::p_linear_phi)
            if (idx != (int)block->instructions.size() - 1) {
                  for (const Operand& op : instr->operands) {
      if (op.isFixed() && op.physReg() == exec)
         if (op.regClass().type() == RegType::vgpr && op.regClass().is_linear())
         if (op.isTemp()) {
      auto it = std::find_if(local_next_uses[idx].begin(), local_next_uses[idx].end(),
         if (it == local_next_uses[idx].end()) {
         } else {
               }
   for (const Definition& def : instr->definitions) {
      if (def.isTemp()) {
      auto it = std::find_if(local_next_uses[idx].begin(), local_next_uses[idx].end(),
         if (it != local_next_uses[idx].end()) {
                     }
      RegisterDemand
   get_demand_before(spill_ctx& ctx, unsigned block_idx, unsigned idx)
   {
      if (idx == 0) {
      RegisterDemand demand = ctx.register_demand[block_idx][idx];
   aco_ptr<Instruction>& instr = ctx.program->blocks[block_idx].instructions[idx];
   aco_ptr<Instruction> instr_before(nullptr);
      } else {
            }
      RegisterDemand
   get_live_in_demand(spill_ctx& ctx, unsigned block_idx)
   {
      unsigned idx = 0;
   RegisterDemand reg_pressure = RegisterDemand();
   Block& block = ctx.program->blocks[block_idx];
   for (aco_ptr<Instruction>& phi : block.instructions) {
      if (!is_phi(phi))
                  /* Killed phi definitions increase pressure in the predecessor but not
   * the block they're in. Since the loops below are both to control
   * pressure of the start of this block and the ends of it's
   * predecessors, we need to count killed unspilled phi definitions here. */
   if (phi->definitions[0].isTemp() && phi->definitions[0].isKill() &&
      !ctx.spills_entry[block_idx].count(phi->definitions[0].getTemp()))
                     /* Consider register pressure from linear predecessors. This can affect
   * reg_pressure if the branch instructions define sgprs. */
   for (unsigned pred : block.linear_preds)
      reg_pressure.sgpr =
            }
      RegisterDemand
   init_live_in_vars(spill_ctx& ctx, Block* block, unsigned block_idx)
   {
               /* first block, nothing was spilled before */
   if (block->linear_preds.empty())
            /* next use distances at the beginning of the current block */
            /* loop header block */
   if (block->loop_nest_depth > ctx.program->blocks[block_idx - 1].loop_nest_depth) {
      assert(block->linear_preds[0] == block_idx - 1);
            /* create new loop_info */
            /* check how many live-through variables should be spilled */
   RegisterDemand reg_pressure = get_live_in_demand(ctx, block_idx);
   RegisterDemand loop_demand = reg_pressure;
   unsigned i = block_idx;
   while (ctx.program->blocks[i].loop_nest_depth >= block->loop_nest_depth) {
      assert(ctx.program->blocks.size() > i);
      }
            for (auto spilled : ctx.spills_exit[block_idx - 1]) {
               /* variable is not live at loop entry: probably a phi operand */
                  /* keep constants and live-through variables spilled */
   if (it->second.first >= loop_end || ctx.remat.count(spilled.first)) {
      ctx.spills_entry[block_idx][spilled.first] = spilled.second;
   spilled_registers += spilled.first;
                  /* select live-through variables and constants */
   RegType type = RegType::vgpr;
   while (loop_demand.exceeds(ctx.target_pressure)) {
      /* if VGPR demand is low enough, select SGPRs */
   if (type == RegType::vgpr && loop_demand.vgpr <= ctx.target_pressure.vgpr)
         /* if SGPR demand is low enough, break */
                  unsigned distance = 0;
   Temp to_spill;
   for (const std::pair<const Temp, std::pair<uint32_t, uint32_t>>& pair :
      next_use_distances) {
   if (pair.first.type() == type &&
      (pair.second.first >= loop_end ||
   (ctx.remat.count(pair.first) && type == RegType::sgpr)) &&
   pair.second.second > distance && !ctx.spills_entry[block_idx].count(pair.first)) {
   to_spill = pair.first;
                  /* select SGPRs or break */
   if (distance == 0) {
      if (type == RegType::sgpr)
         type = RegType::sgpr;
               uint32_t spill_id;
   if (!ctx.spills_exit[block_idx - 1].count(to_spill)) {
         } else {
                  ctx.spills_entry[block_idx][to_spill] = spill_id;
   spilled_registers += to_spill;
               /* shortcut */
   if (!loop_demand.exceeds(ctx.target_pressure))
            /* if reg pressure is too high at beginning of loop, add variables with furthest use */
            while (reg_pressure.exceeds(ctx.target_pressure)) {
      unsigned distance = 0;
                  for (const std::pair<const Temp, std::pair<uint32_t, uint32_t>>& pair :
      next_use_distances) {
   if (pair.first.type() == type && pair.second.second > distance &&
      !ctx.spills_entry[block_idx].count(pair.first)) {
   to_spill = pair.first;
         }
   assert(distance != 0);
   ctx.spills_entry[block_idx][to_spill] = ctx.allocate_spill_id(to_spill.regClass());
   spilled_registers += to_spill;
                           /* branch block */
   if (block->linear_preds.size() == 1 && !(block->kind & block_kind_loop_exit)) {
      /* keep variables spilled if they are alive and not used in the current block */
   unsigned pred_idx = block->linear_preds[0];
   for (std::pair<Temp, uint32_t> pair : ctx.spills_exit[pred_idx]) {
      if (pair.first.type() != RegType::sgpr) {
         }
   auto next_use_distance_it = next_use_distances.find(pair.first);
   if (next_use_distance_it != next_use_distances.end() &&
      next_use_distance_it->second.first != block_idx) {
   ctx.spills_entry[block_idx].insert(pair);
         }
   if (block->logical_preds.size() == 1) {
      pred_idx = block->logical_preds[0];
   for (std::pair<Temp, uint32_t> pair : ctx.spills_exit[pred_idx]) {
      if (pair.first.type() != RegType::vgpr) {
         }
   auto next_use_distance_it = next_use_distances.find(pair.first);
   if (next_use_distance_it != next_use_distances.end() &&
      next_use_distance_it->second.first != block_idx) {
   ctx.spills_entry[block_idx].insert(pair);
                     /* if register demand is still too high, we just keep all spilled live vars
   * and process the block */
   if (block->register_demand.sgpr - spilled_registers.sgpr > ctx.target_pressure.sgpr) {
      pred_idx = block->linear_preds[0];
   for (std::pair<Temp, uint32_t> pair : ctx.spills_exit[pred_idx]) {
      if (pair.first.type() == RegType::sgpr && next_use_distances.count(pair.first) &&
      ctx.spills_entry[block_idx].insert(pair).second) {
            }
   if (block->register_demand.vgpr - spilled_registers.vgpr > ctx.target_pressure.vgpr &&
      block->logical_preds.size() == 1) {
   pred_idx = block->logical_preds[0];
   for (std::pair<Temp, uint32_t> pair : ctx.spills_exit[pred_idx]) {
      if (pair.first.type() == RegType::vgpr && next_use_distances.count(pair.first) &&
      ctx.spills_entry[block_idx].insert(pair).second) {
                                 /* else: merge block */
            /* keep variables spilled on all incoming paths */
   for (const std::pair<const Temp, std::pair<uint32_t, uint32_t>>& pair : next_use_distances) {
      std::vector<unsigned>& preds =
         /* If it can be rematerialized, keep the variable spilled if all predecessors do not reload
   * it. Otherwise, if any predecessor reloads it, ensure it's reloaded on all other
   * predecessors. The idea is that it's better in practice to rematerialize redundantly than to
   * create lots of phis. */
   /* TODO: test this idea with more than Dawn of War III shaders (the current pipeline-db
   * doesn't seem to exercise this path much) */
   bool remat = ctx.remat.count(pair.first);
   bool spill = !remat;
   uint32_t spill_id = 0;
   for (unsigned pred_idx : preds) {
      /* variable is not even live at the predecessor: probably from a phi */
   if (!ctx.next_use_distances_end[pred_idx].count(pair.first)) {
      spill = false;
      }
   if (!ctx.spills_exit[pred_idx].count(pair.first)) {
      partial_spills.emplace(pair.first, false);
   if (!remat)
      } else {
      partial_spills[pair.first] = true;
   /* it might be that on one incoming path, the variable has a different spill_id, but
   * add_couple_code() will take care of that. */
   spill_id = ctx.spills_exit[pred_idx][pair.first];
   if (remat)
         }
   if (spill) {
      ctx.spills_entry[block_idx][pair.first] = spill_id;
   partial_spills.erase(pair.first);
                  /* same for phis */
   for (aco_ptr<Instruction>& phi : block->instructions) {
      if (!is_phi(phi))
         if (!phi->definitions[0].isTemp())
            std::vector<unsigned>& preds =
         bool is_all_spilled = true;
   for (unsigned i = 0; i < phi->operands.size(); i++) {
      if (phi->operands[i].isUndefined())
         is_all_spilled &= phi->operands[i].isTemp() &&
               if (is_all_spilled) {
      /* The phi is spilled at all predecessors. Keep it spilled. */
   ctx.spills_entry[block_idx][phi->definitions[0].getTemp()] =
            } else {
      /* Phis might increase the register pressure. */
                  /* if reg pressure at first instruction is still too high, add partially spilled variables */
   RegisterDemand reg_pressure = get_live_in_demand(ctx, block_idx);
            while (reg_pressure.exceeds(ctx.target_pressure)) {
      assert(!partial_spills.empty());
   std::map<Temp, bool>::iterator it = partial_spills.begin();
   Temp to_spill = Temp();
   bool is_spilled_or_phi = false;
   unsigned distance = 0;
            while (it != partial_spills.end()) {
               if (it->first.type() == type && ((it->second && !is_spilled_or_phi) ||
                  distance = next_use_distances.at(it->first).second;
   to_spill = it->first;
      }
      }
            ctx.spills_entry[block_idx][to_spill] = ctx.allocate_spill_id(to_spill.regClass());
   partial_spills.erase(to_spill);
   spilled_registers += to_spill;
                  }
      void
   add_coupling_code(spill_ctx& ctx, Block* block, unsigned block_idx)
   {
      /* no coupling code necessary */
   if (block->linear_preds.size() == 0)
            std::vector<aco_ptr<Instruction>> instructions;
   /* branch block: TODO take other branch into consideration */
   if (block->linear_preds.size() == 1 &&
      !(block->kind & (block_kind_loop_exit | block_kind_loop_header))) {
   assert(ctx.processed[block->linear_preds[0]]);
   assert(ctx.register_demand[block_idx].size() == block->instructions.size());
   std::vector<RegisterDemand> reg_demand;
   unsigned insert_idx = 0;
            for (std::pair<const Temp, std::pair<uint32_t, uint32_t>>& live :
                     if (!live.first.is_linear())
         /* still spilled */
                  /* in register at end of predecessor */
   auto spills_exit_it = ctx.spills_exit[pred_idx].find(live.first);
   if (spills_exit_it == ctx.spills_exit[pred_idx].end()) {
      std::map<Temp, Temp>::iterator it = ctx.renames[pred_idx].find(live.first);
   if (it != ctx.renames[pred_idx].end())
                     /* variable is spilled at predecessor and live at current block: create reload instruction */
   Temp new_name = ctx.program->allocateTmp(live.first.regClass());
   aco_ptr<Instruction> reload = do_reload(ctx, live.first, new_name, spills_exit_it->second);
   instructions.emplace_back(std::move(reload));
   reg_demand.push_back(demand_before);
               if (block->logical_preds.size() == 1) {
      do {
      assert(insert_idx < block->instructions.size());
   instructions.emplace_back(std::move(block->instructions[insert_idx]));
   reg_demand.push_back(ctx.register_demand[block_idx][insert_idx]);
               unsigned pred_idx = block->logical_preds[0];
   for (std::pair<const Temp, std::pair<uint32_t, uint32_t>>& live :
      ctx.next_use_distances_start[block_idx]) {
   if (live.first.is_linear())
         /* still spilled */
                  /* in register at end of predecessor */
   auto spills_exit_it = ctx.spills_exit[pred_idx].find(live.first);
   if (spills_exit_it == ctx.spills_exit[pred_idx].end()) {
      std::map<Temp, Temp>::iterator it = ctx.renames[pred_idx].find(live.first);
   if (it != ctx.renames[pred_idx].end())
                     /* variable is spilled at predecessor and live at current block:
   * create reload instruction */
   Temp new_name = ctx.program->allocateTmp(live.first.regClass());
   aco_ptr<Instruction> reload =
         instructions.emplace_back(std::move(reload));
   reg_demand.emplace_back(reg_demand.back());
                  /* combine new reload instructions with original block */
   if (!instructions.empty()) {
      reg_demand.insert(reg_demand.end(),
               ctx.register_demand[block_idx] = std::move(reg_demand);
   instructions.insert(instructions.end(),
                     std::move_iterator<std::vector<aco_ptr<Instruction>>::iterator>(
      }
               /* loop header and merge blocks: check if all (linear) predecessors have been processed */
   for (ASSERTED unsigned pred : block->linear_preds)
            /* iterate the phi nodes for which operands to spill at the predecessor */
   for (aco_ptr<Instruction>& phi : block->instructions) {
      if (!is_phi(phi))
            /* if the phi is not spilled, add to instructions */
   if (!phi->definitions[0].isTemp() ||
      !ctx.spills_entry[block_idx].count(phi->definitions[0].getTemp())) {
   instructions.emplace_back(std::move(phi));
               std::vector<unsigned>& preds =
                  for (unsigned i = 0; i < phi->operands.size(); i++) {
                                    if (spill_op.isTemp()) {
                     std::map<Temp, Temp>::iterator rename_it = ctx.renames[pred_idx].find(var);
   /* prevent the defining instruction from being DCE'd if it could be rematerialized */
                  /* check if variable is already spilled at predecessor */
   auto spilled = ctx.spills_exit[pred_idx].find(var);
   if (spilled != ctx.spills_exit[pred_idx].end()) {
      if (spilled->second != def_spill_id)
                     /* rename if necessary */
   if (rename_it != ctx.renames[pred_idx].end()) {
      spill_op.setTemp(rename_it->second);
                           /* add interferences and affinity */
   for (std::pair<Temp, uint32_t> pair : ctx.spills_exit[pred_idx])
                  aco_ptr<Pseudo_instruction> spill{
         spill->operands[0] = spill_op;
   spill->operands[1] = Operand::c32(spill_id);
   Block& pred = ctx.program->blocks[pred_idx];
   unsigned idx = pred.instructions.size();
   do {
      assert(idx != 0);
      } while (phi->opcode == aco_opcode::p_phi &&
                        /* Add the original name to predecessor's spilled variables */
   if (spill_op.isTemp())
               /* remove phi from instructions */
               /* iterate all (other) spilled variables for which to spill at the predecessor */
   // TODO: would be better to have them sorted: first vgprs and first with longest distance
   for (std::pair<Temp, uint32_t> pair : ctx.spills_entry[block_idx]) {
      std::vector<unsigned> preds =
            for (unsigned pred_idx : preds) {
      /* variable is already spilled at predecessor */
   auto spilled = ctx.spills_exit[pred_idx].find(pair.first);
   if (spilled != ctx.spills_exit[pred_idx].end()) {
      if (spilled->second != pair.second)
                     /* variable is dead at predecessor, it must be from a phi: this works because of CSSA form */
                  /* add interferences between spilled variable and predecessors exit spills */
   for (std::pair<Temp, uint32_t> exit_spill : ctx.spills_exit[pred_idx]) {
      if (exit_spill.first == pair.first)
                     /* variable is in register at predecessor and has to be spilled */
   /* rename if necessary */
   Temp var = pair.first;
   std::map<Temp, Temp>::iterator rename_it = ctx.renames[pred_idx].find(var);
   if (rename_it != ctx.renames[pred_idx].end()) {
      var = rename_it->second;
               aco_ptr<Pseudo_instruction> spill{
         spill->operands[0] = Operand(var);
   spill->operands[1] = Operand::c32(pair.second);
   Block& pred = ctx.program->blocks[pred_idx];
   unsigned idx = pred.instructions.size();
   do {
      assert(idx != 0);
      } while (pair.first.type() == RegType::vgpr &&
         std::vector<aco_ptr<Instruction>>::iterator it = std::next(pred.instructions.begin(), idx);
   pred.instructions.insert(it, std::move(spill));
                  /* iterate phis for which operands to reload */
   for (aco_ptr<Instruction>& phi : instructions) {
      assert(phi->opcode == aco_opcode::p_phi || phi->opcode == aco_opcode::p_linear_phi);
   assert(!phi->definitions[0].isTemp() ||
            std::vector<unsigned>& preds =
         for (unsigned i = 0; i < phi->operands.size(); i++) {
      if (!phi->operands[i].isTemp())
                  /* if the operand was reloaded, rename */
   if (!ctx.spills_exit[pred_idx].count(phi->operands[i].getTemp())) {
      std::map<Temp, Temp>::iterator it =
         if (it != ctx.renames[pred_idx].end()) {
      phi->operands[i].setTemp(it->second);
      } else {
      auto remat_it = ctx.remat.find(phi->operands[i].getTemp());
   if (remat_it != ctx.remat.end()) {
            }
                        /* reload phi operand at end of predecessor block */
   Temp new_name = ctx.program->allocateTmp(tmp.regClass());
   Block& pred = ctx.program->blocks[pred_idx];
   unsigned idx = pred.instructions.size();
   do {
      assert(idx != 0);
      } while (phi->opcode == aco_opcode::p_phi &&
         std::vector<aco_ptr<Instruction>>::iterator it = std::next(pred.instructions.begin(), idx);
                  /* reload spilled exec mask directly to exec */
   if (!phi->definitions[0].isTemp()) {
      assert(phi->definitions[0].isFixed() && phi->definitions[0].physReg() == exec);
   reload->definitions[0] = phi->definitions[0];
      } else {
      ctx.spills_exit[pred_idx].erase(tmp);
   ctx.renames[pred_idx][tmp] = new_name;
                              /* iterate live variables for which to reload */
   // TODO: reload at current block if variable is spilled on all predecessors
   for (std::pair<const Temp, std::pair<uint32_t, uint32_t>>& pair :
      ctx.next_use_distances_start[block_idx]) {
   /* skip spilled variables */
   if (ctx.spills_entry[block_idx].count(pair.first))
         std::vector<unsigned> preds =
            /* variable is dead at predecessor, it must be from a phi */
   bool is_dead = false;
   for (unsigned pred_idx : preds) {
      if (!ctx.next_use_distances_end[pred_idx].count(pair.first))
      }
   if (is_dead)
         for (unsigned pred_idx : preds) {
      /* the variable is not spilled at the predecessor */
                  /* variable is spilled at predecessor and has to be reloaded */
   Temp new_name = ctx.program->allocateTmp(pair.first.regClass());
   Block& pred = ctx.program->blocks[pred_idx];
   unsigned idx = pred.instructions.size();
   do {
      assert(idx != 0);
      } while (pair.first.type() == RegType::vgpr &&
                  aco_ptr<Instruction> reload =
                  ctx.spills_exit[pred.index].erase(pair.first);
               /* check if we have to create a new phi for this variable */
   Temp rename = Temp();
   bool is_same = true;
   for (unsigned pred_idx : preds) {
      if (!ctx.renames[pred_idx].count(pair.first)) {
      if (rename == Temp())
         else
      } else {
      if (rename == Temp())
         else
               if (!is_same)
               if (!is_same) {
      /* the variable was renamed differently in the predecessors: we have to create a phi */
   aco_opcode opcode = pair.first.is_linear() ? aco_opcode::p_linear_phi : aco_opcode::p_phi;
   aco_ptr<Pseudo_instruction> phi{
         rename = ctx.program->allocateTmp(pair.first.regClass());
   for (unsigned i = 0; i < phi->operands.size(); i++) {
      Temp tmp;
   if (ctx.renames[preds[i]].count(pair.first)) {
         } else if (preds[i] >= block_idx) {
         } else {
      tmp = pair.first;
   /* prevent the defining instruction from being DCE'd if it could be rematerialized */
   if (ctx.remat.count(tmp))
      }
      }
   phi->definitions[0] = Definition(rename);
               /* the variable was renamed: add new name to renames */
   if (!(rename == Temp() || rename == pair.first))
               /* combine phis with instructions */
   unsigned idx = 0;
   while (!block->instructions[idx]) {
                  if (!ctx.processed[block_idx]) {
      assert(!(block->kind & block_kind_loop_header));
   RegisterDemand demand_before = get_demand_before(ctx, block_idx, idx);
   ctx.register_demand[block->index].erase(ctx.register_demand[block->index].begin(),
         ctx.register_demand[block->index].insert(ctx.register_demand[block->index].begin(),
               std::vector<aco_ptr<Instruction>>::iterator start = std::next(block->instructions.begin(), idx);
   instructions.insert(
      instructions.end(), std::move_iterator<std::vector<aco_ptr<Instruction>>::iterator>(start),
         }
      void
   process_block(spill_ctx& ctx, unsigned block_idx, Block* block, RegisterDemand spilled_registers)
   {
               std::vector<aco_ptr<Instruction>> instructions;
            /* phis are handled separately */
   while (block->instructions[idx]->opcode == aco_opcode::p_phi ||
                        if (block->register_demand.exceeds(ctx.target_pressure)) {
         } else {
                           while (idx < block->instructions.size()) {
               /* Spilling is handled as part of phis (they should always have the same or higher register
   * demand). If we try to spill here, we might not be able to reduce the register demand enough
   * because there is no path to spill constant/undef phi operands. */
   if (instr->opcode == aco_opcode::p_branch) {
      instructions.emplace_back(std::move(instr));
   idx++;
                        /* rename and reload operands */
   for (Operand& op : instr->operands) {
      if (!op.isTemp())
         if (!current_spills.count(op.getTemp())) {
      /* the Operand is in register: check if it was renamed */
   auto rename_it = ctx.renames[block_idx].find(op.getTemp());
   if (rename_it != ctx.renames[block_idx].end()) {
         } else {
      /* prevent its defining instruction from being DCE'd if it could be rematerialized */
   auto remat_it = ctx.remat.find(op.getTemp());
   if (remat_it != ctx.remat.end()) {
            }
      }
   /* the Operand is spilled: add it to reloads */
   Temp new_tmp = ctx.program->allocateTmp(op.regClass());
   ctx.renames[block_idx][op.getTemp()] = new_tmp;
   reloads[new_tmp] = std::make_pair(op.getTemp(), current_spills[op.getTemp()]);
   current_spills.erase(op.getTemp());
   op.setTemp(new_tmp);
               /* check if register demand is low enough before and after the current instruction */
                                       /* if reg pressure is too high, spill variable with furthest next use */
   while ((new_demand - spilled_registers).exceeds(ctx.target_pressure)) {
      unsigned distance = 0;
   Temp to_spill;
   bool do_rematerialize = false;
   RegType type = RegType::sgpr;
                  for (std::pair<Temp, uint32_t> pair : ctx.local_next_use_distance[idx]) {
      if (pair.first.type() != type)
         bool can_rematerialize = ctx.remat.count(pair.first);
   if (((pair.second > distance && can_rematerialize == do_rematerialize) ||
      (can_rematerialize && !do_rematerialize && pair.second > idx)) &&
   !current_spills.count(pair.first)) {
   to_spill = pair.first;
   distance = pair.second;
                                 /* add interferences with currently spilled variables */
   for (std::pair<Temp, uint32_t> pair : current_spills)
                                       /* rename if necessary */
   if (ctx.renames[block_idx].count(to_spill)) {
                  /* add spill to new instructions */
   aco_ptr<Pseudo_instruction> spill{
         spill->operands[0] = Operand(to_spill);
   spill->operands[1] = Operand::c32(spill_id);
                  /* add reloads and instruction to new instructions */
   for (std::pair<const Temp, std::pair<Temp, uint32_t>>& pair : reloads) {
      aco_ptr<Instruction> reload =
            }
   instructions.emplace_back(std::move(instr));
                  }
      void
   spill_block(spill_ctx& ctx, unsigned block_idx)
   {
               /* determine set of variables which are spilled at the beginning of the block */
            /* add interferences for spilled variables */
   for (auto it = ctx.spills_entry[block_idx].begin(); it != ctx.spills_entry[block_idx].end();
      ++it) {
   for (auto it2 = std::next(it); it2 != ctx.spills_entry[block_idx].end(); ++it2)
               bool is_loop_header = block->loop_nest_depth && ctx.loop_header.top()->index == block_idx;
   if (!is_loop_header) {
      /* add spill/reload code on incoming control flow edges */
                        /* check conditions to process this block */
   bool process = (block->register_demand - spilled_registers).exceeds(ctx.target_pressure) ||
            for (auto it = current_spills.begin(); !process && it != current_spills.end(); ++it) {
      if (ctx.next_use_distances_start[block_idx].at(it->first).first == block_idx)
               assert(ctx.spills_exit[block_idx].empty());
   ctx.spills_exit[block_idx] = current_spills;
   if (process) {
                           /* check if the next block leaves the current loop */
   if (block->loop_nest_depth == 0 ||
      ctx.program->blocks[block_idx + 1].loop_nest_depth >= block->loop_nest_depth)
                  /* preserve original renames at end of loop header block */
            /* add coupling code to all loop header predecessors */
            /* propagate new renames through loop: i.e. repair the SSA */
   renames.swap(ctx.renames[loop_header->index]);
   for (std::pair<Temp, Temp> rename : renames) {
      for (unsigned idx = loop_header->index; idx <= block_idx; idx++) {
                     /* first rename phis */
   while (instr_it != current.instructions.end()) {
      aco_ptr<Instruction>& phi = *instr_it;
   if (phi->opcode != aco_opcode::p_phi && phi->opcode != aco_opcode::p_linear_phi)
         /* no need to rename the loop header phis once again. this happened in
   * add_coupling_code() */
   if (idx == loop_header->index) {
                        for (Operand& op : phi->operands) {
      if (!op.isTemp())
         if (op.getTemp() == rename.first)
      }
               /* variable is not live at beginning of this block */
                  /* if the variable is live at the block's exit, add rename */
                  /* rename all uses in this block */
   bool renamed = false;
   while (!renamed && instr_it != current.instructions.end()) {
      aco_ptr<Instruction>& instr = *instr_it;
   for (Operand& op : instr->operands) {
      if (!op.isTemp())
         if (op.getTemp() == rename.first) {
      op.setTemp(rename.second);
   /* we can stop with this block as soon as the variable is spilled */
   if (instr->opcode == aco_opcode::p_spill)
         }
                     /* remove loop header info from stack */
      }
      Temp
   load_scratch_resource(spill_ctx& ctx, Builder& bld, bool apply_scratch_offset)
   {
      Temp private_segment_buffer = ctx.program->private_segment_buffer;
   if (!private_segment_buffer.bytes()) {
      Temp addr_lo =
         Temp addr_hi =
         private_segment_buffer =
      } else if (ctx.program->stage.hw != AC_HW_COMPUTE_SHADER) {
      private_segment_buffer =
               if (apply_scratch_offset) {
      Temp addr_lo = bld.tmp(s1);
   Temp addr_hi = bld.tmp(s1);
   bld.pseudo(aco_opcode::p_split_vector, Definition(addr_lo), Definition(addr_hi),
            Temp carry = bld.tmp(s1);
   addr_lo = bld.sop2(aco_opcode::s_add_u32, bld.def(s1), bld.scc(Definition(carry)), addr_lo,
         addr_hi = bld.sop2(aco_opcode::s_addc_u32, bld.def(s1), bld.def(s1, scc), addr_hi,
            private_segment_buffer =
               uint32_t rsrc_conf =
            if (ctx.program->gfx_level >= GFX10) {
      rsrc_conf |= S_008F0C_FORMAT(V_008F0C_GFX10_FORMAT_32_FLOAT) |
            } else if (ctx.program->gfx_level <= GFX7) {
      /* dfmt modifies stride on GFX8/GFX9 when ADD_TID_EN=1 */
   rsrc_conf |= S_008F0C_NUM_FORMAT(V_008F0C_BUF_NUM_FORMAT_FLOAT) |
      }
   /* older generations need element size = 4 bytes. element size removed in GFX9 */
   if (ctx.program->gfx_level <= GFX8)
            return bld.pseudo(aco_opcode::p_create_vector, bld.def(s4), private_segment_buffer,
      }
      void
   setup_vgpr_spill_reload(spill_ctx& ctx, Block& block,
               {
               uint32_t offset_range;
   if (ctx.program->gfx_level >= GFX9) {
      offset_range =
      } else {
      if (scratch_size < 4095)
         else
                        Builder rsrc_bld(ctx.program);
   if (block.kind & block_kind_top_level) {
         } else if (ctx.scratch_rsrc == Temp() && (!overflow || ctx.program->gfx_level < GFX9)) {
      Block* tl_block = &block;
   while (!(tl_block->kind & block_kind_top_level))
            /* find p_logical_end */
   std::vector<aco_ptr<Instruction>>& prev_instructions = tl_block->instructions;
   unsigned idx = prev_instructions.size() - 1;
   while (prev_instructions[idx]->opcode != aco_opcode::p_logical_end)
                     /* If spilling overflows the constant offset range at any point, we need to emit the soffset
   * before every spill/reload to avoid increasing register demand.
   */
   Builder offset_bld = rsrc_bld;
   if (overflow)
            *offset = spill_slot * 4;
   if (ctx.program->gfx_level >= GFX9) {
               if (ctx.scratch_rsrc == Temp() || overflow) {
      int32_t saddr = scratch_size - ctx.program->dev.scratch_global_offset_min;
   if ((int32_t)*offset > (int32_t)ctx.program->dev.scratch_global_offset_max) {
      saddr += (int32_t)*offset;
               /* GFX9+ uses scratch_* instructions, which don't use a resource. */
         } else {
      if (ctx.scratch_rsrc == Temp())
            if (overflow) {
      uint32_t soffset =
                     } else {
               }
      void
   spill_vgpr(spill_ctx& ctx, Block& block, std::vector<aco_ptr<Instruction>>& instructions,
         {
               uint32_t spill_id = spill->operands[1].constantValue();
            Temp scratch_offset = ctx.program->scratch_offset;
   unsigned offset;
            assert(spill->operands[0].isTemp());
   Temp temp = spill->operands[0].getTemp();
            Builder bld(ctx.program, &instructions);
   if (temp.size() > 1) {
      Instruction* split{create_instruction<Pseudo_instruction>(aco_opcode::p_split_vector,
         split->operands[0] = Operand(temp);
   for (unsigned i = 0; i < temp.size(); i++)
         bld.insert(split);
   for (unsigned i = 0; i < temp.size(); i++, offset += 4) {
      Temp elem = split->definitions[i].getTemp();
   if (ctx.program->gfx_level >= GFX9) {
      bld.scratch(aco_opcode::scratch_store_dword, Operand(v1), ctx.scratch_rsrc, elem,
      } else {
      Instruction* instr = bld.mubuf(aco_opcode::buffer_store_dword, ctx.scratch_rsrc,
                  } else if (ctx.program->gfx_level >= GFX9) {
      bld.scratch(aco_opcode::scratch_store_dword, Operand(v1), ctx.scratch_rsrc, temp, offset,
      } else {
      Instruction* instr = bld.mubuf(aco_opcode::buffer_store_dword, ctx.scratch_rsrc, Operand(v1),
               }
      void
   reload_vgpr(spill_ctx& ctx, Block& block, std::vector<aco_ptr<Instruction>>& instructions,
         {
      uint32_t spill_id = reload->operands[0].constantValue();
            Temp scratch_offset = ctx.program->scratch_offset;
   unsigned offset;
                     Builder bld(ctx.program, &instructions);
   if (def.size() > 1) {
      Instruction* vec{create_instruction<Pseudo_instruction>(aco_opcode::p_create_vector,
         vec->definitions[0] = def;
   for (unsigned i = 0; i < def.size(); i++, offset += 4) {
      Temp tmp = bld.tmp(v1);
   vec->operands[i] = Operand(tmp);
   if (ctx.program->gfx_level >= GFX9) {
      bld.scratch(aco_opcode::scratch_load_dword, Definition(tmp), Operand(v1),
            } else {
      Instruction* instr =
      bld.mubuf(aco_opcode::buffer_load_dword, Definition(tmp), ctx.scratch_rsrc,
            }
      } else if (ctx.program->gfx_level >= GFX9) {
      bld.scratch(aco_opcode::scratch_load_dword, def, Operand(v1), ctx.scratch_rsrc, offset,
      } else {
      Instruction* instr = bld.mubuf(aco_opcode::buffer_load_dword, def, ctx.scratch_rsrc,
               }
      void
   add_interferences(spill_ctx& ctx, std::vector<bool>& is_assigned, std::vector<uint32_t>& slots,
         {
      for (unsigned other : ctx.interferences[id].second) {
      if (!is_assigned[other])
            RegClass other_rc = ctx.interferences[other].first;
   unsigned slot = slots[other];
         }
      unsigned
   find_available_slot(std::vector<bool>& used, unsigned wave_size, unsigned size, bool is_sgpr)
   {
      unsigned wave_size_minus_one = wave_size - 1;
            while (true) {
      bool available = true;
   for (unsigned i = 0; i < size; i++) {
      if (slot + i < used.size() && used[slot + i]) {
      available = false;
         }
   if (!available) {
      slot++;
               if (is_sgpr && ((slot & wave_size_minus_one) > wave_size - size)) {
      slot = align(slot, wave_size);
                        if (slot + size > used.size())
                  }
      void
   assign_spill_slots_helper(spill_ctx& ctx, RegType type, std::vector<bool>& is_assigned,
         {
               /* assign slots for ids with affinities first */
   for (std::vector<uint32_t>& vec : ctx.affinities) {
      if (ctx.interferences[vec[0]].first.type() != type)
            for (unsigned id : vec) {
                                 unsigned slot = find_available_slot(
            for (unsigned id : vec) {
               if (ctx.is_reloaded[id]) {
      slots[id] = slot;
                     /* assign slots for ids without affinities */
   for (unsigned id = 0; id < ctx.interferences.size(); id++) {
      if (is_assigned[id] || !ctx.is_reloaded[id] || ctx.interferences[id].first.type() != type)
                     unsigned slot = find_available_slot(
            slots[id] = slot;
                  }
      void
   end_unused_spill_vgprs(spill_ctx& ctx, Block& block, std::vector<Temp>& vgpr_spill_temps,
               {
      std::vector<bool> is_used(vgpr_spill_temps.size());
   for (std::pair<Temp, uint32_t> pair : spills) {
      if (pair.first.type() == RegType::sgpr && ctx.is_reloaded[pair.second])
               std::vector<Temp> temps;
   for (unsigned i = 0; i < vgpr_spill_temps.size(); i++) {
      if (vgpr_spill_temps[i].id() && !is_used[i]) {
      temps.push_back(vgpr_spill_temps[i]);
         }
   if (temps.empty() || block.linear_preds.empty())
            aco_ptr<Instruction> destr{create_instruction<Pseudo_instruction>(
         for (unsigned i = 0; i < temps.size(); i++)
            std::vector<aco_ptr<Instruction>>::iterator it = block.instructions.begin();
   while (is_phi(*it))
            }
      void
   assign_spill_slots(spill_ctx& ctx, unsigned spills_to_vgpr)
   {
      std::vector<uint32_t> slots(ctx.interferences.size());
            /* first, handle affinities: just merge all interferences into both spill ids */
   for (std::vector<uint32_t>& vec : ctx.affinities) {
      for (unsigned i = 0; i < vec.size(); i++) {
      for (unsigned j = i + 1; j < vec.size(); j++) {
      assert(vec[i] != vec[j]);
   bool reloaded = ctx.is_reloaded[vec[i]] || ctx.is_reloaded[vec[j]];
   ctx.is_reloaded[vec[i]] = reloaded;
            }
   for (ASSERTED uint32_t i = 0; i < ctx.interferences.size(); i++)
      for (ASSERTED uint32_t id : ctx.interferences[i].second)
         /* for each spill slot, assign as many spill ids as possible */
   assign_spill_slots_helper(ctx, RegType::sgpr, is_assigned, slots, &ctx.sgpr_spill_slots);
            for (unsigned id = 0; id < is_assigned.size(); id++)
            for (std::vector<uint32_t>& vec : ctx.affinities) {
      for (unsigned i = 0; i < vec.size(); i++) {
      for (unsigned j = i + 1; j < vec.size(); j++) {
      assert(is_assigned[vec[i]] == is_assigned[vec[j]]);
   if (!is_assigned[vec[i]])
         assert(ctx.is_reloaded[vec[i]] == ctx.is_reloaded[vec[j]]);
   assert(ctx.interferences[vec[i]].first.type() ==
                           /* hope, we didn't mess up */
   std::vector<Temp> vgpr_spill_temps((ctx.sgpr_spill_slots + ctx.wave_size - 1) / ctx.wave_size);
            /* replace pseudo instructions with actual hardware instructions */
   unsigned last_top_level_block_idx = 0;
               if (block.kind & block_kind_top_level) {
                        /* If the block has no predecessors (for example in RT resume shaders),
   * we cannot reuse the current scratch_rsrc temp because its definition is unreachable */
   if (block.linear_preds.empty())
               std::vector<aco_ptr<Instruction>>::iterator it;
   std::vector<aco_ptr<Instruction>> instructions;
   instructions.reserve(block.instructions.size());
   Builder bld(ctx.program, &instructions);
                                 if (!ctx.is_reloaded[spill_id]) {
         } else if (!is_assigned[spill_id]) {
         } else if (ctx.interferences[spill_id].first.type() == RegType::vgpr) {
                                    /* check if the linear vgpr already exists */
   if (vgpr_spill_temps[spill_slot / ctx.wave_size] == Temp()) {
      Temp linear_vgpr = ctx.program->allocateTmp(v1.as_linear());
   vgpr_spill_temps[spill_slot / ctx.wave_size] = linear_vgpr;
   aco_ptr<Pseudo_instruction> create{create_instruction<Pseudo_instruction>(
         create->definitions[0] = Definition(linear_vgpr);
   /* find the right place to insert this definition */
   if (last_top_level_block_idx == block.index) {
      /* insert right before the current instruction */
      } else {
      assert(last_top_level_block_idx < block.index);
   /* insert before the branch at last top level block */
   std::vector<aco_ptr<Instruction>>& block_instrs =
                        /* spill sgpr: just add the vgpr temp to operands */
   Pseudo_instruction* spill =
         spill->operands[0] = Operand(vgpr_spill_temps[spill_slot / ctx.wave_size]);
   spill->operands[1] = Operand::c32(spill_slot % ctx.wave_size);
                  } else if ((*it)->opcode == aco_opcode::p_reload) {
                     if (!is_assigned[spill_id]) {
         } else if (ctx.interferences[spill_id].first.type() == RegType::vgpr) {
                           /* check if the linear vgpr already exists */
   if (vgpr_spill_temps[spill_slot / ctx.wave_size] == Temp()) {
      Temp linear_vgpr = ctx.program->allocateTmp(v1.as_linear());
   vgpr_spill_temps[spill_slot / ctx.wave_size] = linear_vgpr;
   aco_ptr<Pseudo_instruction> create{create_instruction<Pseudo_instruction>(
         create->definitions[0] = Definition(linear_vgpr);
   /* find the right place to insert this definition */
   if (last_top_level_block_idx == block.index) {
      /* insert right before the current instruction */
      } else {
      assert(last_top_level_block_idx < block.index);
   /* insert before the branch at last top level block */
   std::vector<aco_ptr<Instruction>>& block_instrs =
                        /* reload sgpr: just add the vgpr temp to operands */
   Pseudo_instruction* reload = create_instruction<Pseudo_instruction>(
         reload->operands[0] = Operand(vgpr_spill_temps[spill_slot / ctx.wave_size]);
   reload->operands[1] = Operand::c32(spill_slot % ctx.wave_size);
   reload->definitions[0] = (*it)->definitions[0];
         } else if (!ctx.unused_remats.count(it->get())) {
            }
               /* update required scratch memory */
      }
      } /* end namespace */
      void
   spill(Program* program, live& live_vars)
   {
      program->config->spilled_vgprs = 0;
                     /* no spilling when register pressure is low enough */
   if (program->num_waves > 0)
            /* lower to CSSA before spilling to ensure correctness w.r.t. phis */
            /* calculate target register demand */
   const RegisterDemand demand = program->max_reg_demand; /* current max */
   const uint16_t sgpr_limit = get_addr_sgpr_from_waves(program, program->min_waves);
   const uint16_t vgpr_limit = get_addr_vgpr_from_waves(program, program->min_waves);
   uint16_t extra_vgprs = 0;
            /* calculate extra VGPRs required for spilling SGPRs */
   if (demand.sgpr > sgpr_limit) {
      unsigned sgpr_spills = demand.sgpr - sgpr_limit;
      }
   /* add extra SGPRs required for spilling VGPRs */
   if (demand.vgpr + extra_vgprs > vgpr_limit) {
      if (program->gfx_level >= GFX9)
         else
         if (demand.sgpr + extra_sgprs > sgpr_limit) {
      /* re-calculate in case something has changed */
   unsigned sgpr_spills = demand.sgpr + extra_sgprs - sgpr_limit;
         }
   /* the spiller has to target the following register demand */
            /* initialize ctx */
   spill_ctx ctx(target, program, live_vars.register_demand);
   compute_global_next_uses(ctx);
            /* create spills and reloads */
   for (unsigned i = 0; i < program->blocks.size(); i++)
            /* assign spill slots and DCE rematerialized code */
            /* update live variable information */
               }
      } // namespace aco
