   /*
   * Copyright 2023 Valve Corporation
   * SPDX-License-Identifier: MIT
   */
      #include "util/bitscan.h"
   #include "util/hash_table.h"
   #include "util/list.h"
   #include "util/ralloc.h"
   #include "nir.h"
   #include "nir_builder.h"
   #include "nir_builder_opcodes.h"
   #include "nir_intrinsics_indices.h"
      /*
   * If we have NIR like
   *
   *    x = load_reg reg
   *    use(x)
   *
   * we can translate to a single instruction use(reg) in one step by inspecting
   * the parent instruction of x, which is convenient for instruction selection
   * that historically used registers.
   *
   * However, if we have an intervening store
   *
   *    x = load_reg reg
   *    store_reg reg, y
   *    use(x)
   *
   * we are no longer able to translate to use(reg), since reg has been
   * overwritten. We could detect the write-after-read hazard at instruction
   * selection time, but that requires an O(n) walk of instructions for each
   * register source read, leading to quadratic compile time. Instead, we ensure
   * this hazard does not happen and then use the simple O(1) translation.
   *
   * We say that a load_register is "trivial" if:
   *
   *  1. every use is in the same block as the load_reg
   *
   *  2. there is no intervening store_register (write-after-read) between the
   *     load and the use.
   *
   * Similar, a store_register is trivial if:
   *
   * 1. the value stored has exactly one use (the store)
   *
   * 2. the value is written in the same block as the store
   *
   * 3. the live range of the components of the value being stored does not
   *    overlap the live range of the destination of any load_reg or the data
   *    source components of any store_reg operating on that same register.  The
   *    live ranges of the data portions of two store_reg intrinscis may overlap
   *    if they have non-intersecting write masks.
   *
   * 3. the producer is not a load_const or ssa_undef (as these historically
   *    could not write to registers so backends are expecting SSA here), or a
   *    load_reg (since backends need a move to copy between registers)
   *
   * 4. if indirect, the indirect index is live at the producer.
   *
   * This pass inserts copies to ensure that all load_reg/store_reg are trivial.
   */
      /*
   * In order to allow for greater freedom elsewhere in the pass, move all
   * reg_decl intrinsics to the top of their block.  This ensures in particular
   * that decl_reg intrinsics occur before the producer of the SSA value
   * consumed by a store_reg whenever they're all in the same block.
   */
   static void
   move_reg_decls(nir_block *block)
   {
               nir_foreach_instr_safe(instr, block) {
      if (instr->type != nir_instr_type_intrinsic)
            nir_intrinsic_instr *intr = nir_instr_as_intrinsic(instr);
   if (intr->intrinsic != nir_intrinsic_decl_reg)
            nir_instr_move(cursor, instr);
         }
      /*
   * Any load can be trivialized by copying immediately after the load and then
   * rewriting uses of the load to read from the copy. That has no functional
   * change, but it means that for every use of the load (the copy), there is no
   * intervening instruction and in particular no intervening store on any control
   * flow path. Therefore the load is trivial.
   */
   static void
   trivialize_load(nir_intrinsic_instr *load)
   {
               nir_builder b = nir_builder_at(nir_after_instr(&load->instr));
   nir_def *copy = nir_mov(&b, &load->def);
   copy->divergent = load->def.divergent;
               }
      struct trivialize_src_state {
      nir_block *block;
      };
      static bool
   trivialize_src(nir_src *src, void *state_)
   {
               nir_instr *parent = src->ssa->parent_instr;
   if (parent->type != nir_instr_type_intrinsic)
            nir_intrinsic_instr *intr = nir_instr_as_intrinsic(parent);
   if (!nir_is_load_reg(intr))
            if (intr->instr.block != state->block ||
      !BITSET_TEST(state->trivial_loads, intr->def.index))
            }
      static void
   trivialize_loads(nir_function_impl *impl, nir_block *block)
   {
      struct trivialize_src_state state = {
      .block = block,
   .trivial_loads = calloc(BITSET_WORDS(impl->ssa_alloc),
               nir_foreach_instr_safe(instr, block) {
      /* Our cross-block serialization can't handle phis */
                     /* We maintain a set of register loads which can be accessed trivially.
   * When we hit a load, it is added to the trivial set. When the register
   * is stored, all loads from the register become nontrivial. That means
   * the window between the load and the store is where the register can be
   * accessed legally.
   *
   * Note that we must track loads and not registers to correctly handle
   * cases like:
   *
   *    %1 = @load_reg %0
   *    %2 = @load_reg %0
   *    @store_reg data, %0
   *    use %1
   *
   * This is pretty obscure but it isn't a big deal to handle.
   */
   if (instr->type == nir_instr_type_intrinsic) {
               /* We don't consider indirect loads to ever be trivial */
   if (intr->intrinsic == nir_intrinsic_load_reg_indirect) {
         } else if (intr->intrinsic == nir_intrinsic_load_reg) {
                           nir_foreach_reg_load(load, reg) {
                                          /* Also check the condition of the next if */
   nir_if *nif = nir_block_get_following_if(block);
   if (nif)
               }
      /*
   * Any store can be made trivial by inserting a copy of the value immediately
   * before the store and reading from the copy instead. Proof:
   *
   * 1. The new value stored (the copy result) is used exactly once.
   *
   * 2. No intervening instructions between the copy and the store.
   *
   * 3. The copy is ALU, not load_const or ssa_undef.
   *
   * 4. The indirect index must be live at the store, which means it is also
   * live at the copy inserted immediately before the store (same live-in set),
   * so it is live at the new producer (the copy).
   */
   static void
   isolate_store(nir_intrinsic_instr *store)
   {
               nir_builder b = nir_builder_at(nir_before_instr(&store->instr));
   nir_def *copy = nir_mov(&b, store->src[0].ssa);
   copy->divergent = store->src[0].ssa->divergent;
      }
      static void
   clear_store(nir_intrinsic_instr *store,
               {
      nir_component_mask_t mask = nir_intrinsic_write_mask(store);
   u_foreach_bit(c, mask) {
      assert(c < num_reg_components);
   assert(reg_stores[c] == store);
         }
      static void
   clear_reg_stores(nir_def *reg,
         {
      /* At any given point in store trivialize pass, every store in the current
   * block is either trivial or in the possibly_trivial_stores map.
   */
   struct hash_entry *entry =
         if (entry == NULL)
            nir_intrinsic_instr **stores = entry->data;
   nir_intrinsic_instr *decl = nir_reg_get_decl(reg);
            for (unsigned c = 0; c < num_components; c++) {
      if (stores[c] == NULL)
                  }
      static void
   trivialize_store(nir_intrinsic_instr *store,
         {
               /* At any given point in store trivialize pass, every store in the current
   * block is either trivial or in the possibly_trivial_stores map.
   */
   struct hash_entry *entry =
         if (entry == NULL)
            nir_intrinsic_instr **stores = entry->data;
   nir_intrinsic_instr *decl = nir_reg_get_decl(reg);
            nir_component_mask_t found = 0;
   for (unsigned c = 0; c < num_components; c++) {
      if (stores[c] == store)
      }
   if (!found)
            /* A store can't be only partially trivial */
            isolate_store(store);
      }
      static void
   trivialize_reg_stores(nir_def *reg, nir_component_mask_t mask,
         {
      /* At any given point in store trivialize pass, every store in the current
   * block is either trivial or in the possibly_trivial_stores map.
   */
   struct hash_entry *entry =
         if (entry == NULL)
            nir_intrinsic_instr **stores = entry->data;
   nir_intrinsic_instr *decl = nir_reg_get_decl(reg);
            u_foreach_bit(c, mask) {
      assert(c < num_components);
   if (stores[c] == NULL)
            isolate_store(stores[c]);
         }
      /*
   * Trivialize for read-after-write hazards, given a load that is between the def
   * and store.
   */
   static void
   trivialize_read_after_write(nir_intrinsic_instr *load,
         {
               unsigned nr = load->def.num_components;
   trivialize_reg_stores(load->src[0].ssa, nir_component_mask(nr),
      }
      static bool
   clear_def(nir_def *def, void *state)
   {
               nir_foreach_use(src, def) {
      if (nir_src_is_if(src))
            nir_instr *parent = nir_src_parent_instr(src);
   if (parent->type != nir_instr_type_intrinsic)
            nir_intrinsic_instr *store = nir_instr_as_intrinsic(parent);
   if (!nir_is_store_reg(store))
            /* Anything global has already been trivialized and can be ignored */
   if (parent->block != def->parent_instr->block)
            if (def == store->src[0].ssa) {
      /* We encountered a value which is written by a store_reg so, if this
   * store is still in possibly_trivial_stores, it is trivial and we
   * can remove it from the set.
   */
   assert(list_is_singular(&def->uses));
      } else {
      /* We encoutered either the ineirect index or the decl_reg (unlikely)
   * before the value while iterating backwards.  Trivialize the store
   * now to maintain dominance.
   */
                     }
      /*
   * If a load_reg will be folded into this instruction, we need to handle
   * read-after-write hazards, the same as if we hit a load_reg directly.
   */
   static bool
   trivialize_source(nir_src *src, void *state)
   {
               nir_intrinsic_instr *load_reg = nir_load_reg_for_def(src->ssa);
   if (load_reg)
               }
      static void
   trivialize_stores(nir_function_impl *impl, nir_block *block)
   {
      /* Hash table mapping decl_reg defs to a num_components-size array of
   * nir_intrinsic_instr*s. Each component contains the pointer to the next
   * store to that component, if one exists in the block while walking
   * backwards that has not yet had an intervening load, or NULL otherwise.
   * This represents the set of stores that, at the current point of iteration,
   * could be trivial.
   */
   struct hash_table *possibly_trivial_stores =
            /* Following the algorithm directly, we would call trivialize_source() on
   * the following if source here because we are walking instructions
   * backwards so you process the following if before instructions in that
   * case.  However, we know a priori that possibly_trivial_stores is empty
   * at this point so trivialize_source() is a no-op.
            nir_foreach_instr_reverse_safe(instr, block) {
               if (instr->type == nir_instr_type_intrinsic) {
               if (nir_is_load_reg(intr)) {
      /* Read-after-write: there is a load between the def and store. */
      } else if (nir_is_store_reg(intr)) {
      nir_def *value = intr->src[0].ssa;
   nir_def *reg = intr->src[1].ssa;
   nir_intrinsic_instr *decl = nir_reg_get_decl(reg);
   unsigned num_components = nir_intrinsic_num_components(decl);
                                                               /* SSA-only instruction types */
   nir_instr *parent = value->parent_instr;
                                 /* Don't allow write masking with non-ALU types for compatibility,
   * since other types didn't have write masks in old NIR.
   */
   nontrivial |=
                  /* Need a move for register copies */
                  if (nontrivial) {
         } else {
                                    if (entry) {
         } else {
                                       u_foreach_bit(c, write_mask) {
      assert(c < num_components);
   assert(stores[c] == NULL);
                        /* Once we have trivialized loads, we are guaranteed that no store_reg
   * exists in the live range of the destination of a load_reg for the
   * same register.  When trivializing stores, we must further ensure that
   * this holds for the entire live range of the data source of the
   * store_reg and not just for the store_reg instruction itself.  Because
   * values are always killed by sources, we can determine live range
   * interference when walking backwards by looking at the sources of each
   * instruction which read from a load_reg and trivializing any store_reg
   * which interfere with that load.
   *
   * We trivialize sources at the end, because we iterate backwards and in
   * program order the sources are read first.
   */
                  }
      void
   nir_trivialize_registers(nir_shader *s)
   {
      nir_foreach_function_impl(impl, s) {
      /* All decl_reg intrinsics are in the start block. */
            nir_foreach_block(block, impl) {
      trivialize_loads(impl, block);
            }
