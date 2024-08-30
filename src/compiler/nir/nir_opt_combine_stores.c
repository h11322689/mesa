   /*
   * Copyright © 2019 Intel Corporation
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
   #include "nir_deref.h"
      #include "util/bitscan.h"
   #include "util/list.h"
   #include "util/u_math.h"
      /* Combine stores of vectors to the same deref into a single store.
   *
   * This per-block pass keeps track of stores of vectors to the same
   * destination and combines them into the last store of the sequence.  Dead
   * stores (or parts of the store) found during the process are removed.
   *
   * A pending combination becomes an actual combination in various situations:
   * at the end of the block, when another instruction uses the memory or due to
   * barriers.
   *
   * Besides vectors, the pass also look at array derefs of vectors.  For direct
   * array derefs, it works like a write mask access to the given component.
   * For indirect access there's no way to know before hand what component it
   * will overlap with, so the combination is finished -- the indirect remains
   * unmodified.
   */
      /* Keep track of a group of stores that can be combined.  All stores share the
   * same destination.
   */
   struct combined_store {
               nir_component_mask_t write_mask;
            /* Latest store added.  It is reused when combining. */
            /* Original store for each component.  The number of times a store appear
   * in this array is kept in the store's pass_flags.
   */
      };
      struct combine_stores_state {
               /* Pending store combinations. */
            /* Per function impl state. */
   nir_builder b;
            /* Allocator and freelist to reuse structs between functions. */
   linear_ctx *lin_ctx;
      };
      static struct combined_store *
   alloc_combined_store(struct combine_stores_state *state)
   {
      struct combined_store *result;
   if (list_is_empty(&state->freelist)) {
         } else {
      result = list_first_entry(&state->freelist,
               list_del(&result->link);
      }
      }
      static void
   free_combined_store(struct combine_stores_state *state,
         {
      list_del(&combo->link);
   combo->write_mask = 0;
      }
      static void
   combine_stores(struct combine_stores_state *state,
         {
      assert(combo->latest);
            /* If the combined writemask is the same as the latest store, we know there
   * is only one store in the combination, so nothing to combine.
   */
   if ((combo->write_mask & nir_intrinsic_write_mask(combo->latest)) ==
      combo->write_mask)
                  /* Build a new vec, to be used as source for the combined store.  As it
   * gets build, remove previous stores that are not needed anymore.
   */
   nir_scalar comps[NIR_MAX_VEC_COMPONENTS] = { 0 };
   unsigned num_components = glsl_get_vector_elements(combo->dst->type);
   unsigned bit_size = combo->latest->src[1].ssa->bit_size;
   for (unsigned i = 0; i < num_components; i++) {
      nir_intrinsic_instr *store = combo->stores[i];
   if (combo->write_mask & (1 << i)) {
               /* If store->num_components == 1 then we are in the deref-of-vec case
   * and store->src[1] is a scalar.  Otherwise, we're a regular vector
   * load and we have to pick off a component.
                  assert(store->instr.pass_flags > 0);
   if (--store->instr.pass_flags == 0 && store != combo->latest)
      } else {
            }
   assert(combo->latest->instr.pass_flags == 0);
            /* Fix the latest store with the combined information. */
            /* In this case, our store is as an array deref of a vector so we need to
   * rewrite it to use a deref to the whole vector.
   */
   if (store->num_components == 1) {
      store->num_components = num_components;
               assert(store->num_components == num_components);
   nir_intrinsic_set_write_mask(store, combo->write_mask);
   nir_src_rewrite(&store->src[1], vec);
      }
      static void
   combine_stores_with_deref(struct combine_stores_state *state,
         {
      if (!nir_deref_mode_may_be(deref, state->modes))
            list_for_each_entry_safe(struct combined_store, combo, &state->pending, link) {
      if (nir_compare_derefs(combo->dst, deref) & nir_derefs_may_alias_bit) {
      combine_stores(state, combo);
            }
      static void
   combine_stores_with_modes(struct combine_stores_state *state,
         {
      if ((state->modes & modes) == 0)
            list_for_each_entry_safe(struct combined_store, combo, &state->pending, link) {
      if (nir_deref_mode_may_be(combo->dst, modes)) {
      combine_stores(state, combo);
            }
      static struct combined_store *
   find_matching_combined_store(struct combine_stores_state *state,
         {
      list_for_each_entry(struct combined_store, combo, &state->pending, link) {
      if (nir_compare_derefs(combo->dst, deref) & nir_derefs_equal_bit)
      }
      }
      static void
   update_combined_store(struct combine_stores_state *state,
         {
      nir_deref_instr *dst = nir_src_as_deref(intrin->src[0]);
   if (!nir_deref_mode_may_be(dst, state->modes))
            unsigned vec_mask;
            if (glsl_type_is_vector(dst->type)) {
      vec_mask = nir_intrinsic_write_mask(intrin);
      } else {
      /* Besides vectors, only direct array derefs of vectors are handled. */
   if (dst->deref_type != nir_deref_type_array ||
      !nir_src_is_const(dst->arr.index) ||
   !glsl_type_is_vector(nir_deref_instr_parent(dst)->type)) {
   combine_stores_with_deref(state, dst);
               uint64_t index = nir_src_as_uint(dst->arr.index);
            if (index >= glsl_get_vector_elements(vec_dst->type)) {
      /* Storing to an invalid index is a no-op. */
   nir_instr_remove(&intrin->instr);
   state->progress = true;
                           struct combined_store *combo = find_matching_combined_store(state, vec_dst);
   if (!combo) {
      combo = alloc_combined_store(state);
   combo->dst = vec_dst;
               /* Use pass_flags to reference count the store based on how many
   * components are still used by the combination.
   */
   intrin->instr.pass_flags = util_bitcount(vec_mask);
            /* Update the combined_store, clearing up older overlapping references. */
   combo->write_mask |= vec_mask;
   while (vec_mask) {
      unsigned i = u_bit_scan(&vec_mask);
            if (prev_store) {
      if (--prev_store->instr.pass_flags == 0) {
         } else {
      assert(glsl_type_is_vector(
         nir_component_mask_t prev_mask = nir_intrinsic_write_mask(prev_store);
      }
      }
         }
      static void
   combine_stores_block(struct combine_stores_state *state, nir_block *block)
   {
      nir_foreach_instr_safe(instr, block) {
      if (instr->type == nir_instr_type_call) {
      combine_stores_with_modes(state, nir_var_shader_out |
                                             if (instr->type != nir_instr_type_intrinsic)
            nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(instr);
   switch (intrin->intrinsic) {
   case nir_intrinsic_store_deref:
      if (nir_intrinsic_access(intrin) & ACCESS_VOLATILE) {
      nir_deref_instr *dst = nir_src_as_deref(intrin->src[0]);
   /* When we see a volatile store, we go ahead and combine all
   * previous non-volatile stores which touch that address and
   * specifically don't add the volatile store to the list.  This
   * way we guarantee that the volatile store isn't combined with
   * anything and no non-volatile stores are combined across a
   * volatile store.
   */
      } else {
                     case nir_intrinsic_barrier:
      if (nir_intrinsic_memory_semantics(intrin) & NIR_MEMORY_RELEASE) {
      combine_stores_with_modes(state,
                  case nir_intrinsic_emit_vertex:
   case nir_intrinsic_emit_vertex_with_counter:
                  case nir_intrinsic_report_ray_intersection:
      combine_stores_with_modes(state, nir_var_mem_ssbo |
                           case nir_intrinsic_ignore_ray_intersection:
   case nir_intrinsic_terminate_ray:
      combine_stores_with_modes(state, nir_var_mem_ssbo |
                     case nir_intrinsic_load_deref: {
      nir_deref_instr *src = nir_src_as_deref(intrin->src[0]);
   combine_stores_with_deref(state, src);
               case nir_intrinsic_load_deref_block_intel:
   case nir_intrinsic_store_deref_block_intel: {
      /* Combine all the stores that may alias with the whole variable (or
   * cast).
   */
   nir_deref_instr *operand = nir_src_as_deref(intrin->src[0]);
   while (nir_deref_instr_parent(operand))
                        combine_stores_with_deref(state, operand);
               case nir_intrinsic_copy_deref:
   case nir_intrinsic_memcpy_deref: {
      nir_deref_instr *dst = nir_src_as_deref(intrin->src[0]);
   nir_deref_instr *src = nir_src_as_deref(intrin->src[1]);
   combine_stores_with_deref(state, dst);
   combine_stores_with_deref(state, src);
               case nir_intrinsic_trace_ray:
   case nir_intrinsic_execute_callable:
   case nir_intrinsic_rt_trace_ray:
   case nir_intrinsic_rt_execute_callable: {
      nir_deref_instr *payload =
         combine_stores_with_deref(state, payload);
               case nir_intrinsic_deref_atomic:
   case nir_intrinsic_deref_atomic_swap: {
      nir_deref_instr *dst = nir_src_as_deref(intrin->src[0]);
   combine_stores_with_deref(state, dst);
               default:
                     /* At the end of the block, try all the remaining combinations. */
      }
      static bool
   combine_stores_impl(struct combine_stores_state *state, nir_function_impl *impl)
   {
      state->progress = false;
            nir_foreach_block(block, impl)
            if (state->progress) {
      nir_metadata_preserve(impl, nir_metadata_block_index |
      } else {
                     }
      bool
   nir_opt_combine_stores(nir_shader *shader, nir_variable_mode modes)
   {
      void *mem_ctx = ralloc_context(NULL);
   struct combine_stores_state state = {
      .modes = modes,
               list_inithead(&state.pending);
                     nir_foreach_function_impl(impl, shader) {
                  ralloc_free(mem_ctx);
      }
