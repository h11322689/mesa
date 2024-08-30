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
      #include <math.h>
   #include "nir.h"
   #include "nir_builder.h"
   #include "nir_constant_expressions.h"
   #include "nir_deref.h"
      /*
   * Implements SSA-based constant folding.
   */
      struct constant_fold_state {
      bool has_load_constant;
      };
      static bool
   try_fold_alu(nir_builder *b, nir_alu_instr *alu)
   {
               /* In the case that any outputs/inputs have unsized types, then we need to
   * guess the bit-size. In this case, the validator ensures that all
   * bit-sizes match so we can just take the bit-size from first
   * output/input with an unsized type. If all the outputs/inputs are sized
   * then we don't need to guess the bit-size at all because the code we
   * generate for constant opcodes in this case already knows the sizes of
   * the types involved and does not need the provided bit-size for anything
   * (although it still requires to receive a valid bit-size).
   */
   unsigned bit_size = 0;
   if (!nir_alu_type_get_type_size(nir_op_infos[alu->op].output_type))
            for (unsigned i = 0; i < nir_op_infos[alu->op].num_inputs; i++) {
      if (bit_size == 0 &&
                           if (src_instr->type != nir_instr_type_load_const)
                  for (unsigned j = 0; j < nir_ssa_alu_instr_src_components(alu, i);
      j++) {
                  if (bit_size == 0)
            nir_const_value dest[NIR_MAX_VEC_COMPONENTS];
   nir_const_value *srcs[NIR_MAX_VEC_COMPONENTS];
   memset(dest, 0, sizeof(dest));
   for (unsigned i = 0; i < nir_op_infos[alu->op].num_inputs; ++i)
         nir_eval_const_opcode(alu->op, dest, alu->def.num_components,
                  b->cursor = nir_before_instr(&alu->instr);
   nir_def *imm = nir_build_imm(b, alu->def.num_components,
               nir_def_rewrite_uses(&alu->def, imm);
   nir_instr_remove(&alu->instr);
               }
      static nir_const_value *
   const_value_for_deref(nir_deref_instr *deref)
   {
      if (!nir_deref_mode_is(deref, nir_var_mem_constant))
            nir_deref_path path;
   nir_deref_path_init(&path, deref, NULL);
   if (path.path[0]->deref_type != nir_deref_type_var)
            nir_variable *var = path.path[0]->var;
   assert(var->data.mode == nir_var_mem_constant);
   if (var->constant_initializer == NULL)
            if (var->constant_initializer->is_null_constant) {
      /* Doesn't matter what casts are in the way, it's all zeros */
   nir_deref_path_finish(&path);
               nir_constant *c = var->constant_initializer;
            for (unsigned i = 1; path.path[i] != NULL; i++) {
      nir_deref_instr *p = path.path[i];
   switch (p->deref_type) {
   case nir_deref_type_var:
            case nir_deref_type_array: {
      assert(v == NULL);
                  uint64_t idx = nir_src_as_uint(p->arr.index);
   if (c->num_elements > 0) {
      assert(glsl_type_is_array(path.path[i - 1]->type));
   if (idx >= c->num_elements)
            } else {
      assert(glsl_type_is_vector(path.path[i - 1]->type));
   assert(glsl_type_is_scalar(p->type));
   if (idx >= NIR_MAX_VEC_COMPONENTS)
            }
               case nir_deref_type_struct:
      assert(glsl_type_is_struct(path.path[i - 1]->type));
   assert(v == NULL && c->num_elements > 0);
   if (p->strct.index >= c->num_elements)
                     default:
                     /* We have to have ended at a vector */
   assert(c->num_elements == 0);
   nir_deref_path_finish(&path);
         fail:
      nir_deref_path_finish(&path);
      }
      static bool
   try_fold_intrinsic(nir_builder *b, nir_intrinsic_instr *intrin,
         {
      switch (intrin->intrinsic) {
   case nir_intrinsic_demote_if:
   case nir_intrinsic_discard_if:
   case nir_intrinsic_terminate_if:
      if (nir_src_is_const(intrin->src[0])) {
      if (nir_src_as_bool(intrin->src[0])) {
      b->cursor = nir_before_instr(&intrin->instr);
   nir_intrinsic_op op;
   switch (intrin->intrinsic) {
   case nir_intrinsic_discard_if:
      op = nir_intrinsic_discard;
      case nir_intrinsic_demote_if:
      op = nir_intrinsic_demote;
      case nir_intrinsic_terminate_if:
      op = nir_intrinsic_terminate;
      default:
         }
   nir_intrinsic_instr *new_instr =
            }
   nir_instr_remove(&intrin->instr);
      }
         case nir_intrinsic_load_deref: {
      nir_deref_instr *deref = nir_src_as_deref(intrin->src[0]);
   nir_const_value *v = const_value_for_deref(deref);
   if (v) {
      b->cursor = nir_before_instr(&intrin->instr);
   nir_def *val = nir_build_imm(b, intrin->def.num_components,
         nir_def_rewrite_uses(&intrin->def, val);
   nir_instr_remove(&intrin->instr);
      }
               case nir_intrinsic_load_constant: {
               if (!nir_src_is_const(intrin->src[0])) {
      state->has_indirect_load_const = true;
               unsigned offset = nir_src_as_uint(intrin->src[0]);
   unsigned base = nir_intrinsic_base(intrin);
   unsigned range = nir_intrinsic_range(intrin);
            b->cursor = nir_before_instr(&intrin->instr);
   nir_def *val;
   if (offset >= range) {
      val = nir_undef(b, intrin->def.num_components,
      } else {
      nir_const_value imm[NIR_MAX_VEC_COMPONENTS];
   memset(imm, 0, sizeof(imm));
   uint8_t *data = (uint8_t *)b->shader->constant_data + base;
   for (unsigned i = 0; i < intrin->num_components; i++) {
                     memcpy(&imm[i].u64, data + offset, bytes);
      }
   val = nir_build_imm(b, intrin->def.num_components,
      }
   nir_def_rewrite_uses(&intrin->def, val);
   nir_instr_remove(&intrin->instr);
               case nir_intrinsic_vote_any:
   case nir_intrinsic_vote_all:
   case nir_intrinsic_read_invocation:
   case nir_intrinsic_read_first_invocation:
   case nir_intrinsic_shuffle:
   case nir_intrinsic_shuffle_xor:
   case nir_intrinsic_shuffle_up:
   case nir_intrinsic_shuffle_down:
   case nir_intrinsic_quad_broadcast:
   case nir_intrinsic_quad_swap_horizontal:
   case nir_intrinsic_quad_swap_vertical:
   case nir_intrinsic_quad_swap_diagonal:
   case nir_intrinsic_quad_swizzle_amd:
   case nir_intrinsic_masked_swizzle_amd:
      /* All of these have the data payload in the first source.  They may
   * have a second source with a shuffle index but that doesn't matter if
   * the data is constant.
   */
   if (nir_src_is_const(intrin->src[0])) {
      nir_def_rewrite_uses(&intrin->def,
         nir_instr_remove(&intrin->instr);
      }
         case nir_intrinsic_vote_feq:
   case nir_intrinsic_vote_ieq:
      if (nir_src_is_const(intrin->src[0])) {
      b->cursor = nir_before_instr(&intrin->instr);
   nir_def_rewrite_uses(&intrin->def,
         nir_instr_remove(&intrin->instr);
      }
         default:
            }
      static bool
   try_fold_txb_to_tex(nir_builder *b, nir_tex_instr *tex)
   {
                        /* nir_to_tgsi_lower_tex mangles many kinds of texture instructions,
   * including txb, into invalid states.  It removes the special
   * parameters and appends the values to the texture coordinate.
   */
   if (bias_idx < 0)
            if (nir_src_is_const(tex->src[bias_idx].src) &&
      nir_src_as_float(tex->src[bias_idx].src) == 0.0) {
   nir_tex_instr_remove_src(tex, bias_idx);
   tex->op = nir_texop_tex;
                  }
      static bool
   try_fold_tex_offset(nir_tex_instr *tex, unsigned *index,
         {
      const int src_idx = nir_tex_instr_src_index(tex, src_type);
   if (src_idx < 0)
            if (!nir_src_is_const(tex->src[src_idx].src))
            *index += nir_src_as_uint(tex->src[src_idx].src);
               }
      static bool
   try_fold_texel_offset_src(nir_tex_instr *tex)
   {
      int offset_src = nir_tex_instr_src_index(tex, nir_tex_src_offset);
   if (offset_src < 0)
            unsigned size = nir_tex_instr_src_size(tex, offset_src);
            for (unsigned i = 0; i < size; i++) {
      nir_scalar comp = nir_scalar_resolved(src->src.ssa, i);
   if (!nir_scalar_is_const(comp) || nir_scalar_as_uint(comp) != 0)
                           }
      static bool
   try_fold_tex(nir_builder *b, nir_tex_instr *tex)
   {
               progress |= try_fold_tex_offset(tex, &tex->texture_index,
         progress |= try_fold_tex_offset(tex, &tex->sampler_index,
            /* txb with a bias of constant zero is just tex. */
   if (tex->op == nir_texop_txb)
            /* tex with a zero offset is just tex. */
               }
      static bool
   try_fold_instr(nir_builder *b, nir_instr *instr, void *_state)
   {
      switch (instr->type) {
   case nir_instr_type_alu:
         case nir_instr_type_intrinsic:
         case nir_instr_type_tex:
         default:
      /* Don't know how to constant fold */
         }
      bool
   nir_opt_constant_folding(nir_shader *shader)
   {
      struct constant_fold_state state;
   state.has_load_constant = false;
            bool progress = nir_shader_instructions_pass(shader, try_fold_instr,
                        /* This doesn't free the constant data if there are no constant loads because
   * the data might still be used but the loads have been lowered to load_ubo
   */
   if (state.has_load_constant && !state.has_indirect_load_const &&
      shader->constant_data_size) {
   ralloc_free(shader->constant_data);
   shader->constant_data = NULL;
                  }
