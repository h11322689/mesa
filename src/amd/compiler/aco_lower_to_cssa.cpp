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
      #include <algorithm>
   #include <map>
   #include <unordered_map>
   #include <vector>
      /*
   * Implements an algorithm to lower to Conventional SSA Form (CSSA).
   * After "Revisiting Out-of-SSA Translation for Correctness, CodeQuality, and Efficiency"
   * by B. Boissinot, A. Darte, F. Rastello, B. Dupont de Dinechin, C. Guillon,
   *
   * By lowering the IR to CSSA, the insertion of parallelcopies is separated from
   * the register coalescing problem. Additionally, correctness is ensured w.r.t. spilling.
   * The algorithm coalesces non-interfering phi-resources while taking value-equality
   * into account. Re-indexes the SSA-defs.
   */
      namespace aco {
   namespace {
      typedef std::vector<Temp> merge_set;
      struct copy {
      Definition def;
      };
      struct merge_node {
      Operand value = Operand(); /* original value: can be an SSA-def or constant value */
   uint32_t index = -1u;      /* index into the vector of merge sets */
            /* we also remember two dominating defs with the same value: */
   Temp equal_anc_in = Temp();  /* within the same merge set */
      };
      struct cssa_ctx {
      Program* program;
   std::vector<IDSet>& live_out;                  /* live-out sets per block */
   std::vector<std::vector<copy>> parallelcopies; /* copies per block */
   std::vector<merge_set> merge_sets;             /* each vector is one (ordered) merge set */
      };
      /* create (virtual) parallelcopies for each phi instruction and
   * already merge copy-definitions with phi-defs into merge sets */
   void
   collect_parallelcopies(cssa_ctx& ctx)
   {
      ctx.parallelcopies.resize(ctx.program->blocks.size());
   Builder bld(ctx.program);
   for (Block& block : ctx.program->blocks) {
      for (aco_ptr<Instruction>& phi : block.instructions) {
                              /* if the definition is not temp, it is the exec mask.
   * We can reload the exec mask directly from the spill slot.
   */
                  std::vector<unsigned>& preds =
                        bool has_preheader_copy = false;
   for (unsigned i = 0; i < phi->operands.size(); i++) {
      Operand op = phi->operands[i];
                  if (def.regClass().type() == RegType::sgpr && !op.isTemp()) {
      /* SGPR inline constants and literals on GFX10+ can be spilled
   * and reloaded directly (without intermediate register) */
   if (op.isConstant()) {
      if (ctx.program->gfx_level >= GFX10)
         if (op.size() == 1 && !op.isLiteral())
      } else {
      assert(op.isFixed() && op.physReg() == exec);
                  /* create new temporary and rename operands */
   Temp tmp = bld.tmp(def.regClass());
                  /* place the new operands in the same merge set */
                  /* update the liveness information */
   if (op.isKill())
                                             /* place the definition in dominance-order */
   if (def.isTemp()) {
      if (has_preheader_copy)
         else if (block.kind & block_kind_loop_header)
         else
            }
            }
      /* check whether the definition of a comes after b. */
   inline bool
   defined_after(cssa_ctx& ctx, Temp a, Temp b)
   {
      merge_node& node_a = ctx.merge_node_table[a.id()];
   merge_node& node_b = ctx.merge_node_table[b.id()];
   if (node_a.defined_at == node_b.defined_at)
               }
      /* check whether a dominates b where b is defined after a */
   inline bool
   dominates(cssa_ctx& ctx, Temp a, Temp b)
   {
      assert(defined_after(ctx, b, a));
   merge_node& node_a = ctx.merge_node_table[a.id()];
   merge_node& node_b = ctx.merge_node_table[b.id()];
   unsigned idom = node_b.defined_at;
   while (idom > node_a.defined_at)
      idom = b.regClass().type() == RegType::vgpr ? ctx.program->blocks[idom].logical_idom
            }
      /* check intersection between var and parent:
   * We already know that parent dominates var. */
   inline bool
   intersects(cssa_ctx& ctx, Temp var, Temp parent)
   {
      merge_node& node_var = ctx.merge_node_table[var.id()];
   merge_node& node_parent = ctx.merge_node_table[parent.id()];
   assert(node_var.index != node_parent.index);
            /* if the parent is live-out at the definition block of var, they intersect */
   bool parent_live = ctx.live_out[block_idx].count(parent.id());
   if (parent_live)
            /* parent is defined in a different block than var */
   if (node_parent.defined_at < node_var.defined_at) {
      /* if the parent is not live-in, they don't interfere */
   std::vector<uint32_t>& preds = var.type() == RegType::vgpr
               for (uint32_t pred : preds) {
      if (!ctx.live_out[pred].count(parent.id()))
                  for (const copy& cp : ctx.parallelcopies[block_idx]) {
      /* if var is defined at the edge, they don't intersect */
   if (cp.def.getTemp() == var)
         if (cp.op.isTemp() && cp.op.getTemp() == parent)
      }
   /* if the parent is live at the edge, they intersect */
   if (parent_live)
            /* both, parent and var, are present in the same block */
   const Block& block = ctx.program->blocks[block_idx];
   for (auto it = block.instructions.crbegin(); it != block.instructions.crend(); ++it) {
      /* if the parent was not encountered yet, it can only be used by a phi */
   if (is_phi(it->get()))
            for (const Definition& def : (*it)->definitions) {
      if (!def.isTemp())
         /* if parent was not found yet, they don't intersect */
   if (def.getTemp() == var)
               for (const Operand& op : (*it)->operands) {
      if (!op.isTemp())
         /* if the var was defined before this point, they intersect */
   if (op.getTemp() == parent)
                     }
      /* check interference between var and parent:
   * i.e. they have different values and intersect.
   * If parent and var share the same value, also updates the equal ancestor. */
   inline bool
   interference(cssa_ctx& ctx, Temp var, Temp parent)
   {
      assert(var != parent);
   merge_node& node_var = ctx.merge_node_table[var.id()];
            if (node_var.index == ctx.merge_node_table[parent.id()].index) {
      /* check/update in other set */
               Temp tmp = parent;
   /* check if var intersects with parent or any equal-valued ancestor */
   while (tmp != Temp() && !intersects(ctx, var, tmp)) {
      merge_node& node_tmp = ctx.merge_node_table[tmp.id()];
               /* no intersection found */
   if (tmp == Temp())
            /* var and parent, same value, but in different sets */
   if (node_var.value == ctx.merge_node_table[parent.id()].value) {
      node_var.equal_anc_out = tmp;
               /* var and parent, different values and intersect */
      }
      /* tries to merge set_b into set_a of given temporary and
   * drops that temporary as it is being coalesced */
   bool
   try_merge_merge_set(cssa_ctx& ctx, Temp dst, merge_set& set_b)
   {
      auto def_node_it = ctx.merge_node_table.find(dst.id());
   uint32_t index = def_node_it->second.index;
   merge_set& set_a = ctx.merge_sets[index];
   std::vector<Temp> dom; /* stack of the traversal */
   merge_set union_set;   /* the new merged merge-set */
   uint32_t i_a = 0;
            while (i_a < set_a.size() || i_b < set_b.size()) {
      Temp current;
   if (i_a == set_a.size())
         else if (i_b == set_b.size())
         /* else pick the one defined first */
   else if (defined_after(ctx, set_a[i_a], set_b[i_b]))
         else
            while (!dom.empty() && !dominates(ctx, dom.back(), current))
            if (!dom.empty() && interference(ctx, current, dom.back()))
            dom.emplace_back(current); /* otherwise, keep checking */
   if (current != dst)
               /* update hashmap */
   for (Temp t : union_set) {
      merge_node& node = ctx.merge_node_table[t.id()];
   /* update the equal ancestors:
   * i.e. the 'closest' dominating def with the same value */
   Temp in = node.equal_anc_in;
   Temp out = node.equal_anc_out;
   if (in == Temp() || (out != Temp() && defined_after(ctx, out, in)))
         node.equal_anc_out = Temp();
   /* update merge-set index */
      }
   set_b = merge_set(); /* free the old set_b */
   ctx.merge_sets[index] = union_set;
               }
      /* returns true if the copy can safely be omitted */
   bool
   try_coalesce_copy(cssa_ctx& ctx, copy copy, uint32_t block_idx)
   {
      /* we can only coalesce temporaries */
   if (!copy.op.isTemp())
            /* try emplace a merge_node for the copy operand */
   merge_node& op_node = ctx.merge_node_table[copy.op.tempId()];
   if (op_node.defined_at == -1u) {
      /* find defining block of operand */
   uint32_t pred = block_idx;
   do {
      block_idx = pred;
   pred = copy.op.regClass().type() == RegType::vgpr ? ctx.program->blocks[pred].logical_idom
      } while (block_idx != pred && ctx.live_out[pred].count(copy.op.tempId()));
   op_node.defined_at = block_idx;
               /* we can only coalesce copies of the same register class */
   if (copy.op.regClass() != copy.def.regClass())
            /* check if this operand has not yet been coalesced */
   if (op_node.index == -1u) {
      merge_set op_set = merge_set{copy.op.getTemp()};
               /* check if this operand has been coalesced into the same set */
   assert(ctx.merge_node_table.count(copy.def.tempId()));
   if (op_node.index == ctx.merge_node_table[copy.def.tempId()].index)
            /* otherwise, try to coalesce both merge sets */
      }
      /* node in the location-transfer-graph */
   struct ltg_node {
      copy cp;
   uint32_t read_idx;
      };
      /* emit the copies in an order that does not
   * create interferences within a merge-set */
   void
   emit_copies_block(Builder& bld, std::map<uint32_t, ltg_node>& ltg, RegType type)
   {
      auto&& it = ltg.begin();
   while (it != ltg.end()) {
      const copy& cp = it->second.cp;
   /* wrong regclass or still needed as operand */
   if (cp.def.regClass().type() != type || it->second.num_uses > 0) {
      ++it;
               /* emit the copy */
            /* update the location transfer graph */
   if (it->second.read_idx != -1u) {
      auto&& other = ltg.find(it->second.read_idx);
   if (other != ltg.end())
      }
   ltg.erase(it);
               /* count the number of remaining circular dependencies */
   unsigned num = std::count_if(ltg.begin(), ltg.end(),
            /* if there are circular dependencies, we just emit them as single parallelcopy */
   if (num) {
      // TODO: this should be restricted to a feasible number of registers
   // and otherwise use a temporary to avoid having to reload more (spilled)
   // variables than we have registers.
   aco_ptr<Pseudo_instruction> copy{create_instruction<Pseudo_instruction>(
         it = ltg.begin();
   for (unsigned i = 0; i < num; i++) {
                     copy->definitions[i] = it->second.cp.def;
   copy->operands[i] = it->second.cp.op;
      }
         }
      /* either emits or coalesces all parallelcopies and
   * renames the phi-operands accordingly. */
   void
   emit_parallelcopies(cssa_ctx& ctx)
   {
               /* we iterate backwards to prioritize coalescing in else-blocks */
   for (int i = ctx.program->blocks.size() - 1; i >= 0; i--) {
      if (ctx.parallelcopies[i].empty())
            std::map<uint32_t, ltg_node> ltg;
   bool has_vgpr_copy = false;
            /* first, try to coalesce all parallelcopies */
   for (const copy& cp : ctx.parallelcopies[i]) {
      if (try_coalesce_copy(ctx, cp, i)) {
      renames.emplace(cp.def.tempId(), cp.op);
   /* update liveness info */
   ctx.live_out[i].erase(cp.def.tempId());
      } else {
      uint32_t read_idx = -1u;
   if (cp.op.isTemp())
         uint32_t write_idx = ctx.merge_node_table[cp.def.tempId()].index;
                  bool is_vgpr = cp.def.regClass().type() == RegType::vgpr;
   has_vgpr_copy |= is_vgpr;
                  /* build location-transfer-graph */
   for (auto& pair : ltg) {
      if (pair.second.read_idx == -1u)
         auto&& it = ltg.find(pair.second.read_idx);
   if (it != ltg.end())
               /* emit parallelcopies ordered */
   Builder bld(ctx.program);
            if (has_vgpr_copy) {
      /* emit VGPR copies */
   auto IsLogicalEnd = [](const aco_ptr<Instruction>& inst) -> bool
   { return inst->opcode == aco_opcode::p_logical_end; };
   auto it =
         bld.reset(&block.instructions, std::prev(it.base()));
               if (has_sgpr_copy) {
      /* emit SGPR copies */
   aco_ptr<Instruction> branch = std::move(block.instructions.back());
   block.instructions.pop_back();
   bld.reset(&block.instructions);
   emit_copies_block(bld, ltg, RegType::sgpr);
                  /* finally, rename coalesced phi operands */
   for (Block& block : ctx.program->blocks) {
      for (aco_ptr<Instruction>& phi : block.instructions) {
                     for (Operand& op : phi->operands) {
      if (!op.isTemp())
         auto&& it = renames.find(op.tempId());
   if (it != renames.end()) {
      op = it->second;
               }
      }
      } /* end namespace */
      void
   lower_to_cssa(Program* program, live& live_vars)
   {
      reindex_ssa(program, live_vars.live_out);
   cssa_ctx ctx = {program, live_vars.live_out};
   collect_parallelcopies(ctx);
            /* update live variable information */
      }
   } // namespace aco
