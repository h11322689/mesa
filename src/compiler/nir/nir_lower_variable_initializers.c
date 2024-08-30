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
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   * IN THE SOFTWARE.
   */
      #include "nir.h"
   #include "nir_builder.h"
      static void
   build_constant_load(nir_builder *b, nir_deref_instr *deref, nir_constant *c)
   {
      if (glsl_type_is_vector_or_scalar(deref->type)) {
      nir_load_const_instr *load =
      nir_load_const_instr_create(b->shader,
            memcpy(load->value, c->values, sizeof(*load->value) * load->def.num_components);
   nir_builder_instr_insert(b, &load->instr);
      } else if (glsl_type_is_struct_or_ifc(deref->type)) {
      unsigned len = glsl_get_length(deref->type);
   for (unsigned i = 0; i < len; i++) {
      build_constant_load(b, nir_build_deref_struct(b, deref, i),
         } else {
      assert(glsl_type_is_array(deref->type) ||
         unsigned len = glsl_get_length(deref->type);
   for (unsigned i = 0; i < len; i++) {
      build_constant_load(b,
                  }
      static bool
   lower_const_initializer(struct nir_builder *b, struct exec_list *var_list,
         {
                        nir_foreach_variable_in_list(var, var_list) {
      if (!(var->data.mode & modes))
            if (var->constant_initializer) {
                     progress = true;
      } else if (var->pointer_initializer) {
                                    progress = true;
                     }
      bool
   nir_lower_variable_initializers(nir_shader *shader, nir_variable_mode modes)
   {
               /* Only some variables have initializers that we want to lower.  Others
   * such as uniforms have initializers which are useful later during linking
   * so we want to skip over those.  Restrict to only variable types where
   * initializers make sense so that callers can use nir_var_all.
   */
   modes &= nir_var_shader_out |
            nir_var_shader_temp |
         nir_foreach_function_with_impl(func, impl, shader) {
      bool impl_progress = false;
            if ((modes & ~nir_var_function_temp) && func->is_entrypoint) {
      impl_progress |= lower_const_initializer(&builder,
                     if (modes & nir_var_function_temp) {
      impl_progress |= lower_const_initializer(&builder,
                     if (impl_progress) {
      progress = true;
   nir_metadata_preserve(impl, nir_metadata_block_index |
            } else {
                        }
      /* Zero initialize shared_size bytes of shared memory by splitting work writes
   * of chunk_size bytes among the invocations.
   *
   * Used for implementing VK_KHR_zero_initialize_workgroup_memory.
   */
   bool
   nir_zero_initialize_shared_memory(nir_shader *shader,
               {
      assert(shared_size > 0);
   assert(chunk_size > 0);
            nir_function_impl *impl = nir_shader_get_entrypoint(shader);
            assert(!shader->info.workgroup_size_variable);
   const unsigned local_count = shader->info.workgroup_size[0] *
                  /* The initialization logic is simplified if we can always split the memory
   * in full chunk_size units.
   */
                     nir_variable *it = nir_local_variable_create(b.impl, glsl_uint_type(),
         nir_def *local_index = nir_load_local_invocation_index(&b);
   nir_def *first_offset = nir_imul_imm(&b, local_index, chunk_size);
            nir_loop *loop = nir_push_loop(&b);
   {
               nir_push_if(&b, nir_uge_imm(&b, offset, shared_size));
   {
         }
            nir_store_shared(&b, nir_imm_zero(&b, chunk_comps, 32), offset,
                  nir_def *new_offset = nir_iadd_imm(&b, offset, chunk_size * local_count);
      }
            nir_barrier(&b, SCOPE_WORKGROUP, SCOPE_WORKGROUP, NIR_MEMORY_ACQ_REL,
                        }
