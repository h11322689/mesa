   /*
   * Copyright © 2011 Intel Corporation
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
      #include "ir.h"
   #include "linker.h"
   #include "ir_uniform.h"
   #include "glsl_symbol_table.h"
   #include "program.h"
   #include "string_to_uint_map.h"
   #include "ir_array_refcount.h"
      #include "main/shader_types.h"
   #include "main/consts_exts.h"
   #include "util/strndup.h"
   #include "util/u_math.h"
      /**
   * \file link_uniforms.cpp
   * Assign locations for GLSL uniforms.
   *
   * \author Ian Romanick <ian.d.romanick@intel.com>
   */
      void
   program_resource_visitor::process(const glsl_type *type, const char *name,
         {
      assert(type->without_array()->is_struct()
            unsigned record_array_count = 1;
            enum glsl_interface_packing packing =
            recursion(type, &name_copy, strlen(name), false, NULL, packing, false,
            }
      void
   program_resource_visitor::process(ir_variable *var, bool use_std430_as_default)
   {
      const glsl_type *t =
            }
      void
   program_resource_visitor::process(ir_variable *var, const glsl_type *var_type,
         {
      unsigned record_array_count = 1;
   const bool row_major =
            enum glsl_interface_packing packing = var->get_interface_type() ?
      var->get_interface_type()->
               const glsl_type *t = var_type;
            /* false is always passed for the row_major parameter to the other
   * processing functions because no information is available to do
   * otherwise.  See the warning in linker.h.
   */
   if (t_without_array->is_struct() ||
            char *name = ralloc_strdup(NULL, var->name);
   recursion(var->type, &name, strlen(name), row_major, NULL, packing,
            } else if (t_without_array->is_interface()) {
      char *name = ralloc_strdup(NULL, glsl_get_type_name(t_without_array));
   const glsl_struct_field *ifc_member = var->data.from_named_ifc_block ?
                  recursion(t, &name, strlen(name), row_major, NULL, packing,
            } else {
      this->set_record_array_count(record_array_count);
         }
      void
   program_resource_visitor::recursion(const glsl_type *t, char **name,
                                       {
      /* Records need to have each field processed individually.
   *
   * Arrays of records need to have each array element processed
   * individually, then each field of the resulting array elements processed
   * individually.
   */
   if (t->is_interface() && named_ifc_member) {
      ralloc_asprintf_rewrite_tail(name, &name_length, ".%s",
         recursion(named_ifc_member->type, name, name_length, row_major, NULL,
      } else if (t->is_struct() || t->is_interface()) {
      if (record_type == NULL && t->is_struct())
            if (t->is_struct())
            for (unsigned i = 0; i < t->length; i++) {
                                    /* Append '.field' to the current variable name. */
   if (name_length == 0) {
         } else {
                  /* The layout of structures at the top level of the block is set
   * during parsing.  For matrices contained in multiple levels of
   * structures in the block, the inner structures have no layout.
   * These cases must potentially inherit the layout from the outer
   * levels.
   */
   bool field_row_major = row_major;
   const enum glsl_matrix_layout matrix_layout =
         if (matrix_layout == GLSL_MATRIX_LAYOUT_ROW_MAJOR) {
         } else if (matrix_layout == GLSL_MATRIX_LAYOUT_COLUMN_MAJOR) {
                  recursion(t->fields.structure[i].type, name, new_length,
            field_row_major,
               /* Only the first leaf-field of the record gets called with the
   * record type pointer.
   */
               if (t->is_struct()) {
      (*name)[name_length] = '\0';
         } else if (t->without_array()->is_struct() ||
            t->without_array()->is_interface() ||
   if (record_type == NULL && t->fields.array->is_struct())
                     /* Shader storage block unsized arrays: add subscript [0] to variable
   * names.
   */
   if (t->is_unsized_array())
                     for (unsigned i = 0; i < length; i++) {
                              recursion(t->fields.array, name, new_length, row_major,
            record_type,
               /* Only the first leaf-field of the record gets called with the
   * record type pointer.
   */
         } else {
      this->set_record_array_count(record_array_count);
         }
      void
   program_resource_visitor::enter_record(const glsl_type *, const char *, bool,
         {
   }
      void
   program_resource_visitor::leave_record(const glsl_type *, const char *, bool,
         {
   }
      void
   program_resource_visitor::set_buffer_offset(unsigned)
   {
   }
      void
   program_resource_visitor::set_record_array_count(unsigned)
   {
   }
      unsigned
   link_calculate_matrix_stride(const glsl_type *matrix, bool row_major,
         {
      const unsigned N = matrix->is_double() ? 8 : 4;
   const unsigned items =
                     /* Matrix stride for std430 mat2xY matrices are not rounded up to
   * vec4 size.
   *
   * Section 7.6.2.2 "Standard Uniform Block Layout" of the OpenGL 4.3 spec
   * says:
   *
   *    2. If the member is a two- or four-component vector with components
   *       consuming N basic machine units, the base alignment is 2N or 4N,
   *       respectively.
   *    ...
   *    4. If the member is an array of scalars or vectors, the base
   *       alignment and array stride are set to match the base alignment of
   *       a single array element, according to rules (1), (2), and (3), and
   *       rounded up to the base alignment of a vec4.
   *    ...
   *    7. If the member is a row-major matrix with C columns and R rows, the
   *       matrix is stored identically to an array of R row vectors with C
   *       components each, according to rule (4).
   *    ...
   *
   *    When using the std430 storage layout, shader storage blocks will be
   *    laid out in buffer storage identically to uniform and shader storage
   *    blocks using the std140 layout, except that the base alignment and
   *    stride of arrays of scalars and vectors in rule 4 and of structures
   *    in rule 9 are not rounded up a multiple of the base alignment of a
   *    vec4.
   */
   return packing == GLSL_INTERFACE_PACKING_STD430
      ? (items < 3 ? items * N : align(items * N, 16))
   }
