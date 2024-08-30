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
      #include "link_uniform_block_active_visitor.h"
   #include "program.h"
   #include "linker_util.h"
      static link_uniform_block_active *
   process_block(void *mem_ctx, struct hash_table *ht, ir_variable *var)
   {
      const hash_entry *const existing_block =
            const glsl_type *const block_type = var->is_interface_instance()
               /* If a block with this block-name has not previously been seen, add it.
   * If a block with this block-name has been seen, it must be identical to
   * the block currently being examined.
   */
   if (existing_block == NULL) {
      link_uniform_block_active *const b =
            b->type = block_type;
   b->has_instance_name = var->is_interface_instance();
            if (var->data.explicit_binding) {
      b->has_binding = true;
      } else {
      b->has_binding = false;
               _mesa_hash_table_insert(ht, glsl_get_type_name(var->get_interface_type()), (void *) b);
      } else {
      link_uniform_block_active *const b =
            if (b->type != block_type
      || b->has_instance_name != var->is_interface_instance())
      else
               assert(!"Should not get here.");
      }
      /* For arrays of arrays this function will give us a middle ground between
   * detecting inactive uniform blocks and structuring them in a way that makes
   * it easy to calculate the offset for indirect indexing.
   *
   * For example given the shader:
   *
   *   uniform ArraysOfArraysBlock
   *   {
   *      vec4 a;
   *   } i[3][4][5];
   *
   *   void main()
   *   {
   *      vec4 b = i[0][1][1].a;
   *      gl_Position = i[2][2][3].a + b;
   *   }
   *
   * There are only 2 active blocks above but for the sake of indirect indexing
   * and not over complicating the code we will end up with a count of 8.  Here
   * each dimension has 2 different indices counted so we end up with 2*2*2
   */
   static struct uniform_block_array_elements **
   process_arrays(void *mem_ctx, ir_dereference_array *ir,
         {
      if (ir) {
      struct uniform_block_array_elements **ub_array_ptr =
         if (*ub_array_ptr == NULL) {
      *ub_array_ptr = rzalloc(mem_ctx, struct uniform_block_array_elements);
   (*ub_array_ptr)->ir = ir;
   (*ub_array_ptr)->aoa_size =
               struct uniform_block_array_elements *ub_array = *ub_array_ptr;
   ir_constant *c = ir->array_index->as_constant();
   if (c) {
      /* Index is a constant, so mark just that element used, if not
   * already.
                  unsigned i;
   for (i = 0; i < ub_array->num_array_elements; i++) {
      if (ub_array->array_elements[i] == idx)
                        if (i == ub_array->num_array_elements) {
      ub_array->array_elements = reralloc(mem_ctx,
                                       } else {
      /* The array index is not a constant, so mark the entire array used. */
   assert(ir->array->type->is_array());
   if (ub_array->num_array_elements < ir->array->type->length) {
      ub_array->num_array_elements = ir->array->type->length;
   ub_array->array_elements = reralloc(mem_ctx,
                        for (unsigned i = 0; i < ub_array->num_array_elements; i++) {
                           } else {
            }
      ir_visitor_status
   link_uniform_block_active_visitor::visit(ir_variable *var)
   {
      if (!var->is_in_buffer_block())
            /* Section 2.11.6 (Uniform Variables) of the OpenGL ES 3.0.3 spec says:
   *
   *     "All members of a named uniform block declared with a shared or
   *     std140 layout qualifier are considered active, even if they are not
   *     referenced in any shader in the program. The uniform block itself is
   *     also considered active, even if no member of the block is
   *     referenced."
   */
   if (var->get_interface_type_packing() == GLSL_INTERFACE_PACKING_PACKED)
            /* Process the block.  Bail if there was an error. */
   link_uniform_block_active *const b =
         if (b == NULL) {
      linker_error(this->prog,
               this->success = false;
               assert(b->array == NULL);
   assert(b->type != NULL);
            /* For uniform block arrays declared with a shared or std140 layout
   * qualifier, mark all its instances as used.
   */
   const glsl_type *type = b->type;
   struct uniform_block_array_elements **ub_array = &b->array;
   while (type->is_array()) {
               *ub_array = rzalloc(this->mem_ctx, struct uniform_block_array_elements);
   (*ub_array)->num_array_elements = type->length;
   (*ub_array)->array_elements = reralloc(this->mem_ctx,
                              for (unsigned i = 0; i < (*ub_array)->num_array_elements; i++) {
         }
   ub_array = &(*ub_array)->array;
                  }
      ir_visitor_status
   link_uniform_block_active_visitor::visit_enter(ir_dereference_array *ir)
   {
      /* cycle through arrays of arrays */
   ir_dereference_array *base_ir = ir;
   while (base_ir->array->ir_type == ir_type_dereference_array)
            ir_dereference_variable *const d =
                  /* If the r-value being dereferenced is not a variable (e.g., a field of a
   * structure) or is not a uniform block instance, continue.
   *
   * WARNING: It is not enough for the variable to be part of uniform block.
   * It must represent the entire block.  Arrays (or matrices) inside blocks
   * that lack an instance name are handled by the ir_dereference_variable
   * function.
   */
   if (var == NULL
      || !var->is_in_buffer_block()
   || !var->is_interface_instance())
         /* Process the block.  Bail if there was an error. */
   link_uniform_block_active *const b =
         if (b == NULL) {
      linker_error(prog,
               this->success = false;
               /* Block arrays must be declared with an instance name.
   */
   assert(b->has_instance_name);
            /* If the block array was declared with a shared or std140 layout
   * qualifier, all its instances have been already marked as used in
   * link_uniform_block_active_visitor::visit(ir_variable *).
   */
   if (var->get_interface_type_packing() == GLSL_INTERFACE_PACKING_PACKED) {
      b->var = var;
                  }
      ir_visitor_status
   link_uniform_block_active_visitor::visit(ir_dereference_variable *ir)
   {
               if (!var->is_in_buffer_block())
                     /* Process the block.  Bail if there was an error. */
   link_uniform_block_active *const b =
         if (b == NULL) {
      linker_error(this->prog,
               this->success = false;
               assert(b->array == NULL);
               }
