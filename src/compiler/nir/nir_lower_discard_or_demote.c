   /*
   * Copyright © 2020 Valve Corporation
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
   *
   */
      #include "nir.h"
   #include "nir_builder.h"
      static bool
   lower_discard_to_demote(nir_builder *b, nir_intrinsic_instr *intrin, void *data)
   {
      switch (intrin->intrinsic) {
   case nir_intrinsic_discard:
      intrin->intrinsic = nir_intrinsic_demote;
      case nir_intrinsic_discard_if:
      intrin->intrinsic = nir_intrinsic_demote_if;
      case nir_intrinsic_load_helper_invocation:
      intrin->intrinsic = nir_intrinsic_is_helper_invocation;
      default:
            }
      static bool
   lower_demote_to_discard(nir_builder *b, nir_intrinsic_instr *intrin, void *data)
   {
      switch (intrin->intrinsic) {
   case nir_intrinsic_demote:
      intrin->intrinsic = nir_intrinsic_discard;
      case nir_intrinsic_demote_if:
      intrin->intrinsic = nir_intrinsic_discard_if;
      case nir_intrinsic_is_helper_invocation:
   case nir_intrinsic_load_helper_invocation: {
      /* If the shader doesn't need helper invocations,
   * we can assume there are none */
   b->cursor = nir_before_instr(&intrin->instr);
   nir_def *zero = nir_imm_false(b);
   nir_def_rewrite_uses(&intrin->def, zero);
   nir_instr_remove(&intrin->instr);
      }
   default:
            }
      static nir_def *
   insert_is_helper(nir_builder *b, nir_instr *instr)
   {
      /* find best place to insert is_helper */
   nir_cf_node *node = &instr->block->cf_node;
   while (node->parent->type != nir_cf_node_function)
         nir_block *block = nir_cf_node_as_block(node);
   if (block == instr->block) {
         } else {
         }
      }
      static bool
   lower_load_helper_to_is_helper(nir_builder *b,
         {
      nir_def *is_helper = *(nir_def **)data;
   switch (intrin->intrinsic) {
   case nir_intrinsic_demote:
   case nir_intrinsic_demote_if:
      /* insert is_helper at last top level occasion */
   if (is_helper == NULL) {
      is_helper = insert_is_helper(b, &intrin->instr);
   *(nir_def **)data = is_helper;
      } else {
            case nir_intrinsic_load_helper_invocation:
      /* Don't update data: as long as we didn't encounter any demote(),
   * we can insert new is_helper() intrinsics. These are placed at
   * top-level blocks to ensure correct behavior w.r.t. loops */
   if (is_helper == NULL)
         nir_def_rewrite_uses(&intrin->def, is_helper);
   nir_instr_remove(&intrin->instr);
      default:
            }
      /**
   * Optimize discard and demote opcodes.
   *
   * If force_correct_quad_ops_after_discard is true and quad operations are
   * used, discard() will be converted to demote() and gl_HelperInvocation will
   * be lowered to helperInvocationEXT(). This is intended as workaround for
   * game bugs to force correct derivatives after kill. This lowering is not
   * valid in the general case as it might change the result of subgroup
   * operations and loop behavior.
   *
   * Otherwise, if demote is used and no ops need helper invocations, demote()
   * will be converted to discard() as an optimization.
   */
   bool
   nir_lower_discard_or_demote(nir_shader *shader,
         {
      if (shader->info.stage != MESA_SHADER_FRAGMENT)
            /* We need uses_discard/demote and needs_*_helper_invocations. */
   nir_shader_gather_info(shader, nir_shader_get_entrypoint(shader));
   /* Validate that if uses_demote is set, uses_discard is also be set. */
            /* Quick skip. */
   if (!shader->info.fs.uses_discard)
                     if (force_correct_quad_ops_after_discard &&
      shader->info.fs.needs_quad_helper_invocations) {
   /* If we need correct derivatives, convert discard to demote only when
   * derivatives are actually used.
   */
   progress = nir_shader_intrinsics_pass(shader, lower_discard_to_demote,
                                    } else if (!shader->info.fs.needs_quad_helper_invocations &&
            !shader->info.fs.needs_all_helper_invocations &&
   /* If we don't need any helper invocations, convert demote to discard. */
   progress = nir_shader_intrinsics_pass(shader, lower_demote_to_discard,
                        } else if (shader->info.fs.uses_demote &&
            BITSET_TEST(shader->info.system_values_read,
   /* load_helper needs to preserve the value (whether an invocation is
   * a helper lane) from the beginning of the shader. */
   nir_def *is_helper = NULL;
   progress = nir_shader_intrinsics_pass(shader,
                           BITSET_CLEAR(shader->info.system_values_read,
               /* Validate again that if uses_demote is set, uses_discard is also be set. */
   assert(!shader->info.fs.uses_demote || shader->info.fs.uses_discard);
      }
