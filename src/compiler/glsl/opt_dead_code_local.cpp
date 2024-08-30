   /*
   * Copyright © 2010 Intel Corporation
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
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   * DEALINGS IN THE SOFTWARE.
   */
      /**
   * \file opt_dead_code_local.cpp
   *
   * Eliminates local dead assignments from the code.
   *
   * This operates on basic blocks, tracking assignments and finding if
   * they're used before the variable is completely reassigned.
   *
   * Compare this to ir_dead_code.cpp, which operates globally looking
   * for assignments to variables that are never read.
   */
      #include "ir.h"
   #include "ir_basic_block.h"
   #include "ir_optimization.h"
   #include "compiler/glsl_types.h"
      static bool debug = false;
      namespace {
      class assignment_entry : public exec_node
   {
   public:
      /* override operator new from exec_node */
            assignment_entry(ir_variable *lhs, ir_assignment *ir)
   {
      assert(lhs);
   assert(ir);
   this->lhs = lhs;
   this->ir = ir;
               ir_variable *lhs;
            /* bitmask of xyzw channels written that haven't been used so far. */
      };
      class kill_for_derefs_visitor : public ir_hierarchical_visitor {
   public:
               kill_for_derefs_visitor(exec_list *assignments)
   {
                  void use_channels(ir_variable *const var, int used)
   {
      if (entry->lhs == var) {
      if (var->type->is_scalar() || var->type->is_vector()) {
         printf("used %s (0x%01x - 0x%01x)\n", entry->lhs->name,
   entry->unused, used & 0xf);
      entry->unused &= ~used;
      entry->remove();
   } else {
         printf("used %s\n", entry->lhs->name);
            }
                     virtual ir_visitor_status visit(ir_dereference_variable *ir)
   {
                           virtual ir_visitor_status visit(ir_swizzle *ir)
   {
      ir_dereference_variable *deref = ir->val->as_dereference_variable();
   return visit_continue;
            int used = 0;
   used |= 1 << ir->mask.x;
   if (ir->mask.num_components > 1)
         if (ir->mask.num_components > 2)
         if (ir->mask.num_components > 3)
                                 virtual ir_visitor_status visit_leave(ir_emit_vertex *)
   {
      /* For the purpose of dead code elimination, emitting a vertex counts as
   * "reading" all of the currently assigned output variables.
   */
   foreach_in_list_safe(assignment_entry, entry, this->assignments) {
      if (entry->lhs->data.mode == ir_var_shader_out) {
      if (debug)
                                 private:
         };
      class array_index_visit : public ir_hierarchical_visitor {
   public:
      array_index_visit(ir_hierarchical_visitor *v)
   {
                  virtual ir_visitor_status visit_enter(class ir_dereference_array *ir)
   {
      ir->array_index->accept(visitor);
               static void run(ir_instruction *ir, ir_hierarchical_visitor *v)
   {
      array_index_visit top_visit(v);
                  };
      } /* unnamed namespace */
      /**
   * Adds an entry to the available copy list if it's a plain assignment
   * of a variable to a variable.
   */
   static bool
   process_assignment(linear_ctx *lin_ctx, ir_assignment *ir, exec_list *assignments)
   {
      ir_variable *var = NULL;
   bool progress = false;
            /* If this is an assignment of the form "foo = foo;", remove the whole
   * instruction and be done with it.
   */
   const ir_variable *const lhs_var = ir->whole_variable_written();
   if (lhs_var != NULL && lhs_var == ir->rhs->whole_variable_referenced()) {
      ir->remove();
               /* Kill assignment entries for things used to produce this assignment. */
            /* Kill assignment enties used as array indices.
   */
   array_index_visit::run(ir->lhs, &v);
   var = ir->lhs->variable_referenced();
            /* Now, check if we did a whole-variable assignment. */
            /* If it's a vector type, we can do per-channel elimination of
   * use of the RHS.
   */
   if (deref_var && (deref_var->var->type->is_scalar() ||
               if (debug)
                  foreach_in_list_safe(assignment_entry, entry, assignments) {
                     /* Skip if the assignment we're trying to eliminate isn't a plain
   * variable deref. */
                  int remove = entry->unused & ir->write_mask;
   if (debug) {
      printf("%s 0x%01x - 0x%01x = 0x%01x\n",
         var->name,
      }
                     if (debug) {
      printf("rewriting:\n  ");
                     entry->ir->write_mask &= ~remove;
   entry->unused &= ~remove;
   if (entry->ir->write_mask == 0) {
      /* Delete the dead assignment. */
   entry->ir->remove();
      } else {
      void *mem_ctx = ralloc_parent(entry->ir);
   /* Reswizzle the RHS arguments according to the new
   * write_mask.
   */
                        for (int i = 0; i < 4; i++) {
      if ((entry->ir->write_mask | remove) & (1 << i)) {
      if (!(remove & (1 << i)))
                        entry->ir->rhs = new(mem_ctx) ir_swizzle(entry->ir->rhs,
               if (debug) {
      printf("to:\n  ");
   entry->ir->print();
                  } else if (ir->whole_variable_written() != NULL) {
      /* We did a whole-variable assignment.  So, any instruction in
   * the assignment list with the same LHS is dead.
   */
   if (debug)
         foreach_in_list_safe(assignment_entry, entry, assignments) {
      if (entry->lhs == var) {
      if (debug)
         entry->ir->remove();
   entry->remove();
                     /* Add this instruction to the assignment list available to be removed. */
   assignment_entry *entry = new(lin_ctx) assignment_entry(var, ir);
            if (debug) {
               printf("current entries\n");
   printf("    %s (0x%01x)\n", entry->lhs->name, entry->unused);
                        }
      static void
   dead_code_local_basic_block(ir_instruction *first,
         ir_instruction *last,
   {
      ir_instruction *ir, *ir_next;
   /* List of avaialble_copy */
   exec_list assignments;
   bool *out_progress = (bool *)data;
            void *ctx = ralloc_context(NULL);
            /* Safe looping, since process_assignment */
      ir = ir_next, ir_next = (ir_instruction *)ir->next) {
                  ir->print();
   printf("\n");
                  progress = process_assignment(lin_ctx, ir_assign, &assignments) ||
               kill_for_derefs_visitor kill(&assignments);
   ir->accept(&kill);
                  break;
      }
   *out_progress = progress;
      }
      /**
   * Does a copy propagation pass on the code present in the instruction stream.
   */
   bool
   do_dead_code_local(exec_list *instructions)
   {
                           }
