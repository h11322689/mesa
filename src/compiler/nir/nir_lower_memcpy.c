   /*
   * Copyright © 2020 Intel Corporation
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
      #include "nir_builder.h"
      #include <string.h>
      /** Returns the type to use for a copy of the given size.
   *
   * The actual type doesn't matter here all that much as we're just going to do
   * a load/store on it and never any arithmetic.
   */
   static const struct glsl_type *
   copy_type_for_byte_size(unsigned size)
   {
      switch (size) {
   case 1:
         case 2:
         case 4:
         case 8:
         case 16:
         default:
            }
      static nir_def *
   memcpy_load_deref_elem(nir_builder *b, nir_deref_instr *parent,
         {
               index = nir_i2iN(b, index, parent->def.bit_size);
   assert(parent->deref_type == nir_deref_type_cast);
               }
      static nir_def *
   memcpy_load_deref_elem_imm(nir_builder *b, nir_deref_instr *parent,
         {
      nir_def *idx = nir_imm_intN_t(b, index, parent->def.bit_size);
      }
      static void
   memcpy_store_deref_elem(nir_builder *b, nir_deref_instr *parent,
         {
               index = nir_i2iN(b, index, parent->def.bit_size);
   assert(parent->deref_type == nir_deref_type_cast);
   deref = nir_build_deref_ptr_as_array(b, parent, index);
      }
      static void
   memcpy_store_deref_elem_imm(nir_builder *b, nir_deref_instr *parent,
         {
      nir_def *idx = nir_imm_intN_t(b, index, parent->def.bit_size);
      }
      static bool
   lower_memcpy_impl(nir_function_impl *impl)
   {
                        nir_foreach_block_safe(block, impl) {
      nir_foreach_instr_safe(instr, block) {
                     nir_intrinsic_instr *cpy = nir_instr_as_intrinsic(instr);
                           nir_deref_instr *dst = nir_src_as_deref(cpy->src[0]);
   nir_deref_instr *src = nir_src_as_deref(cpy->src[1]);
   if (nir_src_is_const(cpy->src[2])) {
      found_const_memcpy = true;
   uint64_t size = nir_src_as_uint(cpy->src[2]);
   uint64_t offset = 0;
   while (offset < size) {
      uint64_t remaining = size - offset;
   /* Find the largest chunk size power-of-two (MSB in remaining)
   * and limit our chunk to 16B (a vec4). It's important to do as
   * many 16B chunks as possible first so that the index
   * computation is correct for
   * memcpy_(load|store)_deref_elem_imm.
   */
                        nir_deref_instr *copy_dst =
      nir_build_deref_cast(&b, &dst->def, dst->modes,
                           uint64_t index = offset / copy_size;
   nir_def *value =
         memcpy_store_deref_elem_imm(&b, copy_dst, index, value);
         } else {
                     /* In this case, we don't have any idea what the size is so we
   * emit a loop which copies one byte at a time.
   */
   nir_deref_instr *copy_dst =
      nir_build_deref_cast(&b, &dst->def, dst->modes,
      nir_deref_instr *copy_src =
                  nir_variable *i = nir_local_variable_create(impl,
         nir_store_var(&b, i, nir_imm_intN_t(&b, 0, size->bit_size), ~0);
   nir_push_loop(&b);
   {
      nir_def *index = nir_load_var(&b, i);
   nir_push_if(&b, nir_uge(&b, index, size));
   {
                        nir_def *value =
         memcpy_store_deref_elem(&b, copy_dst, index, value);
      }
                     if (found_non_const_memcpy) {
         } else if (found_const_memcpy) {
      nir_metadata_preserve(impl, nir_metadata_block_index |
      } else {
                     }
      bool
   nir_lower_memcpy(nir_shader *shader)
   {
               nir_foreach_function_impl(impl, shader) {
      if (lower_memcpy_impl(impl))
                  }
