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
      #include "sfn_liverangeevaluator_helpers.h"
      #include "sfn_virtualvalues.h"
   #include "util/u_math.h"
      #include <cassert>
   #include <iostream>
   #include <limits>
      namespace r600 {
      ProgramScope::ProgramScope(
      ProgramScope *parent, ProgramScopeType type, int id, int depth, int scope_begin):
   scope_type(type),
   scope_id(id),
   scope_nesting_depth(depth),
   scope_begin(scope_begin),
   scope_end(-1),
   break_loop_line(std::numeric_limits<int>::max()),
      {
   }
      ProgramScope::ProgramScope():
         {
   }
      ProgramScopeType
   ProgramScope::type() const
   {
         }
      ProgramScope *
   ProgramScope::parent() const
   {
         }
      int
   ProgramScope::nesting_depth() const
   {
         }
      bool
   ProgramScope::is_loop() const
   {
         }
      bool
   ProgramScope::is_in_loop() const
   {
      if (scope_type == loop_body)
            if (parent_scope)
               }
      const ProgramScope *
   ProgramScope::innermost_loop() const
   {
      if (scope_type == loop_body)
            if (parent_scope)
               }
      const ProgramScope *
   ProgramScope::outermost_loop() const
   {
      const ProgramScope *loop = nullptr;
            do {
      if (p->type() == loop_body)
                        }
      bool
   ProgramScope::is_child_of_ifelse_id_sibling(const ProgramScope *scope) const
   {
      const ProgramScope *my_parent = in_parent_ifelse_scope();
   while (my_parent) {
      /* is a direct child? */
   if (my_parent == scope)
         /* is a child of the conditions sibling? */
   if (my_parent->id() == scope->id())
            }
      }
      bool
   ProgramScope::is_child_of(const ProgramScope *scope) const
   {
      const ProgramScope *my_parent = parent();
   while (my_parent) {
      if (my_parent == scope)
            }
      }
      const ProgramScope *
   ProgramScope::enclosing_conditional() const
   {
      if (is_conditional())
            if (parent_scope)
               }
      bool
   ProgramScope::contains_range_of(const ProgramScope& other) const
   {
         }
      bool
   ProgramScope::is_conditional() const
   {
      return scope_type == if_branch || scope_type == else_branch ||
      }
      const ProgramScope *
   ProgramScope::in_else_scope() const
   {
      if (scope_type == else_branch)
            if (parent_scope)
               }
      const ProgramScope *
   ProgramScope::in_parent_ifelse_scope() const
   {
      if (parent_scope)
         else
      }
      const ProgramScope *
   ProgramScope::in_ifelse_scope() const
   {
      if (scope_type == if_branch || scope_type == else_branch)
            if (parent_scope)
               }
      bool
   ProgramScope::is_switchcase_scope_in_loop() const
   {
      return (scope_type == switch_case_branch || scope_type == switch_default_branch) &&
      }
      bool
   ProgramScope::break_is_for_switchcase() const
   {
      if (scope_type == loop_body)
            if (scope_type == switch_case_branch || scope_type == switch_default_branch ||
      scope_type == switch_body)
         if (parent_scope)
               }
      int
   ProgramScope::id() const
   {
         }
      int
   ProgramScope::begin() const
   {
         }
      int
   ProgramScope::end() const
   {
         }
      void
   ProgramScope::set_end(int end)
   {
      if (scope_end == -1)
      }
      void
   ProgramScope::set_loop_break_line(int line)
   {
      if (scope_type == loop_body) {
         } else {
      if (parent_scope)
         }
      int
   ProgramScope::loop_break_line() const
   {
         }
      RegisterCompAccess::RegisterCompAccess(LiveRange range):
      last_read_scope(nullptr),
   first_read_scope(nullptr),
   first_write_scope(nullptr),
   first_write(range.start),
   last_read(range.end),
   last_write(range.start),
   first_read(std::numeric_limits<int>::max()),
   conditionality_in_loop_id(conditionality_untouched),
   if_scope_write_flags(0),
   next_ifelse_nesting_depth(0),
   current_unpaired_if_write_scope(nullptr),
   was_written_in_current_else_scope(false),
      {
   }
      RegisterCompAccess::RegisterCompAccess():
         {
   }
      void
   RegisterCompAccess::record_read(int block, int line, ProgramScope *scope, LiveRangeEntry::EUse use)
   {
               if (alu_block_id == block_id_uninitalized) {
         } else if (alu_block_id != block) {
                  if (use != LiveRangeEntry::use_unspecified)
         if (last_read < line)
            if (first_read > line) {
      first_read = line;
               /* If the conditionality of the first write is already resolved then
   * no further checks are required.
   */
   if (conditionality_in_loop_id == write_is_unconditional ||
      conditionality_in_loop_id == write_is_conditional)
         /* Check whether we are in a condition within a loop */
   const ProgramScope *ifelse_scope = scope->in_ifelse_scope();
   const ProgramScope *enclosing_loop;
               /* If we have either not yet written to this register nor writes are
   * resolved as unconditional in the enclosing loop then check whether
   * we read before write in an IF/ELSE branch.
   */
   if ((conditionality_in_loop_id != write_is_conditional) &&
                           /* Has been written in this or a parent scope? - this makes the
   * temporary unconditionally set at this point.
   */
                  /* Has been written in the same scope before it was read? */
   if (ifelse_scope->type() == if_branch) {
      if (current_unpaired_if_write_scope->id() == scope->id())
      } else {
      if (was_written_in_current_else_scope)
                  /* The temporary was read (conditionally) before it is written, hence
   * it should survive a loop. This can be signaled like if it were
   * conditionally written.
   */
            }
      void
   RegisterCompAccess::record_write(int block, int line, ProgramScope *scope)
   {
      last_write = line;
   if (alu_block_id == block_id_uninitalized) {
         } else if (alu_block_id != block) {
                  if (first_write < 0) {
      first_write = line;
            /* If the first write we encounter is not in a conditional branch, or
   * the conditional write is not within a loop, then this is to be
   * considered an unconditional dominant write.
   */
   const ProgramScope *conditional = scope->enclosing_conditional();
   if (!conditional || !conditional->innermost_loop()) {
                     /* The conditionality of the first write is already resolved. */
   if (conditionality_in_loop_id == write_is_unconditional ||
      conditionality_in_loop_id == write_is_conditional)
         /* If the nesting depth is larger than the supported level,
   * then we assume conditional writes.
   */
   if (next_ifelse_nesting_depth >= supported_ifelse_nesting_depth) {
      conditionality_in_loop_id = write_is_conditional;
               /* If we are in an IF/ELSE scope within a loop and the loop has not
   * been resolved already, then record this write.
   */
   const ProgramScope *ifelse_scope = scope->in_ifelse_scope();
   if (ifelse_scope && ifelse_scope->innermost_loop() &&
      ifelse_scope->innermost_loop()->id() != conditionality_in_loop_id)
   }
      void
   RegisterCompAccess::record_ifelse_write(const ProgramScope& scope)
   {
      if (scope.type() == if_branch) {
      /* The first write in an IF branch within a loop implies unresolved
   * conditionality (if it was untouched or unconditional before).
   */
   conditionality_in_loop_id = conditionality_unresolved;
   was_written_in_current_else_scope = false;
      } else {
      was_written_in_current_else_scope = true;
         }
      void
   RegisterCompAccess::record_if_write(const ProgramScope& scope)
   {
      /* Don't record write if this IF scope if it ...
   * - is not the first write in this IF scope,
   * - has already been written in a parent IF scope.
   * In both cases this write is a secondary write that doesn't contribute
   * to resolve conditionality.
   *
   * Record the write if it
   * - is the first one (obviously),
   * - happens in an IF branch that is a child of the ELSE branch of the
   *   last active IF/ELSE pair. In this case recording this write is used to
   *   established whether the write is (un-)conditional in the scope
   * enclosing this outer IF/ELSE pair.
   */
   if (!current_unpaired_if_write_scope ||
      (current_unpaired_if_write_scope->id() != scope.id() &&
   scope.is_child_of_ifelse_id_sibling(current_unpaired_if_write_scope))) {
   if_scope_write_flags |= 1 << next_ifelse_nesting_depth;
   current_unpaired_if_write_scope = &scope;
         }
      void
   RegisterCompAccess::record_else_write(const ProgramScope& scope)
   {
               /* If the temporary was written in an IF branch on the same scope level
   * and this branch is the sibling of this ELSE branch, then we have a
   * pair of writes that makes write access to this temporary unconditional
   * in the enclosing scope.
            if ((if_scope_write_flags & mask) &&
      (scope.id() == current_unpaired_if_write_scope->id())) {
   --next_ifelse_nesting_depth;
            /* The following code deals with propagating unconditionality from
   * inner levels of nested IF/ELSE to the outer levels like in
   *
   * 1: var t;
   * 2: if (a) {        <- start scope A
   * 3:    if (b)
   * 4:         t = ...
   * 5:    else
   * 6:         t = ...
   * 7: } else {        <- start scope B
   * 8:    if (c)
   * 9:         t = ...
   * A:    else         <- start scope C
   * B:         t = ...
   * C: }
   *
                     if (1 << (next_ifelse_nesting_depth - 1) & if_scope_write_flags) {
      /* We are at the end of scope C and already recorded a write
   * within an IF scope (A), the sibling of the parent ELSE scope B,
   * and it is not yet resolved. Mark that as the last relevant
   * IF scope. Below the write will be resolved for the A/B
   * scope pair.
   */
      } else {
         }
   /* Promote the first write scope to the enclosing scope because
   * the current IF/ELSE pair is now irrelevant for the analysis.
   * This is also required to evaluate the minimum life time for t in
   * {
   *    var t;
   *    if (a)
   *      t = ...
   *    else
   *      t = ...
   *    x = t;
   *    ...
   * }
   */
            /* If some parent is IF/ELSE and in a loop then propagate the
   * write to that scope. Otherwise the write is unconditional
   * because it happens in both corresponding IF/ELSE branches
   * in this loop, and hence, record the loop id to signal the
   * resolution.
   */
   if (parent_ifelse && parent_ifelse->is_in_loop()) {
         } else {
            } else {
      /* The temporary was not written in the IF branch corresponding
   * to this ELSE branch, hence the write is conditional.
   */
         }
      bool
   RegisterCompAccess::conditional_ifelse_write_in_loop() const
   {
         }
      void
   RegisterCompAccess::propagate_live_range_to_dominant_write_scope()
   {
      first_write = first_write_scope->begin();
            if (last_read < lr)
      }
      void
   RegisterCompAccess::update_required_live_range()
   {
               /* This register component is not used at all, or only read,
   * mark it as unused and ignore it when renaming.
   * glsl_to_tgsi_visitor::renumber_registers will take care of
   * eliminating registers that are not written to.
   */
   if (last_write < 0) {
      m_range.start = -1;
   m_range.end = -1;
               /* Only written to, just make sure the register component is not
   * reused in the range it is used to write to
   */
   if (!last_read_scope) {
      m_range.start = first_write;
   m_range.end = last_write + 1;
                        /* The register was pre-defines, so th first write scope is the outerpost
   * scopw */
   if (!first_write_scope) {
      first_write_scope = first_read_scope;
   while (first_write_scope->parent())
               const ProgramScope *enclosing_scope_first_read = first_read_scope;
            /* We read before writing in a loop
   * hence the value must survive the loops
   */
   if ((first_read <= first_write) && first_read_scope->is_in_loop()) {
      keep_for_full_loop = true;
               /* A conditional write within a (nested) loop must survive the outermost
   * loop if the last read was not within the same scope.
   */
   const ProgramScope *conditional = enclosing_scope_first_write->enclosing_conditional();
   if (conditional && !conditional->contains_range_of(*last_read_scope) &&
      (conditional->is_switchcase_scope_in_loop() ||
   conditional_ifelse_write_in_loop())) {
   keep_for_full_loop = true;
               /* Evaluate the scope that is shared by all: required first write scope,
   * required first read before write scope, and last read scope.
   */
   const ProgramScope *enclosing_scope = enclosing_scope_first_read;
   if (enclosing_scope_first_write->contains_range_of(*enclosing_scope))
            if (last_read_scope->contains_range_of(*enclosing_scope))
            while (!enclosing_scope->contains_range_of(*enclosing_scope_first_write) ||
            enclosing_scope = enclosing_scope->parent();
               /* Propagate the last read scope to the target scope */
   while (enclosing_scope->nesting_depth() < last_read_scope->nesting_depth()) {
      /* If the read is in a loop and we have to move up the scope we need to
   * extend the live range to the end of this current loop because at this
   * point we don't know whether the component was written before
   * un-conditionally in the same loop.
   */
   if (last_read_scope->is_loop())
                        /* If the variable has to be kept for the whole loop, and we
   * are currently in a loop, then propagate the live range.
   */
   if (keep_for_full_loop && first_write_scope->is_loop())
            /* Propagate the first_dominant_write scope to the target scope */
   while (enclosing_scope->nesting_depth() < first_write_scope->nesting_depth()) {
      /* Propagate live_range if there was a break in a loop and the write was
   * after the break inside that loop. Note, that this is only needed if
   * we move up in the scopes.
   */
   if (first_write_scope->loop_break_line() < first_write) {
      keep_for_full_loop = true;
                        /* Propagate live_range if we are now in a loop */
   if (keep_for_full_loop && first_write_scope->is_loop())
               /* The last write past the last read is dead code, but we have to
   * ensure that the component is not reused too early, hence extend the
   * live_range past the last write.
   */
   if (last_write >= last_read)
            /* Here we are at the same scope, all is resolved */
   m_range.start = first_write;
      }
      const int RegisterCompAccess::conditionality_untouched = std::numeric_limits<int>::max();
      const int RegisterCompAccess::write_is_unconditional =
            RegisterAccess::RegisterAccess(const std::array<size_t, 4>& sizes)
   {
      for (int i = 0; i < 4; ++i)
      }
      RegisterCompAccess&
   RegisterAccess::operator()(const Register& reg)
   {
      assert(reg.chan() < 4);
   assert(m_access_record[reg.chan()].size() > (size_t)reg.index());
      }
      } // namespace r600
