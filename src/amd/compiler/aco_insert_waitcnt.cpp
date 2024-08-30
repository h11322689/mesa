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
      #include "common/sid.h"
      #include <map>
   #include <stack>
   #include <vector>
      namespace aco {
      namespace {
      /**
   * The general idea of this pass is:
   * The CFG is traversed in reverse postorder (forward) and loops are processed
   * several times until no progress is made.
   * Per BB two wait_ctx is maintained: an in-context and out-context.
   * The in-context is the joined out-contexts of the predecessors.
   * The context contains a map: gpr -> wait_entry
   * consisting of the information about the cnt values to be waited for.
   * Note: After merge-nodes, it might occur that for the same register
   *       multiple cnt values are to be waited for.
   *
   * The values are updated according to the encountered instructions:
   * - additional events increment the counter of waits of the same type
   * - or erase gprs with counters higher than to be waited for.
   */
      // TODO: do a more clever insertion of wait_cnt (lgkm_cnt)
   // when there is a load followed by a use of a previous load
      /* Instructions of the same event will finish in-order except for smem
   * and maybe flat. Instructions of different events may not finish in-order. */
   enum wait_event : uint16_t {
      event_smem = 1 << 0,
   event_lds = 1 << 1,
   event_gds = 1 << 2,
   event_vmem = 1 << 3,
   event_vmem_store = 1 << 4, /* GFX10+ */
   event_flat = 1 << 5,
   event_exp_pos = 1 << 6,
   event_exp_param = 1 << 7,
   event_exp_mrt_null = 1 << 8,
   event_gds_gpr_lock = 1 << 9,
   event_vmem_gpr_lock = 1 << 10,
   event_sendmsg = 1 << 11,
   event_ldsdir = 1 << 12,
   event_valu = 1 << 13,
   event_trans = 1 << 14,
   event_salu = 1 << 15,
      };
      enum counter_type : uint8_t {
      counter_exp = 1 << 0,
   counter_lgkm = 1 << 1,
   counter_vm = 1 << 2,
   counter_vs = 1 << 3,
   counter_alu = 1 << 4,
      };
      enum vmem_type : uint8_t {
      vmem_nosampler = 1 << 0,
   vmem_sampler = 1 << 1,
      };
      static const uint16_t exp_events = event_exp_pos | event_exp_param | event_exp_mrt_null |
         static const uint16_t lgkm_events = event_smem | event_lds | event_gds | event_flat | event_sendmsg;
   static const uint16_t vm_events = event_vmem | event_flat;
   static const uint16_t vs_events = event_vmem_store;
      /* On GFX11+ the SIMD frontend doesn't switch to issuing instructions from a different
   * wave if there is an ALU stall. Hence we have an instruction (s_delay_alu) to signal
   * that we should switch to a different wave and contains info on dependencies as to
   * when we can switch back.
   *
   * This seems to apply only for ALU->ALU dependencies as other instructions have better
   * integration with the frontend.
   *
   * Note that if we do not emit s_delay_alu things will still be correct, but the wave
   * will stall in the ALU (and the ALU will be doing nothing else). We'll use this as
   * I'm pretty sure our cycle info is wrong at times (necessarily so, e.g. wave64 VALU
   * instructions can take a different number of cycles based on the exec mask)
   */
   struct alu_delay_info {
      /* These are the values directly above the max representable value, i.e. the wait
   * would turn into a no-op when we try to wait for something further back than
   * this.
   */
   static constexpr int8_t valu_nop = 5;
            /* How many VALU instructions ago this value was written */
   int8_t valu_instrs = valu_nop;
   /* Cycles until the writing VALU instruction is finished */
            /* How many Transcedent instructions ago this value was written */
   int8_t trans_instrs = trans_nop;
   /* Cycles until the writing Transcendent instruction is finished */
            /* Cycles until the writing SALU instruction is finished*/
            bool combine(const alu_delay_info& other)
   {
      bool changed = other.valu_instrs < valu_instrs || other.trans_instrs < trans_instrs ||
               valu_instrs = std::min(valu_instrs, other.valu_instrs);
   trans_instrs = std::min(trans_instrs, other.trans_instrs);
   salu_cycles = std::max(salu_cycles, other.salu_cycles);
   valu_cycles = std::max(valu_cycles, other.valu_cycles);
   trans_cycles = std::max(trans_cycles, other.trans_cycles);
               /* Needs to be called after any change to keep the data consistent. */
   void fixup()
   {
      if (valu_instrs >= valu_nop || valu_cycles <= 0) {
      valu_instrs = valu_nop;
               if (trans_instrs >= trans_nop || trans_cycles <= 0) {
      trans_instrs = trans_nop;
                           /* Returns true if a wait would be a no-op */
   bool empty() const
   {
                  UNUSED void print(FILE* output) const
   {
      if (valu_instrs != valu_nop)
         if (valu_cycles)
         if (trans_instrs != trans_nop)
         if (trans_cycles)
         if (salu_cycles)
         };
      uint8_t
   get_counters_for_event(wait_event ev)
   {
      switch (ev) {
   case event_smem:
   case event_lds:
   case event_gds:
   case event_sendmsg: return counter_lgkm;
   case event_vmem: return counter_vm;
   case event_vmem_store: return counter_vs;
   case event_flat: return counter_vm | counter_lgkm;
   case event_exp_pos:
   case event_exp_param:
   case event_exp_mrt_null:
   case event_gds_gpr_lock:
   case event_vmem_gpr_lock:
   case event_ldsdir: return counter_exp;
   case event_valu:
   case event_trans:
   case event_salu: return counter_alu;
   default: return 0;
      }
      struct wait_entry {
      wait_imm imm;
   alu_delay_info delay;
   uint16_t events;  /* use wait_event notion */
   uint8_t counters; /* use counter_type notion */
   bool wait_on_read : 1;
   bool logical : 1;
            wait_entry(wait_event event_, wait_imm imm_, alu_delay_info delay_, bool logical_,
            : imm(imm_), delay(delay_), events(event_), counters(get_counters_for_event(event_)),
               bool join(const wait_entry& other)
   {
      bool changed = (other.events & ~events) || (other.counters & ~counters) ||
               events |= other.events;
   counters |= other.counters;
   changed |= imm.combine(other.imm);
   changed |= delay.combine(other.delay);
   wait_on_read |= other.wait_on_read;
   vmem_types |= other.vmem_types;
   logical &= other.logical;
               void remove_counter(counter_type counter)
   {
               if (counter == counter_lgkm) {
      imm.lgkm = wait_imm::unset_counter;
               if (counter == counter_vm) {
      imm.vm = wait_imm::unset_counter;
   events &= ~event_vmem;
               if (counter == counter_exp) {
      imm.exp = wait_imm::unset_counter;
               if (counter == counter_vs) {
      imm.vs = wait_imm::unset_counter;
               if (!(counters & counter_lgkm) && !(counters & counter_vm))
            if (counter == counter_alu) {
      delay = alu_delay_info();
                  UNUSED void print(FILE* output) const
   {
      fprintf(output, "logical: %u\n", logical);
   imm.print(output);
   delay.print(output);
   if (events)
         if (counters)
         if (!wait_on_read)
         if (!logical)
         if (vmem_types)
         };
      struct wait_ctx {
      Program* program;
   enum amd_gfx_level gfx_level;
   uint16_t max_vm_cnt;
   uint16_t max_exp_cnt;
   uint16_t max_lgkm_cnt;
   uint16_t max_vs_cnt;
            bool vm_nonzero = false;
   bool exp_nonzero = false;
   bool lgkm_nonzero = false;
   bool vs_nonzero = false;
   bool pending_flat_lgkm = false;
   bool pending_flat_vm = false;
            wait_imm barrier_imm[storage_count];
                     wait_ctx() {}
   wait_ctx(Program* program_)
      : program(program_), gfx_level(program_->gfx_level),
      max_vm_cnt(program_->gfx_level >= GFX9 ? 62 : 14), max_exp_cnt(6),
   max_lgkm_cnt(program_->gfx_level >= GFX10 ? 62 : 14),
   max_vs_cnt(program_->gfx_level >= GFX10 ? 62 : 0),
            bool join(const wait_ctx* other, bool logical)
   {
      bool changed = other->exp_nonzero > exp_nonzero || other->vm_nonzero > vm_nonzero ||
                        exp_nonzero |= other->exp_nonzero;
   vm_nonzero |= other->vm_nonzero;
   lgkm_nonzero |= other->lgkm_nonzero;
   vs_nonzero |= other->vs_nonzero;
   pending_flat_lgkm |= other->pending_flat_lgkm;
   pending_flat_vm |= other->pending_flat_vm;
            for (const auto& entry : other->gpr_map) {
                     using iterator = std::map<PhysReg, wait_entry>::iterator;
   const std::pair<iterator, bool> insert_pair = gpr_map.insert(entry);
   if (insert_pair.second) {
         } else {
                     for (unsigned i = 0; i < storage_count; i++) {
      changed |= barrier_imm[i].combine(other->barrier_imm[i]);
   changed |= (other->barrier_events[i] & ~barrier_events[i]) != 0;
                           void wait_and_remove_from_entry(PhysReg reg, wait_entry& entry, counter_type counter)
   {
                  UNUSED void print(FILE* output) const
   {
      fprintf(output, "exp_nonzero: %u\n", exp_nonzero);
   fprintf(output, "vm_nonzero: %u\n", vm_nonzero);
   fprintf(output, "lgkm_nonzero: %u\n", lgkm_nonzero);
   fprintf(output, "vs_nonzero: %u\n", vs_nonzero);
   fprintf(output, "pending_flat_lgkm: %u\n", pending_flat_lgkm);
   fprintf(output, "pending_flat_vm: %u\n", pending_flat_vm);
   for (const auto& entry : gpr_map) {
      fprintf(output, "gpr_map[%c%u] = {\n", entry.first.reg() >= 256 ? 'v' : 's',
         entry.second.print(output);
               for (unsigned i = 0; i < storage_count; i++) {
      if (!barrier_imm[i].empty() || barrier_events[i]) {
      fprintf(output, "barriers[%u] = {\n", i);
   barrier_imm[i].print(output);
   fprintf(output, "events: %u\n", barrier_events[i]);
               };
      uint8_t
   get_vmem_type(Instruction* instr)
   {
      if (instr->opcode == aco_opcode::image_bvh64_intersect_ray)
         else if (instr->isMIMG() && !instr->operands[1].isUndefined() &&
               else if (instr->isVMEM() || instr->isScratch() || instr->isGlobal())
            }
      void
   check_instr(wait_ctx& ctx, wait_imm& wait, alu_delay_info& delay, Instruction* instr)
   {
      for (const Operand op : instr->operands) {
      if (op.isConstant() || op.isUndefined())
            /* check consecutively read gprs */
   for (unsigned j = 0; j < op.size(); j++) {
      PhysReg reg{op.physReg() + j};
   std::map<PhysReg, wait_entry>::iterator it = ctx.gpr_map.find(reg);
                  wait.combine(it->second.imm);
   if (instr->isVALU() || instr->isSALU())
                  for (const Definition& def : instr->definitions) {
      /* check consecutively written gprs */
   for (unsigned j = 0; j < def.getTemp().size(); j++) {
               std::map<PhysReg, wait_entry>::iterator it = ctx.gpr_map.find(reg);
                  /* Vector Memory reads and writes return in the order they were issued */
   uint8_t vmem_type = get_vmem_type(instr);
   if (vmem_type && ((it->second.events & vm_events) == event_vmem) &&
                  /* LDS reads and writes return in the order they were issued. same for GDS */
   if (instr->isDS() &&
                           }
      bool
   parse_wait_instr(wait_ctx& ctx, wait_imm& imm, Instruction* instr)
   {
      if (instr->opcode == aco_opcode::s_waitcnt_vscnt &&
      instr->definitions[0].physReg() == sgpr_null) {
   imm.vs = std::min<uint8_t>(imm.vs, instr->sopk().imm);
      } else if (instr->opcode == aco_opcode::s_waitcnt) {
      imm.combine(wait_imm(ctx.gfx_level, instr->sopp().imm));
      }
      }
      bool
   parse_delay_alu(wait_ctx& ctx, alu_delay_info& delay, Instruction* instr)
   {
      if (instr->opcode != aco_opcode::s_delay_alu)
            unsigned imm[2] = {instr->sopp().imm & 0xf, (instr->sopp().imm >> 7) & 0xf};
   for (unsigned i = 0; i < 2; ++i) {
      alu_delay_wait wait = (alu_delay_wait)imm[i];
   if (wait >= alu_delay_wait::VALU_DEP_1 && wait <= alu_delay_wait::VALU_DEP_4)
         else if (wait >= alu_delay_wait::TRANS32_DEP_1 && wait <= alu_delay_wait::TRANS32_DEP_3)
         else if (wait >= alu_delay_wait::SALU_CYCLE_1)
               delay.valu_cycles = instr->pass_flags & 0xffff;
               }
      void
   perform_barrier(wait_ctx& ctx, wait_imm& imm, memory_sync_info sync, unsigned semantics)
   {
      sync_scope subgroup_scope =
         if ((sync.semantics & semantics) && sync.scope > subgroup_scope) {
      unsigned storage = sync.storage;
   while (storage) {
                              uint16_t events = ctx.barrier_events[idx];
                  /* in non-WGP, the L1 (L0 on GFX10+) cache keeps all memory operations
   * in-order for the same workgroup */
                  if (events)
            }
      void
   force_waitcnt(wait_ctx& ctx, wait_imm& imm)
   {
      if (ctx.vm_nonzero)
         if (ctx.exp_nonzero)
         if (ctx.lgkm_nonzero)
            if (ctx.gfx_level >= GFX10) {
      if (ctx.vs_nonzero)
         }
      void
   update_alu(wait_ctx& ctx, bool is_valu, bool is_trans, bool clear, int cycles)
   {
      std::map<PhysReg, wait_entry>::iterator it = ctx.gpr_map.begin();
   while (it != ctx.gpr_map.end()) {
               if (clear) {
         } else {
      entry.delay.valu_instrs += is_valu ? 1 : 0;
   entry.delay.trans_instrs += is_trans ? 1 : 0;
   entry.delay.salu_cycles -= cycles;
                  entry.delay.fixup();
   if (it->second.delay.empty())
               if (!entry.counters)
         else
         }
      void
   kill(wait_imm& imm, alu_delay_info& delay, Instruction* instr, wait_ctx& ctx,
         {
      if (instr->opcode == aco_opcode::s_setpc_b64 || (debug_flags & DEBUG_FORCE_WAITCNT)) {
      /* Force emitting waitcnt states right after the instruction if there is
   * something to wait for. This is also applied for s_setpc_b64 to ensure
   * waitcnt states are inserted before jumping to the PS epilog.
   */
               /* Make sure POPS coherent memory accesses have reached the L2 cache before letting the
   * overlapping waves proceed into the ordered section.
   */
   if (ctx.program->has_pops_overlapped_waves_wait &&
      (ctx.gfx_level >= GFX11 ? instr->isEXP() && instr->exp().done
               if (ctx.vm_nonzero)
         if (ctx.gfx_level >= GFX10 && ctx.vs_nonzero)
         /* Await SMEM loads too, as it's possible for an application to create them, like using a
   * scalarization loop - pointless and unoptimal for an inherently divergent address of
   * per-pixel data, but still can be done at least synthetically and must be handled correctly.
   */
   if (ctx.program->has_smem_buffer_or_global_loads && ctx.lgkm_nonzero)
                        /* It's required to wait for scalar stores before "writing back" data.
   * It shouldn't cost anything anyways since we're about to do s_endpgm.
   */
   if (ctx.lgkm_nonzero && instr->opcode == aco_opcode::s_dcache_wb) {
      assert(ctx.gfx_level >= GFX8);
               if (ctx.gfx_level >= GFX10 && instr->isSMEM()) {
      /* GFX10: A store followed by a load at the same address causes a problem because
   * the load doesn't load the correct values unless we wait for the store first.
   * This is NOT mitigated by an s_nop.
   *
   * TODO: Refine this when we have proper alias analysis.
   */
   if (ctx.pending_s_buffer_store && !instr->smem().definitions.empty() &&
      !instr->smem().sync.can_reorder()) {
                  if (instr->opcode == aco_opcode::ds_ordered_count &&
      ((instr->ds().offset1 | (instr->ds().offset0 >> 8)) & 0x1)) {
               if (instr->opcode == aco_opcode::p_barrier)
         else
            if (!imm.empty() || !delay.empty()) {
      if (ctx.pending_flat_vm && imm.vm != wait_imm::unset_counter)
         if (ctx.pending_flat_lgkm && imm.lgkm != wait_imm::unset_counter)
            /* reset counters */
   ctx.exp_nonzero &= imm.exp != 0;
   ctx.vm_nonzero &= imm.vm != 0;
   ctx.lgkm_nonzero &= imm.lgkm != 0;
            /* update barrier wait imms */
   for (unsigned i = 0; i < storage_count; i++) {
      wait_imm& bar = ctx.barrier_imm[i];
   uint16_t& bar_ev = ctx.barrier_events[i];
   if (bar.exp != wait_imm::unset_counter && imm.exp <= bar.exp) {
      bar.exp = wait_imm::unset_counter;
      }
   if (bar.vm != wait_imm::unset_counter && imm.vm <= bar.vm) {
      bar.vm = wait_imm::unset_counter;
      }
   if (bar.lgkm != wait_imm::unset_counter && imm.lgkm <= bar.lgkm) {
      bar.lgkm = wait_imm::unset_counter;
      }
   if (bar.vs != wait_imm::unset_counter && imm.vs <= bar.vs) {
      bar.vs = wait_imm::unset_counter;
      }
   if (bar.vm == wait_imm::unset_counter && bar.lgkm == wait_imm::unset_counter)
               if (ctx.program->gfx_level >= GFX11) {
      update_alu(ctx, false, false, false,
               /* remove all gprs with higher counter from map */
   std::map<PhysReg, wait_entry>::iterator it = ctx.gpr_map.begin();
   while (it != ctx.gpr_map.end()) {
      if (imm.exp != wait_imm::unset_counter && imm.exp <= it->second.imm.exp)
         if (imm.vm != wait_imm::unset_counter && imm.vm <= it->second.imm.vm)
         if (imm.lgkm != wait_imm::unset_counter && imm.lgkm <= it->second.imm.lgkm)
         if (imm.vs != wait_imm::unset_counter && imm.vs <= it->second.imm.vs)
         if (delay.valu_instrs <= it->second.delay.valu_instrs)
         if (delay.trans_instrs <= it->second.delay.trans_instrs)
         it->second.delay.fixup();
   if (it->second.delay.empty())
         if (!it->second.counters)
         else
                  if (imm.vm == 0)
         if (imm.lgkm == 0) {
      ctx.pending_flat_lgkm = false;
         }
      void
   update_barrier_counter(uint8_t* ctr, unsigned max)
   {
      if (*ctr != wait_imm::unset_counter && *ctr < max)
      }
      void
   update_barrier_imm(wait_ctx& ctx, uint8_t counters, wait_event event, memory_sync_info sync)
   {
      for (unsigned i = 0; i < storage_count; i++) {
      wait_imm& bar = ctx.barrier_imm[i];
   uint16_t& bar_ev = ctx.barrier_events[i];
   if (sync.storage & (1 << i) && !(sync.semantics & semantic_private)) {
      bar_ev |= event;
   if (counters & counter_lgkm)
         if (counters & counter_vm)
         if (counters & counter_exp)
         if (counters & counter_vs)
      } else if (!(bar_ev & ctx.unordered_events) && !(ctx.unordered_events & event)) {
      if (counters & counter_lgkm && (bar_ev & lgkm_events) == event)
         if (counters & counter_vm && (bar_ev & vm_events) == event)
         if (counters & counter_exp && (bar_ev & exp_events) == event)
         if (counters & counter_vs && (bar_ev & vs_events) == event)
            }
      void
   update_counters(wait_ctx& ctx, wait_event event, memory_sync_info sync = memory_sync_info())
   {
               if (counters & counter_lgkm)
         if (counters & counter_vm)
         if (counters & counter_exp)
         if (counters & counter_vs)
                     if (ctx.unordered_events & event)
            if (ctx.pending_flat_lgkm)
         if (ctx.pending_flat_vm)
            for (std::pair<const PhysReg, wait_entry>& e : ctx.gpr_map) {
               if (entry.events & ctx.unordered_events)
                     if ((counters & counter_exp) && (entry.events & exp_events) == event &&
      entry.imm.exp < ctx.max_exp_cnt)
      if ((counters & counter_lgkm) && (entry.events & lgkm_events) == event &&
      entry.imm.lgkm < ctx.max_lgkm_cnt)
      if ((counters & counter_vm) && (entry.events & vm_events) == event &&
      entry.imm.vm < ctx.max_vm_cnt)
      if ((counters & counter_vs) && (entry.events & vs_events) == event &&
      entry.imm.vs < ctx.max_vs_cnt)
      }
      void
   update_counters_for_flat_load(wait_ctx& ctx, memory_sync_info sync = memory_sync_info())
   {
               ctx.lgkm_nonzero = true;
                     for (std::pair<PhysReg, wait_entry> e : ctx.gpr_map) {
      if (e.second.counters & counter_vm)
         if (e.second.counters & counter_lgkm)
      }
   ctx.pending_flat_lgkm = true;
      }
      void
   insert_wait_entry(wait_ctx& ctx, PhysReg reg, RegClass rc, wait_event event, bool wait_on_read,
         {
      uint16_t counters = get_counters_for_event(event);
   wait_imm imm;
   if (counters & counter_lgkm)
         if (counters & counter_vm)
         if (counters & counter_exp)
         if (counters & counter_vs)
            alu_delay_info delay;
   if (event == event_valu) {
      delay.valu_instrs = 0;
      } else if (event == event_trans) {
      delay.trans_instrs = 0;
      } else if (event == event_salu) {
                  wait_entry new_entry(event, imm, delay, !rc.is_linear() && !force_linear, wait_on_read);
            for (unsigned i = 0; i < rc.size(); i++) {
      auto it = ctx.gpr_map.emplace(PhysReg{reg.reg() + i}, new_entry);
   if (!it.second)
         }
      void
   insert_wait_entry(wait_ctx& ctx, Operand op, wait_event event, uint8_t vmem_types = 0)
   {
      if (!op.isConstant() && !op.isUndefined())
      }
      void
   insert_wait_entry(wait_ctx& ctx, Definition def, wait_event event, uint8_t vmem_types = 0,
         {
      /* We can't safely write to unwritten destination VGPR lanes with DS/VMEM on GFX11 without
   * waiting for the load to finish.
   * Also, follow linear control flow for ALU because it's unlikely that the hardware does per-lane
   * dependency checks.
   */
   uint32_t ds_vmem_events = event_lds | event_gds | event_vmem | event_flat;
   uint32_t alu_events = event_trans | event_valu | event_salu;
            insert_wait_entry(ctx, def.physReg(), def.regClass(), event, true, vmem_types, cycles,
      }
      void
   gen_alu(Instruction* instr, wait_ctx& ctx)
   {
      Instruction_cycle_info cycle_info = get_cycle_info(*ctx.program, *instr);
   bool is_valu = instr->isVALU();
   bool is_trans = instr->isTrans();
   bool clear = instr->isEXP() || instr->isDS() || instr->isMIMG() || instr->isFlatLike() ||
            wait_event event = (wait_event)0;
   if (is_trans)
         else if (is_valu)
         else if (instr->isSALU())
            if (event != (wait_event)0) {
      for (const Definition& def : instr->definitions)
      }
   update_alu(ctx, is_valu && instr_info.classes[(int)instr->opcode] != instr_class::wmma, is_trans,
      }
      void
   gen(Instruction* instr, wait_ctx& ctx)
   {
      switch (instr->format) {
   case Format::EXP: {
               wait_event ev;
   if (exp_instr.dest <= 9)
         else if (exp_instr.dest <= 15)
         else
                  /* insert new entries for exported vgprs */
   for (unsigned i = 0; i < 4; i++) {
      if (exp_instr.enabled_mask & (1 << i)) {
      unsigned idx = exp_instr.compressed ? i >> 1 : i;
   assert(idx < exp_instr.operands.size());
         }
   insert_wait_entry(ctx, exec, s2, ev, false);
      }
   case Format::FLAT: {
      FLAT_instruction& flat = instr->flat();
   if (ctx.gfx_level < GFX10 && !instr->definitions.empty())
         else
            if (!instr->definitions.empty())
            }
   case Format::SMEM: {
      SMEM_instruction& smem = instr->smem();
            if (!instr->definitions.empty())
         else if (ctx.gfx_level >= GFX10 && !smem.sync.can_reorder())
               }
   case Format::DS: {
      DS_instruction& ds = instr->ds();
   update_counters(ctx, ds.gds ? event_gds : event_lds, ds.sync);
   if (ds.gds)
            if (!instr->definitions.empty())
            if (ds.gds) {
      for (const Operand& op : instr->operands)
            }
      }
   case Format::LDSDIR: {
      LDSDIR_instruction& ldsdir = instr->ldsdir();
   update_counters(ctx, event_ldsdir, ldsdir.sync);
   insert_wait_entry(ctx, instr->definitions[0], event_ldsdir);
      }
   case Format::MUBUF:
   case Format::MTBUF:
   case Format::MIMG:
   case Format::GLOBAL:
   case Format::SCRATCH: {
      wait_event ev =
                  if (!instr->definitions.empty())
            if (ctx.gfx_level == GFX6 && instr->format != Format::MIMG && instr->operands.size() == 4) {
      update_counters(ctx, event_vmem_gpr_lock);
      } else if (ctx.gfx_level == GFX6 && instr->isMIMG() && !instr->operands[2].isUndefined()) {
      update_counters(ctx, event_vmem_gpr_lock);
                  }
   case Format::SOPP: {
      if (instr->opcode == aco_opcode::s_sendmsg || instr->opcode == aco_opcode::s_sendmsghalt)
            }
   case Format::SOP1: {
      if (instr->opcode == aco_opcode::s_sendmsg_rtn_b32 ||
      instr->opcode == aco_opcode::s_sendmsg_rtn_b64) {
   update_counters(ctx, event_sendmsg);
      }
      }
   default: break;
      }
      void
   emit_waitcnt(wait_ctx& ctx, std::vector<aco_ptr<Instruction>>& instructions, wait_imm& imm)
   {
      if (imm.vs != wait_imm::unset_counter) {
      assert(ctx.gfx_level >= GFX10);
   SOPK_instruction* waitcnt_vs =
         waitcnt_vs->definitions[0] = Definition(sgpr_null, s1);
   waitcnt_vs->imm = imm.vs;
   instructions.emplace_back(waitcnt_vs);
      }
   if (!imm.empty()) {
      SOPP_instruction* waitcnt =
         waitcnt->imm = imm.pack(ctx.gfx_level);
   waitcnt->block = -1;
      }
      }
      void
   emit_delay_alu(wait_ctx& ctx, std::vector<aco_ptr<Instruction>>& instructions,
         {
      uint32_t imm = 0;
   if (delay.trans_instrs != delay.trans_nop) {
                  if (delay.valu_instrs != delay.valu_nop) {
                  /* Note that we can only put 2 wait conditions in the instruction, so if we have all 3 we just
   * drop the SALU one. Here we use that this doesn't really affect correctness so occasionally
   * getting this wrong isn't an issue. */
   if (delay.salu_cycles && imm <= 0xf) {
      unsigned cycles = std::min<uint8_t>(3, delay.salu_cycles);
               SOPP_instruction* inst =
         inst->imm = imm;
   inst->block = -1;
   inst->pass_flags = (delay.valu_cycles | (delay.trans_cycles << 16));
   instructions.emplace_back(inst);
      }
      void
   handle_block(Program* program, Block& block, wait_ctx& ctx)
   {
               wait_imm queued_imm;
            for (aco_ptr<Instruction>& instr : block.instructions) {
      bool is_wait = parse_wait_instr(ctx, queued_imm, instr.get());
            memory_sync_info sync_info = get_sync_info(instr.get());
            if (program->gfx_level >= GFX11)
                  if (instr->format != Format::PSEUDO_BARRIER && !is_wait && !is_delay_alu) {
      if (instr->isVINTERP_INREG() && queued_imm.exp != wait_imm::unset_counter) {
      instr->vinterp_inreg().wait_exp = MIN2(instr->vinterp_inreg().wait_exp, queued_imm.exp);
               if (!queued_imm.empty())
                        bool is_ordered_count_acquire =
                                 if (is_ordered_count_acquire)
                  /* For last block of a program which has succeed shader part, wait all memory ops done
   * before go to next shader part.
   */
   if (block.kind & block_kind_end_with_regs)
            if (!queued_imm.empty())
         if (!queued_delay.empty())
               }
      } /* end namespace */
      void
   insert_wait_states(Program* program)
   {
      /* per BB ctx */
   std::vector<bool> done(program->blocks.size());
   std::vector<wait_ctx> in_ctx(program->blocks.size(), wait_ctx(program));
            std::stack<unsigned, std::vector<unsigned>> loop_header_indices;
            if (program->pending_lds_access) {
      update_barrier_imm(in_ctx[0], get_counters_for_event(event_lds), event_lds,
               for (Definition def : program->args_pending_vmem) {
      update_counters(in_ctx[0], event_vmem);
               for (unsigned i = 0; i < program->blocks.size();) {
               if (current.kind & block_kind_discard_early_exit) {
      /* Because the jump to the discard early exit block may happen anywhere in a block, it's
   * not possible to join it with its predecessors this way.
   * We emit all required waits when emitting the discard block.
   */
                        if (current.kind & block_kind_loop_header) {
         } else if (current.kind & block_kind_loop_exit) {
      bool repeat = false;
   if (loop_progress == loop_header_indices.size()) {
      i = loop_header_indices.top();
      }
   loop_header_indices.pop();
   loop_progress = std::min<unsigned>(loop_progress, loop_header_indices.size());
   if (repeat)
               bool changed = false;
   for (unsigned b : current.linear_preds)
         for (unsigned b : current.logical_preds)
            if (done[current.index] && !changed) {
      in_ctx[current.index] = std::move(ctx);
      } else {
                  loop_progress = std::max<unsigned>(loop_progress, current.loop_nest_depth);
                                 /* Combine s_delay_alu using the skip field. */
   if (program->gfx_level >= GFX11) {
      for (Block& block : program->blocks) {
      int i = 0;
   int prev_delay_alu = -1;
   for (aco_ptr<Instruction>& instr : block.instructions) {
      if (instr->opcode != aco_opcode::s_delay_alu) {
                        uint16_t imm = instr->sopp().imm;
   int skip = i - prev_delay_alu - 1;
   if (imm >> 7 || prev_delay_alu < 0 || skip >= 6) {
      if (imm >> 7 == 0)
                           block.instructions[prev_delay_alu]->sopp().imm |= (skip << 4) | (imm << 7);
      }
            }
      } // namespace aco
