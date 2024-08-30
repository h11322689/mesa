   /*
   * Copyright © 2017 Red Hat
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
   * Authors:
   *    Rob Clark <robclark@freedesktop.org>
   */
      #include "nir.h"
   #include "nir_builder.h"
      /*
   * Remap atomic counters to SSBOs, starting from the shader's next SSBO slot
   * (info.num_ssbos).
   */
      static nir_deref_instr *
   deref_offset_var(nir_builder *b, unsigned binding, unsigned offset_align_state)
   {
      gl_state_index16 tokens[STATE_LENGTH] = { offset_align_state, binding };
   nir_variable *var = nir_find_state_variable(b->shader, tokens);
   if (!var) {
      var = nir_state_variable_create(b->shader, glsl_uint_type(), "offset", tokens);
      }
      }
      static bool
   lower_instr(nir_intrinsic_instr *instr, unsigned ssbo_offset, nir_builder *b, unsigned offset_align_state)
   {
               /* Initialize to something to avoid spurious compiler warning */
                     switch (instr->intrinsic) {
   case nir_intrinsic_atomic_counter_inc:
   case nir_intrinsic_atomic_counter_add:
   case nir_intrinsic_atomic_counter_pre_dec:
   case nir_intrinsic_atomic_counter_post_dec:
      /* inc and dec get remapped to add: */
   atomic_op = nir_atomic_op_iadd;
      case nir_intrinsic_atomic_counter_read:
      op = nir_intrinsic_load_ssbo;
      case nir_intrinsic_atomic_counter_min:
      atomic_op = nir_atomic_op_umin;
      case nir_intrinsic_atomic_counter_max:
      atomic_op = nir_atomic_op_umax;
      case nir_intrinsic_atomic_counter_and:
      atomic_op = nir_atomic_op_iand;
      case nir_intrinsic_atomic_counter_or:
      atomic_op = nir_atomic_op_ior;
      case nir_intrinsic_atomic_counter_xor:
      atomic_op = nir_atomic_op_ixor;
      case nir_intrinsic_atomic_counter_exchange:
      atomic_op = nir_atomic_op_xchg;
      case nir_intrinsic_atomic_counter_comp_swap:
      op = nir_intrinsic_ssbo_atomic_swap;
   atomic_op = nir_atomic_op_cmpxchg;
      default:
                  nir_def *buffer = nir_imm_int(b, ssbo_offset + nir_intrinsic_base(instr));
            nir_def *offset_load = NULL;
   if (offset_align_state) {
      nir_deref_instr *deref_offset = deref_offset_var(b, nir_intrinsic_base(instr), offset_align_state);
      }
   nir_intrinsic_instr *new_instr =
         if (nir_intrinsic_has_atomic_op(new_instr))
            /* a couple instructions need special handling since they don't map
   * 1:1 with ssbo atomics
   */
   switch (instr->intrinsic) {
   case nir_intrinsic_atomic_counter_inc:
      /* remapped to ssbo_atomic_add: { buffer_idx, offset, +1 } */
   temp = nir_imm_int(b, +1);
   new_instr->src[0] = nir_src_for_ssa(buffer);
   new_instr->src[1] = nir_src_for_ssa(instr->src[0].ssa);
   new_instr->src[2] = nir_src_for_ssa(temp);
      case nir_intrinsic_atomic_counter_pre_dec:
   case nir_intrinsic_atomic_counter_post_dec:
      /* remapped to ssbo_atomic_add: { buffer_idx, offset, -1 } */
   /* NOTE semantic difference so we adjust the return value below */
   temp = nir_imm_int(b, -1);
   new_instr->src[0] = nir_src_for_ssa(buffer);
   new_instr->src[1] = nir_src_for_ssa(instr->src[0].ssa);
   new_instr->src[2] = nir_src_for_ssa(temp);
      case nir_intrinsic_atomic_counter_read:
      /* remapped to load_ssbo: { buffer_idx, offset } */
   new_instr->src[0] = nir_src_for_ssa(buffer);
   new_instr->src[1] = nir_src_for_ssa(instr->src[0].ssa);
      default:
      /* remapped to ssbo_atomic_x: { buffer_idx, offset, data, (compare)? } */
   new_instr->src[0] = nir_src_for_ssa(buffer);
   new_instr->src[1] = nir_src_for_ssa(instr->src[0].ssa);
   new_instr->src[2] = nir_src_for_ssa(instr->src[1].ssa);
   if (op == nir_intrinsic_ssbo_atomic_swap)
                     if (offset_load)
            if (nir_intrinsic_range_base(instr))
      new_instr->src[1].ssa = nir_iadd(b, new_instr->src[1].ssa,
         if (new_instr->intrinsic == nir_intrinsic_load_ssbo) {
               /* we could be replacing an intrinsic with fixed # of dest
   * num_components with one that has variable number.  So
   * best to take this from the dest:
   */
               nir_def_init(&new_instr->instr, &new_instr->def,
         nir_instr_insert_before(&instr->instr, &new_instr->instr);
            if (instr->intrinsic == nir_intrinsic_atomic_counter_pre_dec) {
      b->cursor = nir_after_instr(&new_instr->instr);
   nir_def *result = nir_iadd(b, &new_instr->def, temp);
      } else {
                     }
      static bool
   is_atomic_uint(const struct glsl_type *type)
   {
      if (glsl_get_base_type(type) == GLSL_TYPE_ARRAY)
            }
      bool
   nir_lower_atomics_to_ssbo(nir_shader *shader, unsigned offset_align_state)
   {
      unsigned ssbo_offset = shader->info.num_ssbos;
            nir_foreach_function_impl(impl, shader) {
      nir_builder builder = nir_builder_create(impl);
   nir_foreach_block(block, impl) {
      nir_foreach_instr_safe(instr, block) {
      if (instr->type == nir_instr_type_intrinsic)
      progress |= lower_instr(nir_instr_as_intrinsic(instr),
               nir_metadata_preserve(impl, nir_metadata_block_index |
               if (progress) {
      /* replace atomic_uint uniforms with ssbo's: */
   unsigned replaced = 0;
   nir_foreach_uniform_variable_safe(var, shader) {
                                                                              ssbo = nir_variable_create(shader, nir_var_mem_ssbo, type, name);
                  /* We can't use num_abos, because it only represents the number of
   * active atomic counters, and currently unlike SSBO's they aren't
   * compacted so num_abos actually isn't a bound on the index passed
   * to nir_intrinsic_atomic_counter_*. e.g. if we have a single atomic
   * counter declared like:
   *
   * layout(binding=1) atomic_uint counter0;
   *
   * then when we lower accesses to it the atomic_counter_* intrinsics
   * will have 1 as the index but num_abos will still be 1.
   */
                  struct glsl_struct_field field = {
      .type = type,
                     ssbo->interface_type =
                                                }
