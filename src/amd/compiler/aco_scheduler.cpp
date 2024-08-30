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
      #include "common/amdgfxregs.h"
      #include <algorithm>
   #include <unordered_set>
   #include <vector>
      #define SMEM_WINDOW_SIZE    (350 - ctx.num_waves * 35)
   #define VMEM_WINDOW_SIZE    (1024 - ctx.num_waves * 64)
   #define POS_EXP_WINDOW_SIZE 512
   #define SMEM_MAX_MOVES      (64 - ctx.num_waves * 4)
   #define VMEM_MAX_MOVES      (256 - ctx.num_waves * 16)
   /* creating clauses decreases def-use distances, so make it less aggressive the lower num_waves is */
   #define VMEM_CLAUSE_MAX_GRAB_DIST (ctx.num_waves * 2)
   #define POS_EXP_MAX_MOVES         512
      namespace aco {
      enum MoveResult {
      move_success,
   move_fail_ssa,
   move_fail_rar,
      };
      /**
   * Cursor for downwards moves, where a single instruction is moved towards
   * or below a group of instruction that hardware can execute as a clause.
   */
   struct DownwardsCursor {
               int insert_idx_clause; /* First clause instruction */
            /* Maximum demand of all clause instructions,
   * i.e. from insert_idx_clause (inclusive) to insert_idx (exclusive) */
   RegisterDemand clause_demand;
   /* Maximum demand of instructions from source_idx to insert_idx_clause (both exclusive) */
            DownwardsCursor(int current_idx, RegisterDemand initial_clause_demand)
      : source_idx(current_idx - 1), insert_idx_clause(current_idx), insert_idx(current_idx + 1),
                  };
      /**
   * Cursor for upwards moves, where a single instruction is moved below
   * another instruction.
   */
   struct UpwardsCursor {
      int source_idx; /* Current instruction to consider for moving */
            /* Maximum demand of instructions from insert_idx (inclusive) to source_idx (exclusive) */
            UpwardsCursor(int source_idx_) : source_idx(source_idx_)
   {
                  bool has_insert_idx() const { return insert_idx != -1; }
      };
      struct MoveState {
               Block* block;
   Instruction* current;
   RegisterDemand* register_demand; /* demand per instruction */
            std::vector<bool> depends_on;
   /* Two are needed because, for downwards VMEM scheduling, one needs to
   * exclude the instructions in the clause, since new instructions in the
   * clause are not moved past any other instructions in the clause. */
   std::vector<bool> RAR_dependencies;
            /* for moving instructions before the current instruction to after it */
   DownwardsCursor downwards_init(int current_idx, bool improved_rar, bool may_form_clauses);
   MoveResult downwards_move(DownwardsCursor&, bool clause);
            /* for moving instructions after the first use of the current instruction upwards */
   UpwardsCursor upwards_init(int source_idx, bool improved_rar);
   bool upwards_check_deps(UpwardsCursor&);
   void upwards_update_insert_idx(UpwardsCursor&);
   MoveResult upwards_move(UpwardsCursor&);
      };
      struct sched_ctx {
      amd_gfx_level gfx_level;
   int16_t num_waves;
   int16_t last_SMEM_stall;
   int last_SMEM_dep_idx;
   MoveState mv;
   bool schedule_pos_exports = true;
      };
      /* This scheduler is a simple bottom-up pass based on ideas from
   * "A Novel Lightweight Instruction Scheduling Algorithm for Just-In-Time Compiler"
   * from Xiaohua Shi and Peng Guo.
   * The basic approach is to iterate over all instructions. When a memory instruction
   * is encountered it tries to move independent instructions from above and below
   * between the memory instruction and it's first user.
   * The novelty is that this scheduler cares for the current register pressure:
   * Instructions will only be moved if the register pressure won't exceed a certain bound.
   */
      template <typename T>
   void
   move_element(T begin_it, size_t idx, size_t before)
   {
      if (idx < before) {
      auto begin = std::next(begin_it, idx);
   auto end = std::next(begin_it, before);
      } else if (idx > before) {
      auto begin = std::next(begin_it, before);
   auto end = std::next(begin_it, idx + 1);
         }
      void
   DownwardsCursor::verify_invariants(const RegisterDemand* register_demand)
   {
      assert(source_idx < insert_idx_clause);
         #ifndef NDEBUG
      RegisterDemand reference_demand;
   for (int i = source_idx + 1; i < insert_idx_clause; ++i) {
         }
            reference_demand = {};
   for (int i = insert_idx_clause; i < insert_idx; ++i) {
         }
      #endif
   }
      DownwardsCursor
   MoveState::downwards_init(int current_idx, bool improved_rar_, bool may_form_clauses)
   {
               std::fill(depends_on.begin(), depends_on.end(), false);
   if (improved_rar) {
      std::fill(RAR_dependencies.begin(), RAR_dependencies.end(), false);
   if (may_form_clauses)
               for (const Operand& op : current->operands) {
      if (op.isTemp()) {
      depends_on[op.tempId()] = true;
   if (improved_rar && op.isFirstKill())
                  DownwardsCursor cursor(current_idx, register_demand[current_idx]);
   cursor.verify_invariants(register_demand);
      }
      /* If add_to_clause is true, the current clause is extended by moving the
   * instruction at source_idx in front of the clause. Otherwise, the instruction
   * is moved past the end of the clause without extending it */
   MoveResult
   MoveState::downwards_move(DownwardsCursor& cursor, bool add_to_clause)
   {
               for (const Definition& def : instr->definitions)
      if (def.isTemp() && depends_on[def.tempId()])
         /* check if one of candidate's operands is killed by depending instruction */
   std::vector<bool>& RAR_deps =
         for (const Operand& op : instr->operands) {
      if (op.isTemp() && RAR_deps[op.tempId()]) {
      // FIXME: account for difference in register pressure
                  if (add_to_clause) {
      for (const Operand& op : instr->operands) {
      if (op.isTemp()) {
      depends_on[op.tempId()] = true;
   if (op.isFirstKill())
                     const int dest_insert_idx = add_to_clause ? cursor.insert_idx_clause : cursor.insert_idx;
   RegisterDemand register_pressure = cursor.total_demand;
   if (!add_to_clause) {
                  /* Check the new demand of the instructions being moved over */
   const RegisterDemand candidate_diff = get_live_changes(instr);
   if (RegisterDemand(register_pressure - candidate_diff).exceeds(max_registers))
            /* New demand for the moved instruction */
   const RegisterDemand temp = get_temp_registers(instr);
   const RegisterDemand temp2 = get_temp_registers(block->instructions[dest_insert_idx - 1]);
   const RegisterDemand new_demand = register_demand[dest_insert_idx - 1] - temp2 + temp;
   if (new_demand.exceeds(max_registers))
            /* move the candidate below the memory load */
            /* update register pressure */
   move_element(register_demand, cursor.source_idx, dest_insert_idx);
   for (int i = cursor.source_idx; i < dest_insert_idx - 1; i++)
         register_demand[dest_insert_idx - 1] = new_demand;
   cursor.insert_idx_clause--;
   if (cursor.source_idx != cursor.insert_idx_clause) {
      /* Update demand if we moved over any instructions before the clause */
      } else {
         }
   if (add_to_clause) {
         } else {
      cursor.clause_demand -= candidate_diff;
               cursor.source_idx--;
   cursor.verify_invariants(register_demand);
      }
      void
   MoveState::downwards_skip(DownwardsCursor& cursor)
   {
               for (const Operand& op : instr->operands) {
      if (op.isTemp()) {
      depends_on[op.tempId()] = true;
   if (improved_rar && op.isFirstKill()) {
      RAR_dependencies[op.tempId()] = true;
            }
   cursor.total_demand.update(register_demand[cursor.source_idx]);
   cursor.source_idx--;
      }
      void
   UpwardsCursor::verify_invariants(const RegisterDemand* register_demand)
   {
   #ifndef NDEBUG
      if (!has_insert_idx()) {
                           RegisterDemand reference_demand;
   for (int i = insert_idx; i < source_idx; ++i) {
         }
      #endif
   }
      UpwardsCursor
   MoveState::upwards_init(int source_idx, bool improved_rar_)
   {
               std::fill(depends_on.begin(), depends_on.end(), false);
            for (const Definition& def : current->definitions) {
      if (def.isTemp())
                  }
      bool
   MoveState::upwards_check_deps(UpwardsCursor& cursor)
   {
      aco_ptr<Instruction>& instr = block->instructions[cursor.source_idx];
   for (const Operand& op : instr->operands) {
      if (op.isTemp() && depends_on[op.tempId()])
      }
      }
      void
   MoveState::upwards_update_insert_idx(UpwardsCursor& cursor)
   {
      cursor.insert_idx = cursor.source_idx;
      }
      MoveResult
   MoveState::upwards_move(UpwardsCursor& cursor)
   {
               aco_ptr<Instruction>& instr = block->instructions[cursor.source_idx];
   for (const Operand& op : instr->operands) {
      if (op.isTemp() && depends_on[op.tempId()])
               /* check if candidate uses/kills an operand which is used by a dependency */
   for (const Operand& op : instr->operands) {
      if (op.isTemp() && (!improved_rar || op.isFirstKill()) && RAR_dependencies[op.tempId()])
               /* check if register pressure is low enough: the diff is negative if register pressure is
   * decreased */
   const RegisterDemand candidate_diff = get_live_changes(instr);
   const RegisterDemand temp = get_temp_registers(instr);
   if (RegisterDemand(cursor.total_demand + candidate_diff).exceeds(max_registers))
         const RegisterDemand temp2 = get_temp_registers(block->instructions[cursor.insert_idx - 1]);
   const RegisterDemand new_demand =
         if (new_demand.exceeds(max_registers))
            /* move the candidate above the insert_idx */
            /* update register pressure */
   move_element(register_demand, cursor.source_idx, cursor.insert_idx);
   register_demand[cursor.insert_idx] = new_demand;
   for (int i = cursor.insert_idx + 1; i <= cursor.source_idx; i++)
                           cursor.insert_idx++;
                        }
      void
   MoveState::upwards_skip(UpwardsCursor& cursor)
   {
      if (cursor.has_insert_idx()) {
      aco_ptr<Instruction>& instr = block->instructions[cursor.source_idx];
   for (const Definition& def : instr->definitions) {
      if (def.isTemp())
      }
   for (const Operand& op : instr->operands) {
      if (op.isTemp())
      }
                           }
      bool
   is_done_sendmsg(amd_gfx_level gfx_level, const Instruction* instr)
   {
      if (gfx_level <= GFX10_3 && instr->opcode == aco_opcode::s_sendmsg)
            }
      bool
   is_pos_prim_export(amd_gfx_level gfx_level, const Instruction* instr)
   {
      /* Because of NO_PC_EXPORT=1, a done=1 position or primitive export can launch PS waves before
   * the NGG/VS wave finishes if there are no parameter exports.
   */
   return instr->opcode == aco_opcode::exp && instr->exp().dest >= V_008DFC_SQ_EXP_POS &&
      }
      memory_sync_info
   get_sync_info_with_hack(const Instruction* instr)
   {
      memory_sync_info sync = get_sync_info(instr);
   if (instr->isSMEM() && !instr->operands.empty() && instr->operands[0].bytes() == 16) {
      // FIXME: currently, it doesn't seem beneficial to omit this due to how our scheduler works
   sync.storage = (storage_class)(sync.storage | storage_buffer);
   sync.semantics =
      }
      }
      struct memory_event_set {
               unsigned bar_acquire;
   unsigned bar_release;
            unsigned access_acquire;
   unsigned access_release;
   unsigned access_relaxed;
      };
      struct hazard_query {
      amd_gfx_level gfx_level;
   bool contains_spill;
   bool contains_sendmsg;
   bool uses_exec;
   bool writes_exec;
   memory_event_set mem_events;
   unsigned aliasing_storage;      /* storage classes which are accessed (non-SMEM) */
      };
      void
   init_hazard_query(const sched_ctx& ctx, hazard_query* query)
   {
      query->gfx_level = ctx.gfx_level;
   query->contains_spill = false;
   query->contains_sendmsg = false;
   query->uses_exec = false;
   query->writes_exec = false;
   memset(&query->mem_events, 0, sizeof(query->mem_events));
   query->aliasing_storage = 0;
      }
      void
   add_memory_event(amd_gfx_level gfx_level, memory_event_set* set, Instruction* instr,
         {
      set->has_control_barrier |= is_done_sendmsg(gfx_level, instr);
   set->has_control_barrier |= is_pos_prim_export(gfx_level, instr);
   if (instr->opcode == aco_opcode::p_barrier) {
      Pseudo_barrier_instruction& bar = instr->barrier();
   if (bar.sync.semantics & semantic_acquire)
         if (bar.sync.semantics & semantic_release)
                              if (!sync->storage)
            if (sync->semantics & semantic_acquire)
         if (sync->semantics & semantic_release)
            if (!(sync->semantics & semantic_private)) {
      if (sync->semantics & semantic_atomic)
         else
         }
      void
   add_to_hazard_query(hazard_query* query, Instruction* instr)
   {
      if (instr->opcode == aco_opcode::p_spill || instr->opcode == aco_opcode::p_reload)
         query->contains_sendmsg |= instr->opcode == aco_opcode::s_sendmsg;
   query->uses_exec |= needs_exec_mask(instr);
   for (const Definition& def : instr->definitions) {
      if (def.isFixed() && def.physReg() == exec)
                                 if (!(sync.semantics & semantic_can_reorder)) {
      unsigned storage = sync.storage;
   /* images and buffer/global memory can alias */ // TODO: more precisely, buffer images and
         if (storage & (storage_buffer | storage_image))
         if (instr->isSMEM())
         else
         }
      enum HazardResult {
      hazard_success,
   hazard_fail_reorder_vmem_smem,
   hazard_fail_reorder_ds,
   hazard_fail_reorder_sendmsg,
   hazard_fail_spill,
   hazard_fail_export,
   hazard_fail_barrier,
   /* Must stop at these failures. The hazard query code doesn't consider them
   * when added. */
   hazard_fail_exec,
      };
      HazardResult
   perform_hazard_query(hazard_query* query, Instruction* instr, bool upwards)
   {
      /* don't schedule discards downwards */
   if (!upwards && instr->opcode == aco_opcode::p_exit_early_if)
            /* In Primitive Ordered Pixel Shading, await overlapped waves as late as possible, and notify
   * overlapping waves that they can continue execution as early as possible.
   */
   if (upwards) {
      if (instr->opcode == aco_opcode::p_pops_gfx9_add_exiting_wave_id ||
      (instr->opcode == aco_opcode::s_wait_event &&
   !(instr->sopp().imm & wait_event_imm_dont_wait_export_ready))) {
         } else {
      if (instr->opcode == aco_opcode::p_pops_gfx9_ordered_section_done) {
                     if (query->uses_exec || query->writes_exec) {
      for (const Definition& def : instr->definitions) {
      if (def.isFixed() && def.physReg() == exec)
         }
   if (query->writes_exec && needs_exec_mask(instr))
            /* Don't move exports so that they stay closer together.
   * Since GFX11, export order matters. MRTZ must come first,
   * then color exports sorted from first to last.
   * Also, with Primitive Ordered Pixel Shading on GFX11+, the `done` export must not be moved
   * above the memory accesses before the queue family scope (more precisely, fragment interlock
   * scope, but it's not available in ACO) release barrier that is expected to be inserted before
   * the export, as well as before any `s_wait_event export_ready` which enters the ordered
   * section, because the `done` export exits the ordered section.
   */
   if (instr->isEXP() || instr->opcode == aco_opcode::p_dual_src_export_gfx11)
            /* don't move non-reorderable instructions */
   if (instr->opcode == aco_opcode::s_memtime || instr->opcode == aco_opcode::s_memrealtime ||
      instr->opcode == aco_opcode::s_setprio || instr->opcode == aco_opcode::s_getreg_b32 ||
   instr->opcode == aco_opcode::p_init_scratch ||
   instr->opcode == aco_opcode::p_jump_to_epilog ||
   instr->opcode == aco_opcode::s_sendmsg_rtn_b32 ||
   instr->opcode == aco_opcode::s_sendmsg_rtn_b64 ||
   instr->opcode == aco_opcode::p_end_with_regs)
         memory_event_set instr_set;
   memset(&instr_set, 0, sizeof(instr_set));
   memory_sync_info sync = get_sync_info_with_hack(instr);
            memory_event_set* first = &instr_set;
   memory_event_set* second = &query->mem_events;
   if (upwards)
            /* everything after barrier(acquire) happens after the atomics/control_barriers before
   * everything after load(acquire) happens after the load
   */
   if ((first->has_control_barrier || first->access_atomic) && second->bar_acquire)
         if (((first->access_acquire || first->bar_acquire) && second->bar_classes) ||
      ((first->access_acquire | first->bar_acquire) &
   (second->access_relaxed | second->access_atomic)))
         /* everything before barrier(release) happens before the atomics/control_barriers after *
   * everything before store(release) happens before the store
   */
   if (first->bar_release && (second->has_control_barrier || second->access_atomic))
         if ((first->bar_classes && (second->bar_release || second->access_release)) ||
      ((first->access_relaxed | first->access_atomic) &
   (second->bar_release | second->access_release)))
         /* don't move memory barriers around other memory barriers */
   if (first->bar_classes && second->bar_classes)
            /* Don't move memory accesses to before control barriers. I don't think
   * this is necessary for the Vulkan memory model, but it might be for GLSL450. */
   unsigned control_classes =
         if (first->has_control_barrier &&
      ((second->access_atomic | second->access_relaxed) & control_classes))
         /* don't move memory loads/stores past potentially aliasing loads/stores */
   unsigned aliasing_storage =
         if ((sync.storage & aliasing_storage) && !(sync.semantics & semantic_can_reorder)) {
      unsigned intersect = sync.storage & aliasing_storage;
   if (intersect & storage_shared)
                     if ((instr->opcode == aco_opcode::p_spill || instr->opcode == aco_opcode::p_reload) &&
      query->contains_spill)
         if (instr->opcode == aco_opcode::s_sendmsg && query->contains_sendmsg)
               }
      void
   schedule_SMEM(sched_ctx& ctx, Block* block, std::vector<RegisterDemand>& register_demand,
         {
      assert(idx != 0);
   int window_size = SMEM_WINDOW_SIZE;
   int max_moves = SMEM_MAX_MOVES;
            /* don't move s_memtime/s_memrealtime */
   if (current->opcode == aco_opcode::s_memtime || current->opcode == aco_opcode::s_memrealtime ||
      current->opcode == aco_opcode::s_sendmsg_rtn_b32 ||
   current->opcode == aco_opcode::s_sendmsg_rtn_b64)
         /* first, check if we have instructions before current to move down */
   hazard_query hq;
   init_hazard_query(ctx, &hq);
                     for (int candidate_idx = idx - 1; k < max_moves && candidate_idx > (int)idx - window_size;
      candidate_idx--) {
   assert(candidate_idx >= 0);
   assert(candidate_idx == cursor.source_idx);
            /* break if we'd make the previous SMEM instruction stall */
   bool can_stall_prev_smem =
         if (can_stall_prev_smem && ctx.last_SMEM_stall >= 0)
            /* break when encountering another MEM instruction, logical_start or barriers */
   if (candidate->opcode == aco_opcode::p_logical_start)
         /* only move VMEM instructions below descriptor loads. be more aggressive at higher num_waves
   * to help create more vmem clauses */
   if ((candidate->isVMEM() || candidate->isFlatLike()) &&
      (cursor.insert_idx - cursor.source_idx > (ctx.num_waves * 4) ||
   current->operands[0].size() == 4))
      /* don't move descriptor loads below buffer loads */
   if (candidate->isSMEM() && !candidate->operands.empty() && current->operands[0].size() == 4 &&
                           HazardResult haz = perform_hazard_query(&hq, candidate.get(), false);
   if (haz == hazard_fail_reorder_ds || haz == hazard_fail_spill ||
      haz == hazard_fail_reorder_sendmsg || haz == hazard_fail_barrier ||
   haz == hazard_fail_export)
      else if (haz != hazard_success)
            /* don't use LDS/GDS instructions to hide latency since it can
   * significantly worsen LDS scheduling */
   if (candidate->isDS() || !can_move_down) {
      add_to_hazard_query(&hq, candidate.get());
   ctx.mv.downwards_skip(cursor);
               MoveResult res = ctx.mv.downwards_move(cursor, false);
   if (res == move_fail_ssa || res == move_fail_rar) {
      add_to_hazard_query(&hq, candidate.get());
   ctx.mv.downwards_skip(cursor);
      } else if (res == move_fail_pressure) {
                  if (candidate_idx < ctx.last_SMEM_dep_idx)
                     /* find the first instruction depending on current or find another MEM */
            bool found_dependency = false;
   /* second, check if we have instructions after current to move up */
   for (int candidate_idx = idx + 1; k < max_moves && candidate_idx < (int)idx + window_size;
      candidate_idx++) {
   assert(candidate_idx == up_cursor.source_idx);
   assert(candidate_idx < (int)block->instructions.size());
            if (candidate->opcode == aco_opcode::p_logical_end)
            /* check if candidate depends on current */
   bool is_dependency = !found_dependency && !ctx.mv.upwards_check_deps(up_cursor);
   /* no need to steal from following VMEM instructions */
   if (is_dependency && (candidate->isVMEM() || candidate->isFlatLike()))
            if (found_dependency) {
      HazardResult haz = perform_hazard_query(&hq, candidate.get(), true);
   if (haz == hazard_fail_reorder_ds || haz == hazard_fail_spill ||
      haz == hazard_fail_reorder_sendmsg || haz == hazard_fail_barrier ||
   haz == hazard_fail_export)
      else if (haz != hazard_success)
               if (is_dependency) {
      if (!found_dependency) {
      ctx.mv.upwards_update_insert_idx(up_cursor);
   init_hazard_query(ctx, &hq);
                  if (is_dependency || !found_dependency) {
      if (found_dependency)
         else
         ctx.mv.upwards_skip(up_cursor);
               MoveResult res = ctx.mv.upwards_move(up_cursor);
   if (res == move_fail_ssa || res == move_fail_rar) {
      /* no need to steal from following VMEM instructions */
   if (res == move_fail_ssa && (candidate->isVMEM() || candidate->isFlatLike()))
         add_to_hazard_query(&hq, candidate.get());
   ctx.mv.upwards_skip(up_cursor);
      } else if (res == move_fail_pressure) {
         }
               ctx.last_SMEM_dep_idx = found_dependency ? up_cursor.insert_idx : 0;
      }
      void
   schedule_VMEM(sched_ctx& ctx, Block* block, std::vector<RegisterDemand>& register_demand,
         {
      assert(idx != 0);
   int window_size = VMEM_WINDOW_SIZE;
   int max_moves = VMEM_MAX_MOVES;
   int clause_max_grab_dist = VMEM_CLAUSE_MAX_GRAB_DIST;
   bool only_clauses = false;
            /* first, check if we have instructions before current to move down */
   hazard_query indep_hq;
   hazard_query clause_hq;
   init_hazard_query(ctx, &indep_hq);
   init_hazard_query(ctx, &clause_hq);
                     for (int candidate_idx = idx - 1; k < max_moves && candidate_idx > (int)idx - window_size;
      candidate_idx--) {
   assert(candidate_idx == cursor.source_idx);
   assert(candidate_idx >= 0);
   aco_ptr<Instruction>& candidate = block->instructions[candidate_idx];
            /* break when encountering another VMEM instruction, logical_start or barriers */
   if (candidate->opcode == aco_opcode::p_logical_start)
            /* break if we'd make the previous SMEM instruction stall */
   bool can_stall_prev_smem =
         if (can_stall_prev_smem && ctx.last_SMEM_stall >= 0)
            bool part_of_clause = false;
   if (current->isVMEM() == candidate->isVMEM()) {
      int grab_dist = cursor.insert_idx_clause - candidate_idx;
   /* We can't easily tell how much this will decrease the def-to-use
   * distances, so just use how far it will be moved as a heuristic. */
   part_of_clause =
               /* if current depends on candidate, add additional dependencies and continue */
   bool can_move_down = !is_vmem || part_of_clause || candidate->definitions.empty();
   if (only_clauses) {
      /* In case of high register pressure, only try to form clauses,
   * and only if the previous clause is not larger
   * than the current one will be.
   */
   if (part_of_clause) {
      int clause_size = cursor.insert_idx - cursor.insert_idx_clause;
   int prev_clause_size = 1;
   while (should_form_clause(current,
               if (prev_clause_size > clause_size + 1)
      } else {
            }
   HazardResult haz =
         if (haz == hazard_fail_reorder_ds || haz == hazard_fail_spill ||
      haz == hazard_fail_reorder_sendmsg || haz == hazard_fail_barrier ||
   haz == hazard_fail_export)
      else if (haz != hazard_success)
            if (!can_move_down) {
      if (part_of_clause)
         add_to_hazard_query(&indep_hq, candidate.get());
   add_to_hazard_query(&clause_hq, candidate.get());
   ctx.mv.downwards_skip(cursor);
               Instruction* candidate_ptr = candidate.get();
   MoveResult res = ctx.mv.downwards_move(cursor, part_of_clause);
   if (res == move_fail_ssa || res == move_fail_rar) {
      if (part_of_clause)
         add_to_hazard_query(&indep_hq, candidate.get());
   add_to_hazard_query(&clause_hq, candidate.get());
   ctx.mv.downwards_skip(cursor);
      } else if (res == move_fail_pressure) {
      only_clauses = true;
   if (part_of_clause)
         add_to_hazard_query(&indep_hq, candidate.get());
   add_to_hazard_query(&clause_hq, candidate.get());
   ctx.mv.downwards_skip(cursor);
      }
   if (part_of_clause)
         else
         if (candidate_idx < ctx.last_SMEM_dep_idx)
               /* find the first instruction depending on current or find another VMEM */
            bool found_dependency = false;
   /* second, check if we have instructions after current to move up */
   for (int candidate_idx = idx + 1; k < max_moves && candidate_idx < (int)idx + window_size;
      candidate_idx++) {
   assert(candidate_idx == up_cursor.source_idx);
   assert(candidate_idx < (int)block->instructions.size());
   aco_ptr<Instruction>& candidate = block->instructions[candidate_idx];
            if (candidate->opcode == aco_opcode::p_logical_end)
            /* check if candidate depends on current */
   bool is_dependency = false;
   if (found_dependency) {
      HazardResult haz = perform_hazard_query(&indep_hq, candidate.get(), true);
   if (haz == hazard_fail_reorder_ds || haz == hazard_fail_spill ||
      haz == hazard_fail_reorder_vmem_smem || haz == hazard_fail_reorder_sendmsg ||
   haz == hazard_fail_barrier || haz == hazard_fail_export)
      else if (haz != hazard_success)
               is_dependency |= !found_dependency && !ctx.mv.upwards_check_deps(up_cursor);
   if (is_dependency) {
      if (!found_dependency) {
      ctx.mv.upwards_update_insert_idx(up_cursor);
   init_hazard_query(ctx, &indep_hq);
         } else if (is_vmem) {
      /* don't move up dependencies of other VMEM instructions */
   for (const Definition& def : candidate->definitions) {
      if (def.isTemp())
                  if (is_dependency || !found_dependency) {
      if (found_dependency)
         else
         ctx.mv.upwards_skip(up_cursor);
               MoveResult res = ctx.mv.upwards_move(up_cursor);
   if (res == move_fail_ssa || res == move_fail_rar) {
      add_to_hazard_query(&indep_hq, candidate.get());
   ctx.mv.upwards_skip(up_cursor);
      } else if (res == move_fail_pressure) {
         }
         }
      void
   schedule_position_export(sched_ctx& ctx, Block* block, std::vector<RegisterDemand>& register_demand,
         {
      assert(idx != 0);
   int window_size = POS_EXP_WINDOW_SIZE / ctx.schedule_pos_export_div;
   int max_moves = POS_EXP_MAX_MOVES / ctx.schedule_pos_export_div;
                     hazard_query hq;
   init_hazard_query(ctx, &hq);
            for (int candidate_idx = idx - 1; k < max_moves && candidate_idx > (int)idx - window_size;
      candidate_idx--) {
   assert(candidate_idx >= 0);
            if (candidate->opcode == aco_opcode::p_logical_start)
         if (candidate->isVMEM() || candidate->isSMEM() || candidate->isFlatLike())
            HazardResult haz = perform_hazard_query(&hq, candidate.get(), false);
   if (haz == hazard_fail_exec || haz == hazard_fail_unreorderable)
            if (haz != hazard_success) {
      add_to_hazard_query(&hq, candidate.get());
   ctx.mv.downwards_skip(cursor);
               MoveResult res = ctx.mv.downwards_move(cursor, false);
   if (res == move_fail_ssa || res == move_fail_rar) {
      add_to_hazard_query(&hq, candidate.get());
   ctx.mv.downwards_skip(cursor);
      } else if (res == move_fail_pressure) {
         }
         }
      unsigned
   schedule_VMEM_store(sched_ctx& ctx, Block* block, std::vector<RegisterDemand>& register_demand,
         {
      hazard_query hq;
            DownwardsCursor cursor = ctx.mv.downwards_init(idx, true, true);
            for (int i = 0; i < VMEM_CLAUSE_MAX_GRAB_DIST; i++) {
      aco_ptr<Instruction>& candidate = block->instructions[cursor.source_idx];
   if (candidate->opcode == aco_opcode::p_logical_start)
            if (!should_form_clause(current, candidate.get())) {
      add_to_hazard_query(&hq, candidate.get());
   ctx.mv.downwards_skip(cursor);
               if (perform_hazard_query(&hq, candidate.get(), false) != hazard_success ||
                                 }
      void
   schedule_block(sched_ctx& ctx, Program* program, Block* block, live& live_vars)
   {
      ctx.last_SMEM_dep_idx = 0;
   ctx.last_SMEM_stall = INT16_MIN;
   ctx.mv.block = block;
            /* go through all instructions and find memory loads */
   unsigned num_stores = 0;
   for (unsigned idx = 0; idx < block->instructions.size(); idx++) {
               if (current->opcode == aco_opcode::p_logical_end)
            if (block->kind & block_kind_export_end && current->isEXP() && ctx.schedule_pos_exports) {
      unsigned target = current->exp().dest;
   if (target >= V_008DFC_SQ_EXP_POS && target < V_008DFC_SQ_EXP_PRIM) {
      ctx.mv.current = current;
   schedule_position_export(ctx, block, live_vars.register_demand[block->index], current,
                  if (current->definitions.empty()) {
      num_stores += current->isVMEM() || current->isFlatLike() ? 1 : 0;
               if (current->isVMEM() || current->isFlatLike()) {
      ctx.mv.current = current;
               if (current->isSMEM()) {
      ctx.mv.current = current;
                  /* GFX11 benefits from creating VMEM store clauses. */
   if (num_stores > 1 && program->gfx_level >= GFX11) {
      for (int idx = block->instructions.size() - 1; idx >= 0; idx--) {
      Instruction* current = block->instructions[idx].get();
                  ctx.mv.current = current;
   idx -=
                  /* resummarize the block's register demand */
   block->register_demand = RegisterDemand();
   for (unsigned idx = 0; idx < block->instructions.size(); idx++) {
            }
      void
   schedule_program(Program* program, live& live_vars)
   {
      /* don't use program->max_reg_demand because that is affected by max_waves_per_simd */
   RegisterDemand demand;
   for (Block& block : program->blocks)
                  sched_ctx ctx;
   ctx.gfx_level = program->gfx_level;
   ctx.mv.depends_on.resize(program->peekAllocationId());
   ctx.mv.RAR_dependencies.resize(program->peekAllocationId());
   ctx.mv.RAR_dependencies_clause.resize(program->peekAllocationId());
   /* Allowing the scheduler to reduce the number of waves to as low as 5
   * improves performance of Thrones of Britannia significantly and doesn't
   * seem to hurt anything else. */
   // TODO: account for possible uneven num_waves on GFX10+
   unsigned wave_fac = program->dev.physical_vgprs / 256;
   if (program->num_waves <= 5 * wave_fac)
         else if (demand.vgpr >= 29)
         else if (demand.vgpr >= 25)
         else
         ctx.num_waves = std::max<uint16_t>(ctx.num_waves, program->min_waves);
   ctx.num_waves = std::min<uint16_t>(ctx.num_waves, program->num_waves);
            /* VMEM_MAX_MOVES and such assume pre-GFX10 wave count */
            assert(ctx.num_waves > 0);
   ctx.mv.max_registers = {int16_t(get_addr_vgpr_from_waves(program, ctx.num_waves * wave_fac) - 2),
            /* NGG culling shaders are very sensitive to position export scheduling.
   * Schedule less aggressively when early primitive export is used, and
   * keep the position export at the very bottom when late primitive export is used.
   */
   if (program->info.has_ngg_culling && program->stage.num_sw_stages() == 1) {
      if (!program->info.has_ngg_early_prim_export)
         else
               for (Block& block : program->blocks)
            /* update max_reg_demand and num_waves */
   RegisterDemand new_demand;
   for (Block& block : program->blocks) {
         }
         /* if enabled, this code asserts that register_demand is updated correctly */
   #if 0
      int prev_num_waves = program->num_waves;
            std::vector<RegisterDemand> demands(program->blocks.size());
   for (unsigned j = 0; j < program->blocks.size(); j++) {
                           for (unsigned j = 0; j < program->blocks.size(); j++) {
      Block &b = program->blocks[j];
   for (unsigned i = 0; i < b.instructions.size(); i++)
                     assert(program->max_reg_demand == prev_max_demand);
      #endif
   }
      } // namespace aco
