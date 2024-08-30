   /*
   * Copyright © 2018 Intel Corporation
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
      struct match_node {
               unsigned next_array_idx;
   int src_wildcard_idx;
            /* The index of the first read of the source path that's part of the copy
   * we're matching. If the last write to the source path is after this, we
   * would get a different result from reading it at the end and we can't
   * emit the copy.
   */
            /* The last time there was a write to this node. */
            /* The last time there was a write to this node which successfully advanced
   * next_array_idx. This helps us catch any intervening aliased writes.
   */
            unsigned num_children;
      };
      struct match_state {
      /* Map from nir_variable * -> match_node */
   struct hash_table *var_nodes;
   /* Map from cast nir_deref_instr * -> match_node */
                                 };
      static struct match_node *
   create_match_node(const struct glsl_type *type, struct match_state *state)
   {
      unsigned num_children = 0;
   if (glsl_type_is_array_or_matrix(type)) {
      /* One for wildcards */
      } else if (glsl_type_is_struct_or_ifc(type)) {
                  struct match_node *node = rzalloc_size(state->dead_ctx,
               node->num_children = num_children;
   node->src_wildcard_idx = -1;
   node->first_src_read = UINT32_MAX;
      }
      static struct match_node *
   node_for_deref(nir_deref_instr *instr, struct match_node *parent,
         {
      unsigned idx;
   switch (instr->deref_type) {
   case nir_deref_type_var: {
      struct hash_entry *entry =
         if (entry) {
         } else {
      struct match_node *node = create_match_node(instr->type, state);
   _mesa_hash_table_insert(state->var_nodes, instr->var, node);
                  case nir_deref_type_cast: {
      struct hash_entry *entry =
         if (entry) {
         } else {
      struct match_node *node = create_match_node(instr->type, state);
   _mesa_hash_table_insert(state->cast_nodes, instr, node);
                  case nir_deref_type_array_wildcard:
      idx = parent->num_children - 1;
         case nir_deref_type_array:
      if (nir_src_is_const(instr->arr.index)) {
      idx = nir_src_as_uint(instr->arr.index);
      } else {
         }
         case nir_deref_type_struct:
      idx = instr->strct.index;
         default:
                  assert(idx < parent->num_children);
   if (parent->children[idx]) {
         } else {
      struct match_node *node = create_match_node(instr->type, state);
   parent->children[idx] = node;
         }
      static struct match_node *
   node_for_wildcard(const struct glsl_type *type, struct match_node *parent,
         {
      assert(glsl_type_is_array_or_matrix(type));
            if (parent->children[idx]) {
         } else {
      struct match_node *node =
         parent->children[idx] = node;
         }
      static struct match_node *
   node_for_path(nir_deref_path *path, struct match_state *state)
   {
      struct match_node *node = NULL;
   for (nir_deref_instr **instr = path->path; *instr; instr++)
               }
      static struct match_node *
   node_for_path_with_wildcard(nir_deref_path *path, unsigned wildcard_idx,
         {
      struct match_node *node = NULL;
   unsigned idx = 0;
   for (nir_deref_instr **instr = path->path; *instr; instr++, idx++) {
      if (idx == wildcard_idx)
         else
                  }
      typedef void (*match_cb)(struct match_node *, struct match_state *);
      static void
   _foreach_child(match_cb cb, struct match_node *node, struct match_state *state)
   {
      if (node->num_children == 0) {
         } else {
      for (unsigned i = 0; i < node->num_children; i++) {
      if (node->children[i])
            }
      static void
   _foreach_aliasing(nir_deref_instr **deref, match_cb cb,
         {
      if (*deref == NULL) {
      cb(node, state);
               switch ((*deref)->deref_type) {
   case nir_deref_type_struct: {
      struct match_node *child = node->children[(*deref)->strct.index];
   if (child)
                     case nir_deref_type_array:
   case nir_deref_type_array_wildcard: {
      if ((*deref)->deref_type == nir_deref_type_array_wildcard ||
      !nir_src_is_const((*deref)->arr.index)) {
   /* This access may touch any index, so we have to visit all of
   * them.
   */
   for (unsigned i = 0; i < node->num_children; i++) {
      if (node->children[i])
         } else {
      /* Visit the wildcard entry if any */
   if (node->children[node->num_children - 1]) {
      _foreach_aliasing(deref + 1, cb,
               unsigned index = nir_src_as_uint((*deref)->arr.index);
   /* Check that the index is in-bounds */
   if (index < node->num_children - 1 && node->children[index])
      }
               case nir_deref_type_cast:
      _foreach_child(cb, node, state);
         default:
            }
      /* Given a deref path, find all the leaf deref nodes that alias it. */
      static void
   foreach_aliasing_node(nir_deref_path *path,
               {
      if (path->path[0]->deref_type == nir_deref_type_var) {
      struct hash_entry *entry = _mesa_hash_table_search(state->var_nodes,
         if (entry)
            hash_table_foreach(state->cast_nodes, entry)
      } else {
      /* Casts automatically alias anything that isn't a cast */
   assert(path->path[0]->deref_type == nir_deref_type_cast);
   hash_table_foreach(state->var_nodes, entry)
            /* Casts alias other casts if the casts are different or if they're the
   * same and the path from the cast may alias as per the usual rules.
   */
   hash_table_foreach(state->cast_nodes, entry) {
      const nir_deref_instr *cast = entry->key;
   assert(cast->deref_type == nir_deref_type_cast);
   if (cast == path->path[0])
         else
            }
      static nir_deref_instr *
   build_wildcard_deref(nir_builder *b, nir_deref_path *path,
         {
               nir_deref_instr *tail =
            for (unsigned i = wildcard_idx + 1; path->path[i]; i++)
               }
      static void
   clobber(struct match_node *node, struct match_state *state)
   {
         }
      static bool
   try_match_deref(nir_deref_path *base_path, int *path_array_idx,
               {
      for (int i = 0;; i++) {
      nir_deref_instr *b = base_path->path[i];
   nir_deref_instr *d = deref_path->path[i];
   /* They have to be the same length */
   if ((b == NULL) != (d == NULL))
            if (b == NULL)
            /* This can happen if one is a deref_array and the other a wildcard */
   if (b->deref_type != d->deref_type)
                  switch (b->deref_type) {
   case nir_deref_type_var:
      if (b->var != d->var)
               case nir_deref_type_array: {
      const bool const_b_idx = nir_src_is_const(b->arr.index);
   const bool const_d_idx = nir_src_is_const(d->arr.index);
                  /* If we don't have an index into the path yet or if this entry in
   * the path is at the array index, see if this is a candidate.  We're
   * looking for an index which is zero in the base deref and arr_idx
   * in the search deref and has a matching array size.
   */
   if ((*path_array_idx < 0 || *path_array_idx == i) &&
      const_b_idx && b_idx == 0 &&
   const_d_idx && d_idx == arr_idx &&
   glsl_get_length(nir_deref_instr_parent(b)->type) ==
         *path_array_idx = i;
               /* We're at the array index but not a candidate */
                  /* If we're not the path array index, we must match exactly.  We
   * could probably just compare SSA values and trust in copy
   * propagation but doing it ourselves means this pass can run a bit
   * earlier.
   */
   if (b->arr.index.ssa == d->arr.index.ssa ||
                              case nir_deref_type_array_wildcard:
            case nir_deref_type_struct:
      if (b->strct.index != d->strct.index)
               default:
                     /* If we got here without failing, we've matched.  However, it isn't an
   * array match unless we found an altered array index.
   */
      }
      static void
   handle_read(nir_deref_instr *src, struct match_state *state)
   {
      /* We only need to create an entry for sources that might be used to form
   * an array copy. Hence no indirects or indexing into a vector.
   */
   if (nir_deref_instr_has_indirect(src) ||
      nir_deref_instr_is_known_out_of_bounds(src) ||
   (src->deref_type == nir_deref_type_array &&
   glsl_type_is_vector(nir_src_as_deref(src->parent)->type)))
         nir_deref_path src_path;
            /* Create a node for this source if it doesn't exist. The point of this is
   * to know which nodes aliasing a given store we actually need to care
   * about, to avoid creating an excessive amount of nodes.
   */
      }
      /* The core implementation, which is used for both copies and writes. Return
   * true if a copy is created.
   */
   static bool
   handle_write(nir_deref_instr *dst, nir_deref_instr *src,
               {
               nir_deref_path dst_path;
            unsigned idx = 0;
   for (nir_deref_instr **instr = dst_path.path; *instr; instr++, idx++) {
      if ((*instr)->deref_type != nir_deref_type_array)
            /* Get the entry where the index is replaced by a wildcard, so that we
   * hopefully can keep matching an array copy.
   */
   struct match_node *dst_node =
            if (!src)
            if (nir_src_as_uint((*instr)->arr.index) != dst_node->next_array_idx)
            if (dst_node->next_array_idx == 0) {
      /* At this point there may be multiple source indices which are zero,
   * so we can't pin down the actual source index. Just store it and
   * move on.
   */
      } else {
      nir_deref_path src_path;
   nir_deref_path_init(&src_path, src, state->dead_ctx);
   bool result = try_match_deref(&dst_node->first_src_path,
                     nir_deref_path_finish(&src_path);
   if (!result)
               /* Check if an aliasing write clobbered the array after the last normal
   * write. For example, with a sequence like this:
   *
   * dst[0][*] = src[0][*];
   * dst[0][0] = 0; // invalidates the array copy dst[*][*] = src[*][*]
   * dst[1][*] = src[1][*];
   *
   * Note that the second write wouldn't reset the entry for dst[*][*]
   * by itself, but it'll be caught by this check when processing the
   * third copy.
   */
   if (dst_node->last_successful_write < dst_node->last_overwritten)
                     /* In this case we've successfully processed an array element. Check if
   * this is the last, so that we can emit an array copy.
   */
   dst_node->next_array_idx++;
   dst_node->first_src_read = MIN2(dst_node->first_src_read, read_index);
   if (dst_node->next_array_idx > 1 &&
      dst_node->next_array_idx == glsl_get_length((*(instr - 1))->type)) {
   /* Make sure that nothing was overwritten. */
   struct match_node *src_node =
      node_for_path_with_wildcard(&dst_node->first_src_path,
               if (src_node->last_overwritten <= dst_node->first_src_read) {
      nir_copy_deref(b, build_wildcard_deref(b, &dst_path, idx),
               foreach_aliasing_node(&dst_path, clobber, state);
         } else {
               reset:
      dst_node->next_array_idx = 0;
   dst_node->src_wildcard_idx = -1;
   dst_node->last_successful_write = 0;
               /* Mark everything aliasing dst_path as clobbered. This needs to happen
   * last since in the loop above we need to know what last clobbered
   * dst_node and this overwrites that.
   */
               }
      static bool
   opt_find_array_copies_block(nir_builder *b, nir_block *block,
         {
                        _mesa_hash_table_clear(state->var_nodes, NULL);
            nir_foreach_instr(instr, block) {
      if (instr->type != nir_instr_type_intrinsic)
            /* Index the instructions before we do anything else. */
            /* Save the index of this instruction */
                     if (intrin->intrinsic == nir_intrinsic_load_deref) {
      handle_read(nir_src_as_deref(intrin->src[0]), state);
               if (intrin->intrinsic != nir_intrinsic_copy_deref &&
                           /* The destination must be local.  If we see a non-local store, we
   * continue on because it won't affect local stores or read-only
   * variables.
   */
   if (!nir_deref_mode_may_be(dst_deref, nir_var_function_temp))
            if (!nir_deref_mode_must_be(dst_deref, nir_var_function_temp)) {
      /* This only happens if we have something that might be a local store
   * but we don't know.  In this case, clear everything.
   */
   nir_deref_path dst_path;
   nir_deref_path_init(&dst_path, dst_deref, state->dead_ctx);
   foreach_aliasing_node(&dst_path, clobber, state);
               /* If there are any known out-of-bounds writes, then we can just skip
   * this write as it's undefined and won't contribute to building up an
   * array copy anyways.
   */
   if (nir_deref_instr_is_known_out_of_bounds(dst_deref))
            nir_deref_instr *src_deref;
   unsigned load_index = 0;
   if (intrin->intrinsic == nir_intrinsic_copy_deref) {
      src_deref = nir_src_as_deref(intrin->src[1]);
      } else {
      assert(intrin->intrinsic == nir_intrinsic_store_deref);
   nir_intrinsic_instr *load = nir_src_as_intrinsic(intrin->src[1]);
   if (load == NULL || load->intrinsic != nir_intrinsic_load_deref) {
         } else {
      src_deref = nir_src_as_deref(load->src[0]);
               if (nir_intrinsic_write_mask(intrin) !=
      (1 << glsl_get_components(dst_deref->type)) - 1) {
                  /* The source must be either local or something that's guaranteed to be
   * read-only.
   */
   if (src_deref &&
      !nir_deref_mode_must_be(src_deref, nir_var_function_temp |
                     /* There must be no indirects in the source or destination and no known
   * out-of-bounds accesses in the source, and the copy must be fully
   * qualified, or else we can't build up the array copy. We handled
   * out-of-bounds accesses to the dest above. The types must match, since
   * copy_deref currently can't bitcast mismatched deref types.
   */
   if (src_deref &&
      (nir_deref_instr_has_indirect(src_deref) ||
   nir_deref_instr_is_known_out_of_bounds(src_deref) ||
   nir_deref_instr_has_indirect(dst_deref) ||
   !glsl_type_is_vector_or_scalar(src_deref->type) ||
   glsl_get_bare_type(src_deref->type) !=
                     state->builder.cursor = nir_after_instr(instr);
   progress |= handle_write(dst_deref, src_deref, instr->index,
                  }
      static bool
   opt_find_array_copies_impl(nir_function_impl *impl)
   {
                        struct match_state s;
   s.dead_ctx = ralloc_context(NULL);
   s.var_nodes = _mesa_pointer_hash_table_create(s.dead_ctx);
   s.cast_nodes = _mesa_pointer_hash_table_create(s.dead_ctx);
            nir_foreach_block(block, impl) {
      if (opt_find_array_copies_block(&b, block, &s))
                        if (progress) {
      nir_metadata_preserve(impl, nir_metadata_block_index |
      } else {
                     }
      /**
   * This peephole optimization looks for a series of load/store_deref or
   * copy_deref instructions that copy an array from one variable to another and
   * turns it into a copy_deref that copies the entire array.  The pattern it
   * looks for is extremely specific but it's good enough to pick up on the
   * input array copies in DXVK and should also be able to pick up the sequence
   * generated by spirv_to_nir for a OpLoad of a large composite followed by
   * OpStore.
   *
   * TODO: Support out-of-order copies.
   */
   bool
   nir_opt_find_array_copies(nir_shader *shader)
   {
               nir_foreach_function_impl(impl, shader) {
      if (opt_find_array_copies_impl(impl))
                  }
