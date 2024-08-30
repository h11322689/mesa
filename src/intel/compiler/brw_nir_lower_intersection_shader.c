   /*
   * Copyright (c) 2020 Intel Corporation
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
      #include "brw_nir_rt.h"
   #include "brw_nir_rt_builder.h"
      static nir_function_impl *
   lower_any_hit_for_intersection(nir_shader *any_hit)
   {
               /* Any-hit shaders need three parameters */
   assert(impl->function->num_params == 0);
   nir_parameter params[] = {
      {
      /* A pointer to a boolean value for whether or not the hit was
   * accepted.
   */
   .num_components = 1,
      },
   {
      /* The hit T value */
   .num_components = 1,
      },
   {
      /* The hit kind */
   .num_components = 1,
         };
   impl->function->num_params = ARRAY_SIZE(params);
   impl->function->params =
                  nir_builder build = nir_builder_at(nir_before_impl(impl));
            nir_def *commit_ptr = nir_load_param(b, 0);
   nir_def *hit_t = nir_load_param(b, 1);
            nir_deref_instr *commit =
      nir_build_deref_cast(b, commit_ptr, nir_var_function_temp,
         nir_foreach_block_safe(block, impl) {
      nir_foreach_instr_safe(instr, block) {
      switch (instr->type) {
   case nir_instr_type_intrinsic: {
      nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(instr);
   switch (intrin->intrinsic) {
   case nir_intrinsic_ignore_ray_intersection:
      b->cursor = nir_instr_remove(&intrin->instr);
   /* We put the newly emitted code inside a dummy if because it's
   * going to contain a jump instruction and we don't want to
   * deal with that mess here.  It'll get dealt with by our
   * control-flow optimization passes.
   */
   nir_store_deref(b, commit, nir_imm_false(b), 0x1);
   nir_push_if(b, nir_imm_true(b));
                     case nir_intrinsic_terminate_ray:
      /* The "normal" handling of terminateRay works fine in
                     case nir_intrinsic_load_ray_t_max:
      nir_def_rewrite_uses(&intrin->def,
                     case nir_intrinsic_load_ray_hit_kind:
      nir_def_rewrite_uses(&intrin->def,
                     default:
         }
               case nir_instr_type_jump: {
      /* Stomp any halts to returns since they only return from the
   * any-hit shader and not necessarily from the intersection
   * shader.  This is safe to do because we've already asserted
   * that we only have the one function.
   */
   nir_jump_instr *jump = nir_instr_as_jump(instr);
   if (jump->type == nir_jump_halt)
                     default:
                                                      }
      void
   brw_nir_lower_intersection_shader(nir_shader *intersection,
               {
               nir_function_impl *any_hit_impl = NULL;
   struct hash_table *any_hit_var_remap = NULL;
   if (any_hit) {
      nir_shader *any_hit_tmp = nir_shader_clone(dead_ctx, any_hit);
   NIR_PASS_V(any_hit_tmp, nir_opt_dce);
   any_hit_impl = lower_any_hit_for_intersection(any_hit_tmp);
                        nir_builder build = nir_builder_at(nir_before_impl(impl));
            nir_def *t_addr = brw_nir_rt_mem_hit_addr(b, false /* committed */);
   nir_variable *commit =
                  assert(impl->end_block->predecessors->entries == 1);
   set_foreach(impl->end_block->predecessors, block_entry) {
      struct nir_block *block = (void *)block_entry->key;
   b->cursor = nir_after_block_before_jump(block);
   nir_push_if(b, nir_load_var(b, commit));
   {
      /* Set the "valid" bit in mem_hit */
   nir_def *ray_addr = brw_nir_rt_mem_hit_addr(b, false /* committed */);
   nir_def *flags_dw_addr = nir_iadd_imm(b, ray_addr, 12);
   nir_store_global(b, flags_dw_addr, 4,
                     }
   nir_push_else(b, NULL);
   {
         }
   nir_pop_if(b, NULL);
               nir_foreach_block_safe(block, impl) {
      nir_foreach_instr_safe(instr, block) {
      switch (instr->type) {
   case nir_instr_type_intrinsic: {
      nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(instr);
   switch (intrin->intrinsic) {
   case nir_intrinsic_report_ray_intersection: {
      b->cursor = nir_instr_remove(&intrin->instr);
                                                               /* bool commit_tmp = false; */
   nir_variable *commit_tmp =
                        nir_push_if(b, nir_iand(b, nir_fge(b, hit_t, min_t),
                                 if (any_hit_impl != NULL) {
      nir_push_if(b, nir_inot(b, nir_load_leaf_opaque_intel(b)));
   {
      nir_def *params[] = {
      &nir_build_deref_var(b, commit_tmp)->def,
   hit_t,
      };
   nir_inline_function_impl(b, any_hit_impl, params,
                                                               nir_store_global(b, nir_iadd_imm(b, ray_addr, 16 + 12), 4,  hit_t, 0x1);
   nir_store_global(b, t_addr, 4,
            }
                     nir_def *accepted = nir_load_var(b, commit_tmp);
   nir_def_rewrite_uses(&intrin->def,
                     default:
         }
               default:
               }
            /* We did some inlining; have to re-index SSA defs */
               }
