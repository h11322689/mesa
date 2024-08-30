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
   * \file lower_mat_op_to_vec.cpp
   *
   * Breaks matrix operation expressions down to a series of vector operations.
   *
   * Generally this is how we have to codegen matrix operations for a
   * GPU, so this gives us the chance to constant fold operations on a
   * column or row.
   */
      #include "ir.h"
   #include "ir_expression_flattening.h"
   #include "compiler/glsl_types.h"
      namespace {
      class ir_mat_op_to_vec_visitor : public ir_hierarchical_visitor {
   public:
      ir_mat_op_to_vec_visitor()
   {
      this->made_progress = false;
                        ir_dereference *get_column(ir_dereference *val, int col);
            void do_mul_mat_mat(ir_dereference *result,
         void do_mul_mat_vec(ir_dereference *result,
         void do_mul_vec_mat(ir_dereference *result,
         void do_mul_mat_scalar(ir_dereference *result,
         void do_equal_mat_mat(ir_dereference *result, ir_dereference *a,
            void *mem_ctx;
      };
      } /* anonymous namespace */
      static bool
   mat_op_to_vec_predicate(ir_instruction *ir)
   {
      ir_expression *expr = ir->as_expression();
            if (!expr)
            for (i = 0; i < expr->num_operands; i++) {
      if (expr->operands[i]->type->is_matrix())
                  }
      bool
   do_mat_op_to_vec(exec_list *instructions)
   {
               /* Pull out any matrix expression to a separate assignment to a
   * temp.  This will make our handling of the breakdown to
   * operations on the matrix's vector components much easier.
   */
                        }
      ir_rvalue *
   ir_mat_op_to_vec_visitor::get_element(ir_dereference *val, int col, int row)
   {
                  }
      ir_dereference *
   ir_mat_op_to_vec_visitor::get_column(ir_dereference *val, int row)
   {
               if (val->type->is_matrix()) {
      val = new(mem_ctx) ir_dereference_array(val,
                  }
      void
   ir_mat_op_to_vec_visitor::do_mul_mat_mat(ir_dereference *result,
               {
      unsigned b_col, i;
   ir_assignment *assign;
            for (b_col = 0; b_col < b->type->matrix_columns; b_col++) {
      /* first column */
   expr = new(mem_ctx) ir_expression(ir_binop_mul,
                  /* following columns */
   for (i = 1; i < a->type->matrix_columns; i++) {
               mul_expr = new(mem_ctx) ir_expression(ir_binop_mul,
               expr = new(mem_ctx) ir_expression(ir_binop_add,
                     assign = new(mem_ctx) ir_assignment(get_column(result, b_col), expr);
         }
      void
   ir_mat_op_to_vec_visitor::do_mul_mat_vec(ir_dereference *result,
               {
      unsigned i;
   ir_assignment *assign;
            /* first column */
   expr = new(mem_ctx) ir_expression(ir_binop_mul,
                  /* following columns */
   for (i = 1; i < a->type->matrix_columns; i++) {
               mul_expr = new(mem_ctx) ir_expression(ir_binop_mul,
                           result = result->clone(mem_ctx, NULL);
   assign = new(mem_ctx) ir_assignment(result, expr);
      }
      void
   ir_mat_op_to_vec_visitor::do_mul_vec_mat(ir_dereference *result,
               {
               for (i = 0; i < b->type->matrix_columns; i++) {
      ir_rvalue *column_result;
   ir_expression *column_expr;
            column_result = result->clone(mem_ctx, NULL);
            column_expr = new(mem_ctx) ir_expression(ir_binop_dot,
                  column_assign = new(mem_ctx) ir_assignment(column_result,
               }
      void
   ir_mat_op_to_vec_visitor::do_mul_mat_scalar(ir_dereference *result,
               {
               for (i = 0; i < a->type->matrix_columns; i++) {
      ir_expression *column_expr;
            column_expr = new(mem_ctx) ir_expression(ir_binop_mul,
                  column_assign = new(mem_ctx) ir_assignment(get_column(result, i),
               }
      void
   ir_mat_op_to_vec_visitor::do_equal_mat_mat(ir_dereference *result,
                     {
      /* This essentially implements the following GLSL:
   *
   * bool equal(mat4 a, mat4 b)
   * {
   *   return !any(bvec4(a[0] != b[0],
   *                     a[1] != b[1],
   *                     a[2] != b[2],
   *                     a[3] != b[3]);
   * }
   *
   * bool nequal(mat4 a, mat4 b)
   * {
   *   return any(bvec4(a[0] != b[0],
   *                    a[1] != b[1],
   *                    a[2] != b[2],
   *                    a[3] != b[3]);
   * }
   */
   const unsigned columns = a->type->matrix_columns;
   const glsl_type *const bvec_type =
            ir_variable *const tmp_bvec =
      new(this->mem_ctx) ir_variable(bvec_type, "mat_cmp_bvec",
               for (unsigned i = 0; i < columns; i++) {
      ir_expression *const cmp =
      new(this->mem_ctx) ir_expression(ir_binop_any_nequal,
               ir_dereference *const lhs =
            ir_assignment *const assign =
                        ir_rvalue *const val = new(this->mem_ctx) ir_dereference_variable(tmp_bvec);
   uint8_t vec_elems = val->type->vector_elements;
   ir_expression *any =
      new(this->mem_ctx) ir_expression(ir_binop_any_nequal, val,
               if (test_equal)
            ir_assignment *const assign =
            }
      static bool
   has_matrix_operand(const ir_expression *expr, unsigned &columns)
   {
      for (unsigned i = 0; i < expr->num_operands; i++) {
      if (expr->operands[i]->type->is_matrix()) {
      columns = expr->operands[i]->type->matrix_columns;
                     }
         ir_visitor_status
   ir_mat_op_to_vec_visitor::visit_leave(ir_assignment *orig_assign)
   {
      ir_expression *orig_expr = orig_assign->rhs->as_expression();
   unsigned int i, matrix_columns = 1;
            if (!orig_expr)
            if (!has_matrix_operand(orig_expr, matrix_columns))
                              ir_dereference_variable *result =
                  /* Store the expression operands in temps so we can use them
   * multiple times.
   */
   for (i = 0; i < orig_expr->num_operands; i++) {
      ir_assignment *assign;
            /* Avoid making a temporary if we don't need to to avoid aliasing. */
   if (deref &&
      deref->variable_referenced() != result->variable_referenced()) {
   op[i] = deref;
               /* Otherwise, store the operand in a temporary generally if it's
   * not a dereference.
   */
   ir_variable *var = new(mem_ctx) ir_variable(orig_expr->operands[i]->type,
                        /* Note that we use this dereference for the assignment.  That means
   * that others that want to use op[i] have to clone the deref.
   */
   op[i] = new(mem_ctx) ir_dereference_variable(var);
   assign = new(mem_ctx) ir_assignment(op[i], orig_expr->operands[i]);
               /* OK, time to break down this matrix operation. */
   switch (orig_expr->operation) {
   case ir_unop_d2f:
   case ir_unop_f2d:
   case ir_unop_f2f16:
   case ir_unop_f2fmp:
   case ir_unop_f162f:
   case ir_unop_neg: {
      /* Apply the operation to each column.*/
   for (i = 0; i < matrix_columns; i++) {
                                    column_assign = new(mem_ctx) ir_assignment(get_column(result, i),
         assert(column_assign->write_mask != 0);
      }
      }
   case ir_binop_add:
   case ir_binop_sub:
   case ir_binop_div:
   case ir_binop_mod: {
      /* For most operations, the matrix version is just going
   * column-wise through and applying the operation to each column
   * if available.
   */
   for (i = 0; i < matrix_columns; i++) {
                     column_expr = new(mem_ctx) ir_expression(orig_expr->operation,
                  column_assign = new(mem_ctx) ir_assignment(get_column(result, i),
         assert(column_assign->write_mask != 0);
      }
      }
   case ir_binop_mul:
      if (op[0]->type->is_matrix()) {
      if (op[1]->type->is_matrix()) {
         } else if (op[1]->type->is_vector()) {
         } else {
      assert(op[1]->type->is_scalar());
         } else {
      assert(op[1]->type->is_matrix());
   if (op[0]->type->is_vector()) {
         } else {
      assert(op[0]->type->is_scalar());
         }
         case ir_binop_all_equal:
   case ir_binop_any_nequal:
      do_equal_mat_mat(result, op[1], op[0],
               default:
      printf("FINISHME: Handle matrix operation for %s\n",
            }
   orig_assign->remove();
               }
