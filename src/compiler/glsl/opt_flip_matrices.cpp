   /*
   * Copyright © 2013 Intel Corporation
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
   * \file opt_flip_matrices.cpp
   *
   * Convert (matrix * vector) operations to (vector * matrixTranspose),
   * which can be done using dot products rather than multiplies and adds.
   * On some hardware, this is more efficient.
   *
   * This currently only does the conversion for built-in matrices which
   * already have transposed equivalents.  Namely, gl_ModelViewProjectionMatrix
   * and gl_TextureMatrix.
   */
   #include "ir.h"
   #include "ir_optimization.h"
   #include "main/macros.h"
      namespace {
   class matrix_flipper : public ir_hierarchical_visitor {
   public:
      matrix_flipper(exec_list *instructions)
   {
      progress = false;
   mvp_transpose = NULL;
            foreach_in_list(ir_instruction, ir, instructions) {
      ir_variable *var = ir->as_variable();
   if (!var)
         if (strcmp(var->name, "gl_ModelViewProjectionMatrixTranspose") == 0)
         if (strcmp(var->name, "gl_TextureMatrixTranspose") == 0)
                                 private:
      ir_variable *mvp_transpose;
      };
   }
      ir_visitor_status
   matrix_flipper::visit_enter(ir_expression *ir)
   {
      if (ir->operation != ir_binop_mul ||
      !ir->operands[0]->type->is_matrix() ||
   !ir->operands[1]->type->is_vector())
         ir_variable *mat_var = ir->operands[0]->variable_referenced();
   if (!mat_var)
            if (mvp_transpose &&
      #ifndef NDEBUG
         ir_dereference_variable *deref = ir->operands[0]->as_dereference_variable();
   #endif
                     ir->operands[0] = ir->operands[1];
               } else if (texmat_transpose &&
            ir_dereference_array *array_ref = ir->operands[0]->as_dereference_array();
   assert(array_ref != NULL);
   ir_dereference_variable *var_ref = array_ref->array->as_dereference_variable();
            ir->operands[0] = ir->operands[1];
                     texmat_transpose->data.max_array_access =
                           }
      bool
   opt_flip_matrices(struct exec_list *instructions)
   {
                           }
