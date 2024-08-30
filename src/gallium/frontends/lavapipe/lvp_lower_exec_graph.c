   /*
   * Copyright © 2023 Valve Corporation
   *
   * SPDX-License-Identifier: MIT
   */
      #include "lvp_private.h"
      #include "nir.h"
   #include "nir_builder.h"
      #define lvp_load_internal_field(b, bit_size, field)                                                \
      nir_load_ssbo(b, 1, bit_size, nir_imm_int(b, 0),                                                \
         #define lvp_store_internal_field(b, value, field, scope)                                           \
      nir_store_ssbo(b, value, nir_imm_int(b, 0),                                                     \
                  nir_iadd_imm(b,                                                                  \
               nir_imul_imm(b, nir_load_local_invocation_index(b),                 \
      static bool
   lvp_lower_node_payload_deref(nir_builder *b, nir_instr *instr, void *data)
   {
      if (instr->type != nir_instr_type_deref)
                     bool is_payload = nir_deref_mode_is(deref, nir_var_mem_node_payload);
   bool is_payload_in = nir_deref_mode_is(deref, nir_var_mem_node_payload_in);
   if (!is_payload && !is_payload_in)
                     if (deref->deref_type != nir_deref_type_var)
            if (is_payload_in) {
      b->cursor = nir_after_instr(instr);
   nir_def *payload = lvp_load_internal_field(b, 64, payload_in);
   nir_deref_instr *cast = nir_build_deref_cast(b, payload, nir_var_mem_global, deref->type, 0);
      } else {
      nir_foreach_use_safe(use, &deref->def) {
      b->cursor = nir_before_instr(nir_src_parent_instr(use));
   nir_def *payload = nir_load_var(b, deref->var);
   nir_deref_instr *cast =
                                    }
      static bool
   lvp_lower_node_payload_derefs(nir_shader *nir)
   {
      return nir_shader_instructions_pass(nir, lvp_lower_node_payload_deref,
      }
      static void
   lvp_build_initialize_node_payloads(nir_builder *b, nir_intrinsic_instr *intr)
   {
      mesa_scope scope = nir_intrinsic_execution_scope(intr);
            nir_deref_instr *payloads_deref = nir_src_as_deref(intr->src[0]);
   assert(payloads_deref->deref_type == nir_deref_type_var);
            nir_def *addr = lvp_load_internal_field(b, 64, payloads);
   if (scope == SCOPE_INVOCATION) {
      nir_def *payloads_offset =
            }
            nir_def *payload_count = intr->src[1].ssa;
            nir_def *node_index = intr->src[1].ssa;
      }
      static bool
   lvp_lower_node_payload_intrinsic(nir_builder *b, nir_intrinsic_instr *intr,
         {
      if (intr->intrinsic == nir_intrinsic_enqueue_node_payloads) {
      nir_instr_remove(&intr->instr);
                        switch (intr->intrinsic) {
   case nir_intrinsic_initialize_node_payloads:
      lvp_build_initialize_node_payloads(b, intr);
   nir_instr_remove(&intr->instr);
      case nir_intrinsic_finalize_incoming_node_payload:
      nir_def_rewrite_uses(&intr->def, nir_imm_true(b));
   nir_instr_remove(&intr->instr);
      case nir_intrinsic_load_coalesced_input_count:
      nir_def_rewrite_uses(&intr->def, nir_imm_int(b, 1));
   nir_instr_remove(&intr->instr);
      default:
            }
      static bool
   lvp_lower_exec_graph_intrinsics(nir_shader *nir)
   {
      return nir_shader_intrinsics_pass(nir, lvp_lower_node_payload_intrinsic,
      }
      static void
   lvp_lower_node_payload_vars(struct lvp_pipeline *pipeline, nir_shader *nir)
   {
      nir_foreach_variable_in_shader(var, nir) {
      if (var->data.mode != nir_var_mem_node_payload &&
                  if (var->data.mode == nir_var_mem_node_payload) {
      assert(var->data.node_name);
   assert(!pipeline->exec_graph.next_name);
               var->data.mode = nir_var_shader_temp;
         }
      bool
   lvp_lower_exec_graph(struct lvp_pipeline *pipeline, nir_shader *nir)
   {
      bool progress = false;
   NIR_PASS(progress, nir, nir_lower_vars_to_explicit_types,
                  if (!progress)
            /* Lower node payload variables to 64-bit addresses. */
            /* Lower exec graph intrinsics to their actual implementation. */
            /* Lower node payloads to load/store_global intructions. */
   lvp_lower_node_payload_derefs(nir);
            /* Cleanup passes */
   NIR_PASS(_, nir, nir_lower_global_vars_to_local);
   NIR_PASS(_, nir, nir_lower_vars_to_ssa);
   NIR_PASS(_, nir, nir_opt_constant_folding);
               }
