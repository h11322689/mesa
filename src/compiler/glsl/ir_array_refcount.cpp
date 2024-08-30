   /*
   * Copyright © 2016 Intel Corporation
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
   * \file ir_array_refcount.cpp
   *
   * Provides a visitor which produces a list of variables referenced.
   */
      #include "ir.h"
   #include "ir_visitor.h"
   #include "ir_array_refcount.h"
   #include "compiler/glsl_types.h"
   #include "util/hash_table.h"
      ir_array_refcount_visitor::ir_array_refcount_visitor()
         {
      this->mem_ctx = ralloc_context(NULL);
      }
      static void
   free_entry(struct hash_entry *entry)
   {
      ir_array_refcount_entry *ivre = (ir_array_refcount_entry *) entry->data;
      }
      ir_array_refcount_visitor::~ir_array_refcount_visitor()
   {
      ralloc_free(this->mem_ctx);
      }
      ir_array_refcount_entry::ir_array_refcount_entry(ir_variable *var)
         {
      num_bits = MAX2(1, var->type->arrays_of_arrays_size());
   bits = new BITSET_WORD[BITSET_WORDS(num_bits)];
            /* Count the "depth" of the arrays-of-arrays. */
   array_depth = 0;
   for (const glsl_type *type = var->type;
      type->is_array();
   type = type->fields.array) {
         }
         ir_array_refcount_entry::~ir_array_refcount_entry()
   {
         }
      ir_array_refcount_entry *
   ir_array_refcount_visitor::get_variable_entry(ir_variable *var)
   {
               struct hash_entry *e = _mesa_hash_table_search(this->ht, var);
   if (e)
            ir_array_refcount_entry *entry = new ir_array_refcount_entry(var);
               }
         array_deref_range *
   ir_array_refcount_visitor::get_array_deref()
   {
      if ((num_derefs + 1) * sizeof(array_deref_range) > derefs_size) {
               if (ptr == NULL)
            derefs_size += 4096;
               array_deref_range *d = &derefs[num_derefs];
               }
      ir_visitor_status
   ir_array_refcount_visitor::visit_enter(ir_dereference_array *ir)
   {
      /* It could also be a vector or a matrix.  Individual elements of vectors
   * are natrices are not tracked, so bail.
   */
   if (!ir->array->type->is_array())
            /* If this array dereference is a child of an array dereference that was
   * already visited, just continue on.  Otherwise, for an arrays-of-arrays
   * dereference like x[1][2][3][4], we'd process the [1][2][3][4] sequence,
   * the [1][2][3] sequence, the [1][2] sequence, and the [1] sequence.  This
   * ensures that we only process the full sequence.
   */
   if (last_array_deref && last_array_deref->array == ir) {
      last_array_deref = ir;
                                 ir_rvalue *rv = ir;
   while (rv->ir_type == ir_type_dereference_array) {
               assert(deref != NULL);
            ir_rvalue *const array = deref->array;
   const ir_constant *const idx = deref->array_index->as_constant();
                     if (idx != NULL) {
         } else {
      /* An unsized array can occur at the end of an SSBO.  We can't track
   * accesses to such an array, so bail.
   */
                                                   /* If the array being dereferenced is not a variable, bail.  At the very
   * least, ir_constant and ir_dereference_record are possible.
   */
   if (var_deref == NULL)
            ir_array_refcount_entry *const entry =
            if (entry == NULL)
            link_util_mark_array_elements_referenced(derefs, num_derefs,
                     }
         ir_visitor_status
   ir_array_refcount_visitor::visit(ir_dereference_variable *ir)
   {
      ir_variable *const var = ir->variable_referenced();
                        }
         ir_visitor_status
   ir_array_refcount_visitor::visit_enter(ir_function_signature *ir)
   {
      /* We don't want to descend into the function parameters and
   * dead-code eliminate them, so just accept the body here.
   */
   visit_list_elements(this, &ir->body);
      }
