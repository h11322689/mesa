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
      #include "compiler/nir_types.h"
   #include "nir.h"
   #include "nir_builder.h"
   #include "nir_deref.h"
      /*
   * Lowers all copy intrinsics to sequences of load/store intrinsics.
   */
      static nir_deref_instr *
   build_deref_to_next_wildcard(nir_builder *b,
               {
      for (; **deref_arr; (*deref_arr)++) {
      if ((**deref_arr)->deref_type == nir_deref_type_array_wildcard)
                        assert(**deref_arr == NULL);
   *deref_arr = NULL;
      }
      static void
   emit_deref_copy_load_store(nir_builder *b,
                              nir_deref_instr *dst_deref,
      {
      if (dst_deref_arr || src_deref_arr) {
      assert(dst_deref_arr && src_deref_arr);
   dst_deref = build_deref_to_next_wildcard(b, dst_deref, &dst_deref_arr);
               if (dst_deref_arr || src_deref_arr) {
      assert(dst_deref_arr && src_deref_arr);
   assert((*dst_deref_arr)->deref_type == nir_deref_type_array_wildcard);
            unsigned length = glsl_get_length(src_deref->type);
   /* The wildcards should represent the same number of elements */
   assert(length == glsl_get_length(dst_deref->type));
            for (unsigned i = 0; i < length; i++) {
      emit_deref_copy_load_store(b,
                           } else {
      assert(glsl_get_bare_type(dst_deref->type) ==
                  nir_store_deref_with_access(b, dst_deref,
               }
      void
   nir_lower_deref_copy_instr(nir_builder *b, nir_intrinsic_instr *copy)
   {
      /* Unfortunately, there's just no good way to handle wildcards except to
   * flip the chain around and walk the list from variable to final pointer.
   */
   nir_deref_instr *dst = nir_instr_as_deref(copy->src[0].ssa->parent_instr);
            nir_deref_path dst_path, src_path;
   nir_deref_path_init(&dst_path, dst, NULL);
            b->cursor = nir_before_instr(&copy->instr);
   emit_deref_copy_load_store(b, dst_path.path[0], &dst_path.path[1],
                        nir_deref_path_finish(&dst_path);
      }
      static bool
   lower_var_copies_instr(nir_builder *b, nir_intrinsic_instr *copy, void *data)
   {
      if (copy->intrinsic != nir_intrinsic_copy_deref)
                     nir_instr_remove(&copy->instr);
   nir_deref_instr_remove_if_unused(nir_src_as_deref(copy->src[0]));
            nir_instr_free(&copy->instr);
      }
      /* Lowers every copy_var instruction in the program to a sequence of
   * load/store instructions.
   */
   bool
   nir_lower_var_copies(nir_shader *shader)
   {
               return nir_shader_intrinsics_pass(shader, lower_var_copies_instr,
                  }
