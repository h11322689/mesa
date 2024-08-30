   /*
   * Copyright © 2018 Timothy Arceri
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
      #include "nir.h"
   #include "nir_builder.h"
   #include "nir_deref.h"
   #include "util/u_dynarray.h"
   #include "util/u_math.h"
   #define XXH_INLINE_ALL
   #include "util/xxhash.h"
      /** @file nir_opt_vectorize_io.c
   *
   * Replaces scalar nir_load_input/nir_store_output operations with
   * vectorized instructions.
   */
   bool
   r600_vectorize_vs_inputs(nir_shader *shader);
      static nir_deref_instr *
   r600_clone_deref_array(nir_builder *b,
               {
               if (!parent)
                                 }
      static bool
   r600_variable_can_rewrite(nir_variable *var)
   {
         /* Skip complex types we don't split in the first place */
   if (!glsl_type_is_vector_or_scalar(glsl_without_array(var->type)))
            /* TODO: add 64/16bit support ? */
   if (glsl_get_bit_size(glsl_without_array(var->type)) != 32)
            /* We only check VSand attribute imputs */
   return (var->data.location >= VERT_ATTRIB_GENERIC0 &&
      }
      static bool
   r600_instr_can_rewrite(nir_instr *instr)
   {
      if (instr->type != nir_instr_type_intrinsic)
                     if (intr->num_components > 3)
            if (intr->intrinsic != nir_intrinsic_load_deref)
            nir_deref_instr *deref = nir_src_as_deref(intr->src[0]);
   if (!nir_deref_mode_is(deref, nir_var_shader_in))
               }
      static bool
   r600_io_access_same_var(const nir_instr *instr1, const nir_instr *instr2)
   {
      assert(instr1->type == nir_instr_type_intrinsic &&
            nir_intrinsic_instr *intr1 = nir_instr_as_intrinsic(instr1);
            nir_variable *var1 = nir_intrinsic_get_var(intr1, 0);
            /* We don't handle combining vars of different base types, so skip those */
   if (glsl_get_base_type(var1->type) != glsl_get_base_type(var2->type))
            if (var1->data.location != var2->data.location)
               }
      static struct util_dynarray *
   r600_vec_instr_stack_create(void *mem_ctx)
   {
      struct util_dynarray *stack = ralloc(mem_ctx, struct util_dynarray);
   util_dynarray_init(stack, mem_ctx);
      }
      static void
   r600_vec_instr_stack_push(struct util_dynarray *stack, nir_instr *instr)
   {
         }
      static unsigned
   r600_correct_location(nir_variable *var)
   {
         }
      static void
   r600_create_new_load(nir_builder *b,
                        nir_intrinsic_instr *intr,
      {
                        nir_intrinsic_instr *new_intr = nir_intrinsic_instr_create(b->shader, intr->intrinsic);
   nir_def_init(&new_intr->instr, &new_intr->def, num_comps,
                  nir_deref_instr *deref = nir_build_deref_var(b, var);
                     if (intr->intrinsic == nir_intrinsic_interp_deref_at_offset ||
      intr->intrinsic == nir_intrinsic_interp_deref_at_sample)
                  for (unsigned i = 0; i < old_num_comps; ++i)
         nir_def *load = nir_swizzle(b, &new_intr->def, channels, old_num_comps);
            /* Remove the old load intrinsic */
      }
      static bool
   r600_vec_instr_stack_pop(nir_builder *b,
                     {
               assert(last == instr);
            nir_intrinsic_instr *intr = nir_instr_as_intrinsic(last);
   nir_variable *var = nir_intrinsic_get_var(intr, 0);
            nir_variable *new_var;
                              /* Don't bother walking the stack if this component can't be vectorised. */
   if (old_num_comps > 3) {
                  if (new_var == var) {
                  r600_create_new_load(
            }
      static bool
   r600_cmp_func(const void *data1, const void *data2)
   {
      const struct util_dynarray *arr1 = data1;
            const nir_instr *instr1 = *(nir_instr **)util_dynarray_begin(arr1);
               }
      #define HASH(hash, data) XXH32(&(data), sizeof(data), (hash))
      static uint32_t
   r600_hash_instr(const nir_instr *instr)
   {
               nir_intrinsic_instr *intr = nir_instr_as_intrinsic(instr);
                     hash = HASH(hash, var->type);
      }
      static uint32_t
   r600_hash_stack(const void *data)
   {
      const struct util_dynarray *stack = data;
   const nir_instr *first = *(nir_instr **)util_dynarray_begin(stack);
      }
      static struct set *
   r600_vec_instr_set_create(void)
   {
         }
      static void
   r600_vec_instr_set_destroy(struct set *instr_set)
   {
         }
      static void
   r600_vec_instr_set_add(struct set *instr_set, nir_instr *instr)
   {
      if (!r600_instr_can_rewrite(instr)) {
                  struct util_dynarray *new_stack = r600_vec_instr_stack_create(instr_set);
                     if (entry) {
      ralloc_free(new_stack);
   struct util_dynarray *stack = (struct util_dynarray *)entry->key;
   r600_vec_instr_stack_push(stack, instr);
                           }
      static bool
   r600_vec_instr_set_remove(nir_builder *b,
                     {
      if (!r600_instr_can_rewrite(instr)) {
         }
   /*
   * It's pretty unfortunate that we have to do this, but it's a side effect
   * of the hash set interfaces. The hash set assumes that we're only
   * interested in storing one equivalent element at a time, and if we try to
   * insert a duplicate element it will remove the original. We could hack up
   * the comparison function to "know" which input is an instruction we
   * passed in and which is an array that's part of the entry, but that
   * wouldn't work because we need to pass an array to _mesa_set_add() in
   * vec_instr_add() above, and _mesa_set_add() will call our comparison
   * function as well.
   */
   struct util_dynarray *temp = r600_vec_instr_stack_create(instr_set);
   r600_vec_instr_stack_push(temp, instr);
   struct set_entry *entry = _mesa_set_search(instr_set, temp);
            if (entry) {
      struct util_dynarray *stack = (struct util_dynarray *)entry->key;
            if (!util_dynarray_num_elements(stack, nir_instr *))
                           }
      static bool
   r600_vectorize_block(nir_builder *b,
                     {
                        for (unsigned i = 0; i < block->num_dom_children; i++) {
      nir_block *child = block->dom_children[i];
               nir_foreach_instr_reverse_safe(instr, block)
   {
                     }
      static void
   r600_create_new_io_var(nir_shader *shader,
                     {
      unsigned num_comps = util_bitcount(comps);
            /* Note: u_bit_scan() strips a component of the comps bitfield here */
            nir_variable *var = nir_variable_clone(vars[location][first_comp], shader);
   var->data.location_frac = first_comp;
                              while (comps) {
      const int comp = u_bit_scan(&comps);
   if (vars[location][comp]) {
               }
      static inline bool
   r600_variables_can_merge(const nir_variable *lhs, const nir_variable *rhs)
   {
         }
      static void
   r600_create_new_io_vars(nir_shader *shader,
               {
      bool can_rewrite_vars = false;
   nir_foreach_variable_with_modes(var, shader, mode)
   {
      if (r600_variable_can_rewrite(var)) {
      can_rewrite_vars = true;
   unsigned loc = r600_correct_location(var);
                  if (!can_rewrite_vars)
            /* We don't handle combining vars of different type e.g. different array
   * lengths.
   */
   for (unsigned i = 0; i < 16; i++) {
                                          for (unsigned k = j + 1; k < 4; k++) {
                                    /* Set comps */
                  for (unsigned n = 0; n < glsl_get_components(vars[i][k]->type); ++n)
         }
   if (comps)
         }
      static bool
   r600_vectorize_io_impl(nir_function_impl *impl)
   {
                        nir_shader *shader = impl->function->shader;
                     struct set *instr_set = r600_vec_instr_set_create();
   bool progress =
            if (progress) {
         } else {
                  r600_vec_instr_set_destroy(instr_set);
      }
      bool
   r600_vectorize_vs_inputs(nir_shader *shader)
   {
               if (shader->info.stage != MESA_SHADER_VERTEX)
            nir_foreach_function_impl(impl, shader)
   {
                     }
