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
   #include "nir_deref.h"
   #include "nir_phi_builder.h"
   #include "nir_vla.h"
      struct deref_node {
      struct deref_node *parent;
                     /* Only valid for things that end up in the direct list.
   * Note that multiple nir_deref_instrs may correspond to this node, but
   * they will all be equivalent, so any is as good as the other.
   */
   nir_deref_path path;
            struct set *loads;
   struct set *stores;
                     /* True if this node is fully direct.  If set, it must be in the children
   * array of its parent.
   */
            /* Set on a root node for a variable to indicate that variable is used by a
   * cast or passed through some other sequence of instructions that are not
   * derefs.
   */
            struct deref_node *wildcard;
   struct deref_node *indirect;
      };
      #define UNDEF_NODE ((struct deref_node *)(uintptr_t)1)
      struct lower_variables_state {
      nir_shader *shader;
   void *dead_ctx;
            /* A hash table mapping variables to deref_node data */
            /* A hash table mapping fully-qualified direct dereferences, i.e.
   * dereferences with no indirect or wildcard array dereferences, to
   * deref_node data.
   *
   * At the moment, we only lower loads, stores, and copies that can be
   * trivially lowered to loads and stores, i.e. copies with no indirects
   * and no wildcards.  If a part of a variable that is being loaded from
   * and/or stored into is also involved in a copy operation with
   * wildcards, then we lower that copy operation to loads and stores, but
   * otherwise we leave copies with wildcards alone. Since the only derefs
   * used in these loads, stores, and trivial copies are ones with no
   * wildcards and no indirects, these are precisely the derefs that we
   * can actually consider lowering.
   */
            /* Controls whether get_deref_node will add variables to the
   * direct_deref_nodes table.  This is turned on when we are initially
   * scanning for load/store instructions.  It is then turned off so we
   * don't accidentally change the direct_deref_nodes table while we're
   * iterating throug it.
   */
               };
      static struct deref_node *
   deref_node_create(struct deref_node *parent,
               {
      size_t size = sizeof(struct deref_node) +
            struct deref_node *node = rzalloc_size(mem_ctx, size);
   node->type = type;
   node->parent = parent;
   exec_node_init(&node->direct_derefs_link);
               }
      /* Returns the deref node associated with the given variable.  This will be
   * the root of the tree representing all of the derefs of the given variable.
   */
   static struct deref_node *
   get_deref_node_for_var(nir_variable *var, struct lower_variables_state *state)
   {
               struct hash_entry *var_entry =
            if (var_entry) {
         } else {
      node = deref_node_create(NULL, var->type, true, state->dead_ctx);
   _mesa_hash_table_insert(state->deref_var_nodes, var, node);
         }
      /* Gets the deref_node for the given deref chain and creates it if it
   * doesn't yet exist.  If the deref is fully-qualified and direct and
   * state->add_to_direct_deref_nodes is true, it will be added to the hash
   * table of of fully-qualified direct derefs.
   */
   static struct deref_node *
   get_deref_node_recur(nir_deref_instr *deref,
         {
      if (deref->deref_type == nir_deref_type_var)
            if (deref->deref_type == nir_deref_type_cast)
            struct deref_node *parent =
         if (parent == NULL)
            if (parent == UNDEF_NODE)
            switch (deref->deref_type) {
   case nir_deref_type_struct:
      assert(glsl_type_is_struct_or_ifc(parent->type));
            if (parent->children[deref->strct.index] == NULL) {
      parent->children[deref->strct.index] =
      deref_node_create(parent, deref->type, parent->is_direct,
                  case nir_deref_type_array: {
      if (nir_src_is_const(deref->arr.index)) {
      uint32_t index = nir_src_as_uint(deref->arr.index);
   /* This is possible if a loop unrolls and generates an
   * out-of-bounds offset.  We need to handle this at least
   * somewhat gracefully.
   */
                  if (parent->children[index] == NULL) {
      parent->children[index] =
                        } else {
      if (parent->indirect == NULL) {
      parent->indirect =
                  }
               case nir_deref_type_array_wildcard:
      if (parent->wildcard == NULL) {
      parent->wildcard =
                     default:
            }
      static struct deref_node *
   get_deref_node(nir_deref_instr *deref, struct lower_variables_state *state)
   {
      /* This pass only works on local variables.  Just ignore any derefs with
   * a non-local mode.
   */
   if (!nir_deref_mode_must_be(deref, nir_var_function_temp))
            if (glsl_type_is_cmat(deref->type))
            struct deref_node *node = get_deref_node_recur(deref, state);
   if (!node)
            /* Insert the node in the direct derefs list.  We only do this if it's not
   * already in the list and we only bother for deref nodes which are used
   * directly in a load or store.
   */
   if (node != UNDEF_NODE && node->is_direct &&
      state->add_to_direct_deref_nodes &&
   node->direct_derefs_link.next == NULL) {
   nir_deref_path_init(&node->path, deref, state->dead_ctx);
   assert(deref->var != NULL);
   exec_list_push_tail(&state->direct_deref_nodes,
                  }
      /* \sa foreach_deref_node_match */
   static void
   foreach_deref_node_worker(struct deref_node *node, nir_deref_instr **path,
                     {
      if (*path == NULL) {
      cb(node, state);
               switch ((*path)->deref_type) {
   case nir_deref_type_struct:
      if (node->children[(*path)->strct.index]) {
      foreach_deref_node_worker(node->children[(*path)->strct.index],
      }
         case nir_deref_type_array: {
               if (node->children[index]) {
      foreach_deref_node_worker(node->children[index],
               if (node->wildcard) {
      foreach_deref_node_worker(node->wildcard,
      }
               default:
            }
      /* Walks over every "matching" deref_node and calls the callback.  A node
   * is considered to "match" if either refers to that deref or matches up t
   * a wildcard.  In other words, the following would match a[6].foo[3].bar:
   *
   * a[6].foo[3].bar
   * a[*].foo[3].bar
   * a[6].foo[*].bar
   * a[*].foo[*].bar
   *
   * The given deref must be a full-length and fully qualified (no wildcards
   * or indirects) deref chain.
   */
   static void
   foreach_deref_node_match(nir_deref_path *path,
                     {
      assert(path->path[0]->deref_type == nir_deref_type_var);
            if (node == NULL)
               }
      /* \sa deref_may_be_aliased */
   static bool
   path_may_be_aliased_node(struct deref_node *node, nir_deref_instr **path,
         {
      if (*path == NULL)
            switch ((*path)->deref_type) {
   case nir_deref_type_struct:
      if (node->children[(*path)->strct.index]) {
      return path_may_be_aliased_node(node->children[(*path)->strct.index],
      } else {
               case nir_deref_type_array: {
      if (!nir_src_is_const((*path)->arr.index))
                     /* If there is an indirect at this level, we're aliased. */
   if (node->indirect)
            if (node->children[index] &&
      path_may_be_aliased_node(node->children[index],
               if (node->wildcard &&
                              default:
            }
      /* Returns true if there are no indirects that can ever touch this deref.
   *
   * For example, if the given deref is a[6].foo, then any uses of a[i].foo
   * would cause this to return false, but a[i].bar would not affect it
   * because it's a different structure member.  A var_copy involving of
   * a[*].bar also doesn't affect it because that can be lowered to entirely
   * direct load/stores.
   *
   * We only support asking this question about fully-qualified derefs.
   * Obviously, it's pointless to ask this about indirects, but we also
   * rule-out wildcards.  Handling Wildcard dereferences would involve
   * checking each array index to make sure that there aren't any indirect
   * references.
   */
   static bool
   path_may_be_aliased(nir_deref_path *path,
         {
      assert(path->path[0]->deref_type == nir_deref_type_var);
   nir_variable *var = path->path[0]->var;
            /* First see if this variable is ever used by anything other than a
   * load/store.  If there's even so much as a cast in the way, we have to
   * assume aliasing and bail.
   */
   if (var_node->has_complex_use)
               }
      static void
   register_complex_use(nir_deref_instr *deref,
         {
      assert(deref->deref_type == nir_deref_type_var);
   struct deref_node *node = get_deref_node_for_var(deref->var, state);
   if (node == NULL)
               }
      static bool
   register_load_instr(nir_intrinsic_instr *load_instr,
         {
      nir_deref_instr *deref = nir_src_as_deref(load_instr->src[0]);
   struct deref_node *node = get_deref_node(deref, state);
   if (node == NULL)
            /* Replace out-of-bounds load derefs with an undef, so that they don't get
   * left around when a driver has lowered all indirects and thus doesn't
   * expect any array derefs at all after vars_to_ssa.
   */
   if (node == UNDEF_NODE) {
      nir_undef_instr *undef =
      nir_undef_instr_create(state->shader,
               nir_instr_insert_before(&load_instr->instr, &undef->instr);
            nir_def_rewrite_uses(&load_instr->def, &undef->def);
               if (node->loads == NULL)
                        }
      static bool
   register_store_instr(nir_intrinsic_instr *store_instr,
         {
      nir_deref_instr *deref = nir_src_as_deref(store_instr->src[0]);
            /* Drop out-of-bounds store derefs, so that they don't get left around when a
   * driver has lowered all indirects and thus doesn't expect any array derefs
   * at all after vars_to_ssa.
   */
   if (node == UNDEF_NODE) {
      nir_instr_remove(&store_instr->instr);
               if (node == NULL)
            if (node->stores == NULL)
                        }
      static void
   register_copy_instr(nir_intrinsic_instr *copy_instr,
         {
      for (unsigned idx = 0; idx < 2; idx++) {
      nir_deref_instr *deref = nir_src_as_deref(copy_instr->src[idx]);
   struct deref_node *node = get_deref_node(deref, state);
   if (node == NULL || node == UNDEF_NODE)
            if (node->copies == NULL)
                  }
      static bool
   register_variable_uses(nir_function_impl *impl,
         {
               nir_foreach_block(block, impl) {
      nir_foreach_instr_safe(instr, block) {
      switch (instr->type) {
                     if (deref->deref_type == nir_deref_type_var &&
                                                switch (intrin->intrinsic) {
   case nir_intrinsic_load_deref:
                  case nir_intrinsic_store_deref:
                  case nir_intrinsic_copy_deref:
                  default:
         }
               default:
               }
      }
      /* Walks over all of the copy instructions to or from the given deref_node
   * and lowers them to load/store intrinsics.
   */
   static void
   lower_copies_to_load_store(struct deref_node *node,
         {
      if (!node->copies)
                     set_foreach(node->copies, copy_entry) {
                        for (unsigned i = 0; i < 2; ++i) {
                     /* Only bother removing copy entries for other nodes */
                  struct set_entry *arg_entry = _mesa_set_search(arg_node->copies, copy);
   assert(arg_entry);
                              }
      /* Performs variable renaming
   *
   * This algorithm is very similar to the one outlined in "Efficiently
   * Computing Static Single Assignment Form and the Control Dependence
   * Graph" by Cytron et al.  The primary difference is that we only put one
   * SSA def on the stack per block.
   */
   static bool
   rename_variables(struct lower_variables_state *state)
   {
               nir_foreach_block(block, state->impl) {
      nir_foreach_instr_safe(instr, block) {
                              switch (intrin->intrinsic) {
   case nir_intrinsic_load_deref: {
      nir_deref_instr *deref = nir_src_as_deref(intrin->src[0]);
                  struct deref_node *node = get_deref_node(deref, state);
                                                nir_alu_instr *mov = nir_alu_instr_create(state->shader,
         mov->src[0].src = nir_src_for_ssa(
                                                      nir_def_rewrite_uses(&intrin->def,
                     case nir_intrinsic_store_deref: {
      nir_deref_instr *deref = nir_src_as_deref(intrin->src[0]);
                  struct deref_node *node = get_deref_node(deref, state);
                                                                                       unsigned wrmask = nir_intrinsic_write_mask(intrin);
   if (wrmask == (1 << intrin->num_components) - 1) {
      /* Whole variable store - just copy the source.  Note that
   * intrin->num_components and value->num_components
   * may differ.
   */
                        new_def = nir_swizzle(&b, value, swiz,
      } else {
      nir_def *old_def =
         /* For writemasked store_var intrinsics, we combine the newly
   * written values with the existing contents of unwritten
   * channels, creating a new SSA value for the whole vector.
   */
   nir_scalar srcs[NIR_MAX_VEC_COMPONENTS];
   for (unsigned i = 0; i < intrin->num_components; i++) {
      if (wrmask & (1 << i)) {
         } else {
                                       nir_phi_builder_value_set_block_def(node->pb_value, block, new_def);
   nir_instr_remove(&intrin->instr);
               default:
                           }
      /** Implements a pass to lower variable uses to SSA values
   *
   * This path walks the list of instructions and tries to lower as many
   * local variable load/store operations to SSA defs and uses as it can.
   * The process involves four passes:
   *
   *  1) Iterate over all of the instructions and mark where each local
   *     variable deref is used in a load, store, or copy.  While we're at
   *     it, we keep track of all of the fully-qualified (no wildcards) and
   *     fully-direct references we see and store them in the
   *     direct_deref_nodes hash table.
   *
   *  2) Walk over the list of fully-qualified direct derefs generated in
   *     the previous pass.  For each deref, we determine if it can ever be
   *     aliased, i.e. if there is an indirect reference anywhere that may
   *     refer to it.  If it cannot be aliased, we mark it for lowering to an
   *     SSA value.  At this point, we lower any var_copy instructions that
   *     use the given deref to load/store operations.
   *
   *  3) Walk over the list of derefs we plan to lower to SSA values and
   *     insert phi nodes as needed.
   *
   *  4) Perform "variable renaming" by replacing the load/store instructions
   *     with SSA definitions and SSA uses.
   */
   static bool
   nir_lower_vars_to_ssa_impl(nir_function_impl *impl)
   {
               state.shader = impl->function->shader;
   state.dead_ctx = ralloc_context(state.shader);
            state.deref_var_nodes = _mesa_pointer_hash_table_create(state.dead_ctx);
            /* Build the initial deref structures and direct_deref_nodes table */
                              /* We're about to iterate through direct_deref_nodes.  Don't modify it. */
            foreach_list_typed_safe(struct deref_node, node, direct_derefs_link,
                              /* We don't build deref nodes for non-local variables */
            if (path_may_be_aliased(path, &state)) {
      exec_node_remove(&node->direct_derefs_link);
               node->lower_to_ssa = true;
                        if (!progress) {
      nir_metadata_preserve(impl, nir_metadata_all);
                        /* We may have lowered some copy instructions to load/store
   * instructions.  The uses from the copy instructions hav already been
   * removed but we need to rescan to ensure that the uses from the newly
   * added load/store instructions are registered.  We need this
   * information for phi node insertion below.
   */
                     BITSET_WORD *store_blocks =
      ralloc_array(state.dead_ctx, BITSET_WORD,
      foreach_list_typed(struct deref_node, node, direct_derefs_link,
            if (!node->lower_to_ssa)
            memset(store_blocks, 0,
            assert(node->path.path[0]->var->constant_initializer == NULL &&
            if (node->stores) {
      set_foreach(node->stores, store_entry) {
      nir_intrinsic_instr *store =
                        node->pb_value =
      nir_phi_builder_add_value(state.phi_builder,
                                          nir_metadata_preserve(impl, nir_metadata_block_index |
                        }
      bool
   nir_lower_vars_to_ssa(nir_shader *shader)
   {
               nir_foreach_function_impl(impl, shader) {
                     }
