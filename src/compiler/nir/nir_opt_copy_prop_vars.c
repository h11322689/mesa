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
   #include "nir_deref.h"
      #include "util/bitscan.h"
   #include "util/u_dynarray.h"
      static const bool debug = false;
      /**
   * Variable-based copy propagation
   *
   * Normally, NIR trusts in SSA form for most of its copy-propagation needs.
   * However, there are cases, especially when dealing with indirects, where SSA
   * won't help you.  This pass is for those times.  Specifically, it handles
   * the following things that the rest of NIR can't:
   *
   *  1) Copy-propagation on variables that have indirect access.  This includes
   *     propagating from indirect stores into indirect loads.
   *
   *  2) Removal of redundant load_deref intrinsics.  We can't trust regular CSE
   *     to do this because it isn't aware of variable writes that may alias the
   *     value and make the former load invalid.
   *
   * This pass uses an intermediate solution between being local / "per-block"
   * and a complete data-flow analysis.  It follows the control flow graph, and
   * propagate the available copy information forward, invalidating data at each
   * cf_node.
   *
   * Removal of dead writes to variables is handled by another pass.
   */
      struct copies {
               /* Hash table of copies referenced by variables */
            /* Array of derefs that can't be chased back to a variable */
      };
      struct copies_dynarray {
      struct list_head node;
            /* The copies structure this dynarray was cloned or created for */
      };
      struct vars_written {
               /* Key is deref and value is the uintptr_t with the write mask. */
      };
      struct value {
      bool is_ssa;
   union {
      struct {
      nir_def *def[NIR_MAX_VEC_COMPONENTS];
      } ssa;
         };
      static void
   value_set_ssa_components(struct value *value, nir_def *def,
         {
      value->is_ssa = true;
   for (unsigned i = 0; i < num_components; i++) {
      value->ssa.def[i] = def;
         }
      struct copy_entry {
                  };
      struct copy_prop_var_state {
               void *mem_ctx;
            /* Maps nodes to vars_written.  Used to invalidate copy entries when
   * visiting each node.
   */
            /* List of copy structures ready for reuse */
               };
      static bool
   value_equals_store_src(struct value *value, nir_intrinsic_instr *intrin)
   {
      assert(intrin->intrinsic == nir_intrinsic_store_deref);
            for (unsigned i = 0; i < intrin->num_components; i++) {
      if ((write_mask & (1 << i)) &&
      (value->ssa.def[i] != intrin->src[1].ssa ||
   value->ssa.component[i] != i))
               }
      static struct vars_written *
   create_vars_written(struct copy_prop_var_state *state)
   {
      struct vars_written *written =
         written->derefs = _mesa_pointer_hash_table_create(state->mem_ctx);
      }
      static void
   gather_vars_written(struct copy_prop_var_state *state,
               {
               switch (cf_node->type) {
   case nir_cf_node_function: {
      nir_function_impl *impl = nir_cf_node_as_function(cf_node);
   foreach_list_typed_safe(nir_cf_node, cf_node, node, &impl->body)
                     case nir_cf_node_block: {
      if (!written)
            nir_block *block = nir_cf_node_as_block(cf_node);
   nir_foreach_instr(instr, block) {
      if (instr->type == nir_instr_type_call) {
      written->modes |= nir_var_shader_out |
                     nir_var_shader_temp |
   nir_var_function_temp |
                              nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(instr);
   switch (intrin->intrinsic) {
   case nir_intrinsic_barrier:
      if (nir_intrinsic_memory_semantics(intrin) & NIR_MEMORY_ACQUIRE)
               case nir_intrinsic_emit_vertex:
   case nir_intrinsic_emit_vertex_with_counter:
                  case nir_intrinsic_trace_ray:
   case nir_intrinsic_execute_callable:
   case nir_intrinsic_rt_trace_ray:
   case nir_intrinsic_rt_execute_callable: {
                              struct hash_entry *ht_entry =
         if (ht_entry) {
         } else {
      _mesa_hash_table_insert(written->derefs, payload,
      }
               case nir_intrinsic_report_ray_intersection:
      written->modes |= nir_var_mem_ssbo |
                           case nir_intrinsic_ignore_ray_intersection:
   case nir_intrinsic_terminate_ray:
      written->modes |= nir_var_mem_ssbo |
                     case nir_intrinsic_deref_atomic:
   case nir_intrinsic_deref_atomic_swap:
   case nir_intrinsic_store_deref:
   case nir_intrinsic_copy_deref:
   case nir_intrinsic_memcpy_deref: {
                              struct hash_entry *ht_entry = _mesa_hash_table_search(written->derefs, dst);
   if (ht_entry)
                                    default:
                                 case nir_cf_node_if: {
                        foreach_list_typed_safe(nir_cf_node, cf_node, node, &if_stmt->then_list)
            foreach_list_typed_safe(nir_cf_node, cf_node, node, &if_stmt->else_list)
                        case nir_cf_node_loop: {
      nir_loop *loop = nir_cf_node_as_loop(cf_node);
                     foreach_list_typed_safe(nir_cf_node, cf_node, node, &loop->body)
                        default:
                  if (new_written) {
      /* Merge new information to the parent control flow node. */
   if (written) {
      written->modes |= new_written->modes;
   hash_table_foreach(new_written->derefs, new_entry) {
      struct hash_entry *old_entry =
      _mesa_hash_table_search_pre_hashed(written->derefs, new_entry->hash,
      if (old_entry) {
      nir_component_mask_t merged = (uintptr_t)new_entry->data |
            } else {
      _mesa_hash_table_insert_pre_hashed(written->derefs, new_entry->hash,
            }
         }
      /* Creates a fresh dynarray */
   static struct copies_dynarray *
   get_copies_dynarray(struct copy_prop_var_state *state)
   {
      struct copies_dynarray *cp_arr =
         util_dynarray_init(&cp_arr->arr, state->mem_ctx);
      }
      /* Checks if the pointer leads to a cloned copy of the array for this hash
   * table or if the pointer was inherited from when the hash table was cloned.
   */
   static bool
   copies_owns_ht_entry(struct copies *copies,
         {
      assert(copies && ht_entry && ht_entry->data);
   struct copies_dynarray *copies_array = ht_entry->data;
      }
      static void
   clone_copies_dynarray_from_src(struct copies_dynarray *dst,
         {
         }
      /* Gets copies array from the hash table entry or clones the source array if
   * the hash entry contains NULL. The values are not cloned when the hash table
   * is created because its expensive to clone everything and most value will
   * never actually be accessed.
   */
   static struct copies_dynarray *
   get_copies_array_from_ht_entry(struct copy_prop_var_state *state,
               {
      struct copies_dynarray *copies_array;
   if (copies_owns_ht_entry(copies, ht_entry)) {
      /* The array already exists so just return it */
      } else {
      /* Clone the array and set the data value for future access */
   copies_array = get_copies_dynarray(state);
   copies_array->owner = copies;
   clone_copies_dynarray_from_src(copies_array, ht_entry->data);
                  }
      static struct copies_dynarray *
   copies_array_for_var(struct copy_prop_var_state *state,
         {
      struct hash_entry *entry = _mesa_hash_table_search(copies->ht, var);
   if (entry != NULL)
            struct copies_dynarray *copies_array = get_copies_dynarray(state);
   copies_array->owner = copies;
               }
      static struct util_dynarray *
   copies_array_for_deref(struct copy_prop_var_state *state,
         {
               struct util_dynarray *copies_array;
   if (deref->_path->path[0]->deref_type != nir_deref_type_var) {
         } else {
      struct copies_dynarray *cpda =
                        }
      static struct copy_entry *
   copy_entry_create(struct copy_prop_var_state *state,
         {
      struct util_dynarray *copies_array =
            struct copy_entry new_entry = {
         };
   util_dynarray_append(copies_array, struct copy_entry, new_entry);
      }
      /* Remove copy entry by swapping it with the last element and reducing the
   * size.  If used inside an iteration on copies, it must be a reverse
   * (backwards) iteration.  It is safe to use in those cases because the swap
   * will not affect the rest of the iteration.
   */
   static void
   copy_entry_remove(struct util_dynarray *copies,
               {
      const struct copy_entry *src =
            /* Because we're removing elements from an array, pointers to those
   * elements are not stable as we modify the array.
   * If relocated_entry != NULL, it's points to an entry we saved off earlier
   * and want to keep pointing to the right spot.
   */
   if (relocated_entry && *relocated_entry == src)
            if (src != entry)
      }
      static bool
   is_array_deref_of_vector(const nir_deref_and_path *deref)
   {
      if (deref->instr->deref_type != nir_deref_type_array)
         nir_deref_instr *parent = nir_deref_instr_parent(deref->instr);
      }
      static struct copy_entry *
   lookup_entry_for_deref(struct copy_prop_var_state *state,
                           {
      struct util_dynarray *copies_array =
            struct copy_entry *entry = NULL;
   util_dynarray_foreach(copies_array, struct copy_entry, iter) {
      nir_deref_compare_result result =
         if (result & allowed_comparisons) {
      entry = iter;
   if (result & nir_derefs_equal_bit) {
      if (equal != NULL)
            }
                     }
      static void
   lookup_entry_and_kill_aliases_copy_array(struct copy_prop_var_state *state,
                                       {
      util_dynarray_foreach_reverse(copies_array, struct copy_entry, iter) {
      nir_deref_compare_result comp =
            if (comp & nir_derefs_equal_bit) {
      /* Make sure it is unique. */
   assert(!*entry && !*entry_removed);
   if (remove_entry) {
      copy_entry_remove(copies_array, iter, NULL);
      } else {
            } else if (comp & nir_derefs_may_alias_bit) {
               }
      static struct copy_entry *
   lookup_entry_and_kill_aliases(struct copy_prop_var_state *state,
                           {
               bool UNUSED entry_removed = false;
                     /* For any other variable types if the variables are different,
   * they don't alias. So we only need to compare different vars and loop
   * over the hash table for ssbos and shared vars.
   */
   if (deref->_path->path[0]->deref_type != nir_deref_type_var ||
      deref->_path->path[0]->var->data.mode == nir_var_mem_ssbo ||
            hash_table_foreach(copies->ht, ht_entry) {
      nir_variable *var = (nir_variable *)ht_entry->key;
   if (deref->_path->path[0]->deref_type == nir_deref_type_var &&
                                 lookup_entry_and_kill_aliases_copy_array(state, &copies_array->arr,
                        if (copies_array->arr.size == 0) {
                     lookup_entry_and_kill_aliases_copy_array(state, &copies->arr, deref,
            } else {
      struct copies_dynarray *cpda =
                  lookup_entry_and_kill_aliases_copy_array(state, copies_array, deref,
                  if (copies_array->size == 0) {
                        }
      static void
   kill_aliases(struct copy_prop_var_state *state,
               struct copies *copies,
   {
                  }
      static struct copy_entry *
   get_entry_and_kill_aliases(struct copy_prop_var_state *state,
                     {
               struct copy_entry *entry =
         if (entry == NULL)
               }
      static void
   apply_barrier_for_modes_to_dynarr(struct util_dynarray *copies_array,
         {
      util_dynarray_foreach_reverse(copies_array, struct copy_entry, iter) {
      if (nir_deref_mode_may_be(iter->dst.instr, modes) ||
      (!iter->src.is_ssa && nir_deref_mode_may_be(iter->src.deref.instr, modes)))
      }
      static void
   apply_barrier_for_modes(struct copy_prop_var_state *state,
         {
      hash_table_foreach(copies->ht, ht_entry) {
      struct copies_dynarray *copies_array =
                           }
      static void
   value_set_from_value(struct value *value, const struct value *from,
         {
      /* We can't have non-zero indexes with non-trivial write masks */
            if (from->is_ssa) {
      /* Clear value if it was being used as non-SSA. */
   value->is_ssa = true;
   /* Only overwrite the written components */
   for (unsigned i = 0; i < NIR_MAX_VEC_COMPONENTS; i++) {
      if (write_mask & (1 << i)) {
      value->ssa.def[base_index + i] = from->ssa.def[i];
            } else {
      /* Non-ssa stores always write everything */
   value->is_ssa = false;
         }
      /* Try to load a single element of a vector from the copy_entry.  If the data
   * isn't available, just let the original intrinsic do the work.
   */
   static bool
   load_element_from_ssa_entry_value(struct copy_prop_var_state *state,
                     {
               /* We don't have the element available, so let the instruction do the work. */
   if (!entry->src.ssa.def[index])
            b->cursor = nir_instr_remove(&intrin->instr);
            assert(entry->src.ssa.component[index] <
         nir_def *def = nir_channel(b, entry->src.ssa.def[index],
            *value = (struct value){
      .is_ssa = true,
   {
      .ssa = {
      .def = { def },
                        }
      /* Do a "load" from an SSA-based entry return it in "value" as a value with a
   * single SSA def.  Because an entry could reference multiple different SSA
   * defs, a vecN operation may be inserted to combine them into a single SSA
   * def before handing it back to the caller.  If the load instruction is no
   * longer needed, it is removed and nir_instr::block is set to NULL.  (It is
   * possible, in some cases, for the load to be used in the vecN operation in
   * which case it isn't deleted.)
   */
   static bool
   load_from_ssa_entry_value(struct copy_prop_var_state *state,
                     {
      if (is_array_deref_of_vector(src)) {
      if (nir_src_is_const(src->instr->arr.index)) {
      unsigned index = nir_src_as_uint(src->instr->arr.index);
   return load_element_from_ssa_entry_value(state, entry, b, intrin,
               /* An SSA copy_entry for the vector won't help indirect load. */
   if (glsl_type_is_vector(entry->dst.instr->type)) {
      assert(entry->dst.instr->type == nir_deref_instr_parent(src->instr)->type);
   /* TODO: If all SSA entries are there, try an if-ladder. */
                           const struct glsl_type *type = entry->dst.instr->type;
            nir_component_mask_t available = 0;
   bool all_same = true;
   for (unsigned i = 0; i < num_components; i++) {
      if (value->ssa.def[i])
            if (value->ssa.def[i] != value->ssa.def[0])
            if (value->ssa.component[i] != i)
               if (all_same) {
      /* Our work here is done */
   b->cursor = nir_instr_remove(&intrin->instr);
   intrin->instr.block = NULL;
               if (available != (1 << num_components) - 1 &&
      intrin->intrinsic == nir_intrinsic_load_deref &&
   (available & nir_def_components_read(&intrin->def)) == 0) {
   /* If none of the components read are available as SSA values, then we
   * should just bail.  Otherwise, we would end up replacing the uses of
   * the load_deref a vecN() that just gathers up its components.
   */
                        nir_def *load_def =
            bool keep_intrin = false;
   nir_scalar comps[NIR_MAX_VEC_COMPONENTS];
   for (unsigned i = 0; i < num_components; i++) {
      if (value->ssa.def[i]) {
         } else {
      /* We don't have anything for this component in our
   * list.  Just re-use a channel from the load.
   */
                                                nir_def *vec = nir_vec_scalars(b, comps, num_components);
            if (!keep_intrin) {
      /* Removing this instruction should not touch the cursor because we
   * created the cursor after the intrinsic and have added at least one
   * instruction (the vec) since then.
   */
   assert(b->cursor.instr != &intrin->instr);
   nir_instr_remove(&intrin->instr);
                  }
      /**
   * Specialize the wildcards in a deref chain
   *
   * This function returns a deref chain identical to \param deref except that
   * some of its wildcards are replaced with indices from \param specific.  The
   * process is guided by \param guide which references the same type as \param
   * specific but has the same wildcard array lengths as \param deref.
   */
   static nir_deref_instr *
   specialize_wildcards(nir_builder *b,
                     {
      nir_deref_instr **deref_p = &deref->path[1];
   nir_deref_instr *ret_tail = deref->path[0];
   for (; *deref_p; deref_p++) {
      if ((*deref_p)->deref_type == nir_deref_type_array_wildcard)
                     nir_deref_instr **guide_p = &guide->path[1];
   nir_deref_instr **spec_p = &specific->path[1];
   for (; *deref_p; deref_p++) {
      if ((*deref_p)->deref_type == nir_deref_type_array_wildcard) {
      /* This is where things get tricky.  We have to search through
   * the entry deref to find its corresponding wildcard and fill
   * this slot in with the value from the src.
   */
   while (*guide_p &&
            guide_p++;
                              guide_p++;
      } else {
                        }
      /* Do a "load" from an deref-based entry return it in "value" as a value.  The
   * deref returned in "value" will always be a fresh copy so the caller can
   * steal it and assign it to the instruction directly without copying it
   * again.
   */
   static bool
   load_from_deref_entry_value(struct copy_prop_var_state *state,
                     {
                        nir_deref_path *entry_dst_path = nir_get_deref_path(state->mem_ctx, &entry->dst);
            bool need_to_specialize_wildcards = false;
   nir_deref_instr **entry_p = &entry_dst_path->path[1];
   nir_deref_instr **src_p = &src_path->path[1];
   while (*entry_p && *src_p) {
      nir_deref_instr *entry_tail = *entry_p++;
            if (src_tail->deref_type == nir_deref_type_array &&
      entry_tail->deref_type == nir_deref_type_array_wildcard)
            /* If the entry deref is longer than the source deref then it refers to a
   * smaller type and we can't source from it.
   */
                     if (need_to_specialize_wildcards) {
      /* The entry has some wildcards that are not in src.  This means we need
   * to construct a new deref based on the entry but using the wildcards
   * from the source and guided by the entry dst.  Oof.
   */
   nir_deref_path *entry_src_path =
         value->deref.instr = specialize_wildcards(b, entry_src_path,
               /* If our source deref is longer than the entry deref, that's ok because
   * it just means the entry deref needs to be extended a bit.
   */
   while (*src_p) {
      nir_deref_instr *src_tail = *src_p++;
                  }
      static bool
   try_load_from_entry(struct copy_prop_var_state *state, struct copy_entry *entry,
               {
      if (entry == NULL)
            if (entry->src.is_ssa) {
         } else {
            }
      static void
   invalidate_copies_for_cf_node(struct copy_prop_var_state *state,
               {
      struct hash_entry *ht_entry = _mesa_hash_table_search(state->vars_written_map, cf_node);
            struct vars_written *written = ht_entry->data;
   if (written->modes) {
      hash_table_foreach(copies->ht, ht_entry) {
                     util_dynarray_foreach_reverse(&copies_array->arr, struct copy_entry, entry) {
      if (nir_deref_mode_may_be(entry->dst.instr, written->modes))
               if (copies_array->arr.size == 0) {
                     util_dynarray_foreach_reverse(&copies->arr, struct copy_entry, entry) {
      if (nir_deref_mode_may_be(entry->dst.instr, written->modes))
                  hash_table_foreach(written->derefs, entry) {
      nir_deref_instr *deref_written = (nir_deref_instr *)entry->key;
   nir_deref_and_path deref = { deref_written, NULL };
         }
      static void
   print_value(struct value *value, unsigned num_components)
   {
      bool same_ssa = true;
   for (unsigned i = 0; i < num_components; i++) {
      if (value->ssa.component[i] != i ||
      (i > 0 && value->ssa.def[i - 1] != value->ssa.def[i])) {
   same_ssa = false;
         }
   if (same_ssa) {
         } else {
      for (int i = 0; i < num_components; i++) {
      if (value->ssa.def[i])
         else
            }
      static void
   print_copy_entry(struct copy_entry *entry)
   {
      printf("    %s ", glsl_get_type_name(entry->dst.instr->type));
   nir_print_deref(entry->dst.instr, stdout);
            unsigned num_components = glsl_get_vector_elements(entry->dst.instr->type);
   print_value(&entry->src, num_components);
      }
      static void
   dump_instr(nir_instr *instr)
   {
      printf("  ");
   nir_print_instr(instr, stdout);
      }
      static void
   dump_copy_entries(struct copies *copies)
   {
      hash_table_foreach(copies->ht, ht_entry) {
      struct util_dynarray *copies_array =
            util_dynarray_foreach(copies_array, struct copy_entry, iter)
               util_dynarray_foreach(&copies->arr, struct copy_entry, iter)
               }
      static void
   copy_prop_vars_block(struct copy_prop_var_state *state,
               {
      if (debug) {
      printf("# block%d\n", block->index);
               nir_foreach_instr_safe(instr, block) {
      if (debug && instr->type == nir_instr_type_deref)
            if (instr->type == nir_instr_type_call) {
      if (debug)
         apply_barrier_for_modes(state, copies, nir_var_shader_out | nir_var_shader_temp | nir_var_function_temp | nir_var_mem_ssbo | nir_var_mem_shared | nir_var_mem_global);
   if (debug)
                     if (instr->type != nir_instr_type_intrinsic)
            nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(instr);
   switch (intrin->intrinsic) {
   case nir_intrinsic_barrier:
                     if (nir_intrinsic_memory_semantics(intrin) & NIR_MEMORY_ACQUIRE)
               case nir_intrinsic_emit_vertex:
   case nir_intrinsic_emit_vertex_with_counter:
                                 case nir_intrinsic_report_ray_intersection:
                  case nir_intrinsic_ignore_ray_intersection:
   case nir_intrinsic_terminate_ray:
                  case nir_intrinsic_load_deref: {
                                             /* If this is a load from a read-only mode, then all this pass would
   * do is combine redundant loads and CSE should be more efficient for
   * that.
   */
   nir_variable_mode ignore = nir_var_read_only_modes & ~nir_var_vec_indexable_modes;
                  /* Ignore trivial casts. If trivial casts are applied to array derefs of vectors,
   * not doing this causes is_array_deref_of_vector to (wrongly) return false. */
   while (src.instr->deref_type == nir_deref_type_cast &&
                  /* Direct array_derefs of vectors operate on the vectors (the parent
   * deref).  Indirects will be handled like other derefs.
   */
   int vec_index = 0;
   nir_deref_and_path vec_src = src;
   if (is_array_deref_of_vector(&src) && nir_src_is_const(src.instr->arr.index)) {
      vec_src.instr = nir_deref_instr_parent(src.instr);
                  /* Loading from an invalid index yields an undef */
   if (vec_index >= vec_comps) {
      b->cursor = nir_instr_remove(instr);
   nir_def *u = nir_undef(b, 1, intrin->def.bit_size);
   nir_def_rewrite_uses(&intrin->def, u);
   state->progress = true;
                  bool src_entry_equal = false;
   struct copy_entry *src_entry =
      lookup_entry_for_deref(state, copies, &src,
      struct value value = { 0 };
   if (try_load_from_entry(state, src_entry, b, intrin, &src, &value)) {
      if (value.is_ssa) {
      /* lookup_load has already ensured that we get a single SSA
   * value that has all of the channels.  We just have to do the
   * rewrite operation.  Note for array derefs of vectors, the
   * channel 0 is used.
   */
   if (intrin->instr.block) {
      /* The lookup left our instruction in-place.  This means it
   * must have used it to vec up a bunch of different sources.
   * We need to be careful when rewriting uses so we don't
   * rewrite the vecN itself.
   */
   nir_def_rewrite_uses_after(&intrin->def,
            } else {
      nir_def_rewrite_uses(&intrin->def,
         } else {
                     /* Put it back in again. */
   nir_builder_instr_insert(b, instr);
   value_set_ssa_components(&value, &intrin->def,
      }
      } else {
      value_set_ssa_components(&value, &intrin->def,
               /* Now that we have a value, we're going to store it back so that we
   * have the right value next time we come looking for it.  In order
   * to do this, we need an exact match, not just something that
   * contains what we're looking for.
   *
   * We avoid doing another lookup if src.instr == vec_src.instr.
   */
   struct copy_entry *entry = src_entry;
   if (src.instr != vec_src.instr)
      entry = lookup_entry_for_deref(state, copies, &vec_src,
                                    /* Update the entry with the value of the load.  This way
   * we can potentially remove subsequent loads.
   */
   value_set_from_value(&entry->src, &value, vec_index,
                     case nir_intrinsic_store_deref: {
                                    /* Ignore trivial casts. If trivial casts are applied to array derefs of vectors,
   * not doing this causes is_array_deref_of_vector to (wrongly) return false. */
   while (dst.instr->deref_type == nir_deref_type_cast &&
                  /* Direct array_derefs of vectors operate on the vectors (the parent
   * deref).  Indirects will be handled like other derefs.
   */
   int vec_index = 0;
   nir_deref_and_path vec_dst = dst;
   if (is_array_deref_of_vector(&dst) && nir_src_is_const(dst.instr->arr.index)) {
                              /* Storing to an invalid index is a no-op. */
   if (vec_index >= vec_comps) {
      nir_instr_remove(instr);
   state->progress = true;
                  if (nir_intrinsic_access(intrin) & ACCESS_VOLATILE) {
      unsigned wrmask = nir_intrinsic_write_mask(intrin);
   kill_aliases(state, copies, &dst, wrmask);
               struct copy_entry *entry =
         if (entry && value_equals_store_src(&entry->src, intrin)) {
      /* If we are storing the value from a load of the same var the
   * store is redundant so remove it.
   */
   nir_instr_remove(instr);
      } else {
      struct value value = { 0 };
   value_set_ssa_components(&value, intrin->src[1].ssa,
         unsigned wrmask = nir_intrinsic_write_mask(intrin);
   struct copy_entry *entry =
                                 case nir_intrinsic_copy_deref: {
                                    /* The copy_deref intrinsic doesn't keep track of num_components, so
   * get it ourselves.
   */
                  if ((nir_intrinsic_src_access(intrin) & ACCESS_VOLATILE) ||
      (nir_intrinsic_dst_access(intrin) & ACCESS_VOLATILE)) {
   kill_aliases(state, copies, &dst, full_mask);
               nir_deref_compare_result comp =
         if (comp & nir_derefs_equal_bit) {
      /* This is a no-op self-copy.  Get rid of it */
   nir_instr_remove(instr);
   state->progress = true;
               /* Copy of direct array derefs of vectors are not handled.  Just
   * invalidate what's written and bail.
   */
   if ((is_array_deref_of_vector(&src) && nir_src_is_const(src.instr->arr.index)) ||
      (is_array_deref_of_vector(&dst) && nir_src_is_const(dst.instr->arr.index))) {
   kill_aliases(state, copies, &dst, full_mask);
               struct copy_entry *src_entry =
         struct value value;
   if (try_load_from_entry(state, src_entry, b, intrin, &src, &value)) {
      /* If load works, intrin (the copy_deref) is removed. */
   if (value.is_ssa) {
         } else {
      /* If this would be a no-op self-copy, don't bother. */
                                                            } else {
      value = (struct value){
      .is_ssa = false,
                  nir_variable *src_var = nir_deref_instr_get_variable(src.instr);
   if (src_var && src_var->data.cannot_coalesce) {
      /* The source cannot be coaleseced, which means we can't propagate
   * this copy.
   */
               struct copy_entry *dst_entry =
         value_set_from_value(&dst_entry->src, &value, 0, full_mask);
               case nir_intrinsic_trace_ray:
   case nir_intrinsic_execute_callable:
   case nir_intrinsic_rt_trace_ray:
   case nir_intrinsic_rt_execute_callable: {
                     nir_deref_and_path payload = {
         };
   nir_component_mask_t full_mask = (1 << glsl_get_vector_elements(payload.instr->type)) - 1;
   kill_aliases(state, copies, &payload, full_mask);
               case nir_intrinsic_memcpy_deref:
   case nir_intrinsic_deref_atomic:
   case nir_intrinsic_deref_atomic_swap:
                     nir_deref_and_path dst = { nir_src_as_deref(intrin->src[0]), NULL };
   unsigned num_components = glsl_get_vector_elements(dst.instr->type);
   unsigned full_mask = (1 << num_components) - 1;
               case nir_intrinsic_store_deref_block_intel: {
                     /* Invalidate the whole variable (or cast) and anything that alias
   * with it.
   */
   nir_deref_and_path dst = { nir_src_as_deref(intrin->src[0]), NULL };
   while (nir_deref_instr_parent(dst.instr))
                        unsigned num_components = glsl_get_vector_elements(dst.instr->type);
   unsigned full_mask = (1 << num_components) - 1;
   kill_aliases(state, copies, &dst, full_mask);
               default:
                  if (debug)
         }
      static void
   clone_copies(struct copy_prop_var_state *state, struct copies *clones,
         {
      /* Simply clone the entire hash table. This is much faster than trying to
   * rebuild it and is needed to avoid slow compilation of very large shaders.
   * If needed we will clone the data later if it is ever looked up.
   */
   assert(clones->ht == NULL);
               }
      /* Returns an existing struct for reuse or creates a new on if they are
   * all in use. This greatly reduces the time spent allocating memory if we
   * were to just creating a fresh one each time.
   */
   static struct copies *
   get_copies_structure(struct copy_prop_var_state *state)
   {
      struct copies *copies;
   if (list_is_empty(&state->unused_copy_structs_list)) {
      copies = ralloc(state->mem_ctx, struct copies);
   copies->ht = NULL;
      } else {
      copies = list_entry(state->unused_copy_structs_list.next,
                        }
      static void
   clear_copies_structure(struct copy_prop_var_state *state,
         {
      ralloc_free(copies->ht);
               }
      static void
   copy_prop_vars_cf_node(struct copy_prop_var_state *state,
         {
      switch (cf_node->type) {
   case nir_cf_node_function: {
               struct copies *impl_copies = get_copies_structure(state);
   impl_copies->ht = _mesa_hash_table_create(state->mem_ctx,
                  foreach_list_typed_safe(nir_cf_node, cf_node, node, &impl->body)
                                 case nir_cf_node_block: {
      nir_block *block = nir_cf_node_as_block(cf_node);
   nir_builder b = nir_builder_create(state->impl);
   copy_prop_vars_block(state, &b, block, copies);
               case nir_cf_node_if: {
               /* Create new hash tables for tracking vars and fill it with clones of
   * the copy arrays for each variable we are tracking.
   *
   * We clone the copies for each branch of the if statement.  The idea is
   * that they both see the same state of available copies, but do not
   * interfere to each other.
   */
   if (!exec_list_is_empty(&if_stmt->then_list)) {
                                                if (!exec_list_is_empty(&if_stmt->else_list)) {
                                                /* Both branches copies can be ignored, since the effect of running both
   * branches was captured in the first pass that collects vars_written.
                                 case nir_cf_node_loop: {
      nir_loop *loop = nir_cf_node_as_loop(cf_node);
            /* Invalidate before cloning the copies for the loop, since the loop
   * body can be executed more than once.
                     struct copies *loop_copies = get_copies_structure(state);
            foreach_list_typed_safe(nir_cf_node, cf_node, node, &loop->body)
                                 default:
            }
      static bool
   nir_copy_prop_vars_impl(nir_function_impl *impl)
   {
               if (debug) {
      nir_metadata_require(impl, nir_metadata_block_index);
               struct copy_prop_var_state state = {
      .impl = impl,
   .mem_ctx = mem_ctx,
               };
                              if (state.progress) {
      nir_metadata_preserve(impl, nir_metadata_block_index |
      } else {
                  ralloc_free(mem_ctx);
      }
      bool
   nir_opt_copy_prop_vars(nir_shader *shader)
   {
               nir_foreach_function_impl(impl, shader) {
                     }
