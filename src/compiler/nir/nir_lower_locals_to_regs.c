   /*
   * Copyright © 2014 Intel Corporation
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
   */
      #include "nir.h"
   #include "nir_builder.h"
   #include "nir_builder_opcodes.h"
   #include "nir_intrinsics_indices.h"
      struct locals_to_regs_state {
               /* A hash table mapping derefs to register handles */
            /* Bit size to use for boolean registers */
               };
      /* The following two functions implement a hash and equality check for
   * variable dreferences.  When the hash or equality function encounters an
   * array, it ignores the offset and whether it is direct or indirect
   * entirely.
   */
   static uint32_t
   hash_deref(const void *void_deref)
   {
               for (const nir_deref_instr *deref = void_deref; deref;
      deref = nir_deref_instr_parent(deref)) {
   switch (deref->deref_type) {
   case nir_deref_type_var:
            case nir_deref_type_array:
            case nir_deref_type_struct:
                  default:
                        }
      static bool
   derefs_equal(const void *void_a, const void *void_b)
   {
      for (const nir_deref_instr *a = void_a, *b = void_b; a || b;
      a = nir_deref_instr_parent(a), b = nir_deref_instr_parent(b)) {
   if (a->deref_type != b->deref_type)
            switch (a->deref_type) {
   case nir_deref_type_var:
            case nir_deref_type_array:
            case nir_deref_type_struct:
      if (a->strct.index != b->strct.index)
               default:
                        }
      static nir_def *
   get_reg_for_deref(nir_deref_instr *deref, struct locals_to_regs_state *state)
   {
               assert(nir_deref_instr_get_variable(deref)->constant_initializer == NULL &&
            struct hash_entry *entry =
         if (entry)
            unsigned array_size = 1;
   for (nir_deref_instr *d = deref; d; d = nir_deref_instr_parent(d)) {
      if (d->deref_type == nir_deref_type_array)
                        uint8_t bit_size = glsl_get_bit_size(deref->type);
   if (bit_size == 1)
            nir_def *reg = nir_decl_reg(&state->builder,
                              }
      struct reg_location {
      nir_def *reg;
   nir_def *indirect;
      };
      static struct reg_location
   get_deref_reg_location(nir_deref_instr *deref,
         {
               nir_def *reg = get_reg_for_deref(deref, state);
            /* It is possible for a user to create a shader that has an array with a
   * single element and then proceed to access it indirectly.  Indirectly
   * accessing a non-array register is not allowed in NIR.  In order to
   * handle this case we just convert it to a direct reference.
   */
   if (nir_intrinsic_num_array_elems(decl) == 0)
            nir_def *indirect = NULL;
            unsigned inner_array_size = 1;
   for (const nir_deref_instr *d = deref; d; d = nir_deref_instr_parent(d)) {
      if (d->deref_type != nir_deref_type_array)
            if (nir_src_is_const(d->arr.index) && !indirect) {
         } else {
      if (indirect) {
         } else {
      indirect = nir_imm_int(b, base_offset);
                              /* Avoid emitting iadd with 0, which is otherwise common, since this
   * pass runs late enough that nothing will clean it up.
   */
   nir_scalar scal = nir_get_scalar(indirect, 0);
   if (nir_scalar_is_const(scal))
         else
                           return (struct reg_location){
      .reg = reg,
   .indirect = indirect,
         }
      static bool
   lower_locals_to_regs_block(nir_block *block,
         {
               nir_foreach_instr_safe(instr, block) {
      if (instr->type != nir_instr_type_intrinsic)
                     switch (intrin->intrinsic) {
   case nir_intrinsic_load_deref: {
      nir_deref_instr *deref = nir_src_as_deref(intrin->src[0]);
                  b->cursor = nir_after_instr(&intrin->instr);
                  nir_def *value;
   unsigned num_array_elems = nir_intrinsic_num_array_elems(decl);
                  if (loc.base_offset >= MAX2(num_array_elems, 1)) {
      /* out-of-bounds read, return 0 instead. */
      } else if (loc.indirect != NULL) {
      value = nir_load_reg_indirect(b, num_components, bit_size,
            } else {
      value = nir_build_load_reg(b, num_components, bit_size,
               nir_def_rewrite_uses(&intrin->def, value);
   nir_instr_remove(&intrin->instr);
   state->progress = true;
               case nir_intrinsic_store_deref: {
      nir_deref_instr *deref = nir_src_as_deref(intrin->src[0]);
                                          nir_def *val = intrin->src[1].ssa;
                  if (loc.base_offset >= MAX2(num_array_elems, 1)) {
         } else if (loc.indirect) {
      nir_store_reg_indirect(b, val, loc.reg, loc.indirect,
            } else {
      nir_build_store_reg(b, val, loc.reg, .base = loc.base_offset,
               nir_instr_remove(&intrin->instr);
   state->progress = true;
               case nir_intrinsic_copy_deref:
                  default:
                        }
      static bool
   impl(nir_function_impl *impl, uint8_t bool_bitsize)
   {
               state.builder = nir_builder_create(impl);
   state.progress = false;
   state.regs_table = _mesa_hash_table_create(NULL, hash_deref, derefs_equal);
                     nir_foreach_block(block, impl) {
                  nir_metadata_preserve(impl, nir_metadata_block_index |
                        }
      bool
   nir_lower_locals_to_regs(nir_shader *shader, uint8_t bool_bitsize)
   {
               nir_foreach_function_impl(func_impl, shader) {
                     }
