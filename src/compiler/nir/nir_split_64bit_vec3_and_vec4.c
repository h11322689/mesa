   /*
   * Copyright © 2022 Collabora Ltd
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
   *    Gert Wollny <gert.wollny@collabora.com>
   */
      #include "nir.h"
   #include "nir_builder.h"
      #include "util/hash_table.h"
   #include "nir_deref.h"
      /* This pass splits stores to and loads from 64 bit vec3
   * and vec4 local variables to use at most vec2, and it also
   * splits phi nodes accordingly.
   *
   * Arrays of vec3 and vec4 are handled directly, arrays of arrays
   * are lowered to arrays on the fly.
   */
      static bool
   nir_split_64bit_vec3_and_vec4_filter(const nir_instr *instr,
         {
      switch (instr->type) {
   case nir_instr_type_intrinsic: {
               switch (intr->intrinsic) {
   case nir_intrinsic_load_deref: {
      if (intr->def.bit_size != 64)
         nir_variable *var = nir_intrinsic_get_var(intr, 0);
   if (var->data.mode != nir_var_function_temp)
            }
   case nir_intrinsic_store_deref: {
      if (nir_src_bit_size(intr->src[1]) != 64)
         nir_variable *var = nir_intrinsic_get_var(intr, 0);
   if (var->data.mode != nir_var_function_temp)
            default:
         }
      }
   case nir_instr_type_phi: {
      nir_phi_instr *phi = nir_instr_as_phi(instr);
   if (phi->def.bit_size != 64)
                     default:
            }
      typedef struct {
      nir_variable *xy;
      } variable_pair;
      static nir_def *
   merge_to_vec3_or_vec4(nir_builder *b, nir_def *load1,
         {
               if (load2->num_components == 1)
      return nir_vec3(b, nir_channel(b, load1, 0),
            else
      return nir_vec4(b, nir_channel(b, load1, 0),
               }
      static nir_def *
   get_linear_array_offset(nir_builder *b, nir_deref_instr *deref)
   {
      nir_deref_path path;
            nir_def *offset = nir_imm_intN_t(b, 0, deref->def.bit_size);
   for (nir_deref_instr **p = &path.path[1]; *p; p++) {
      switch ((*p)->deref_type) {
   case nir_deref_type_array: {
      nir_def *index = (*p)->arr.index.ssa;
   int stride = glsl_array_size((*p)->type);
   if (stride >= 0)
         else
            }
   default:
            }
   nir_deref_path_finish(&path);
      }
      static variable_pair *
   get_var_pair(nir_builder *b, nir_variable *old_var,
         {
      variable_pair *new_var = NULL;
   unsigned old_components = glsl_get_components(
                     struct hash_entry *entry = _mesa_hash_table_search(split_vars, old_var);
   if (!entry) {
      new_var = (variable_pair *)calloc(1, sizeof(variable_pair));
   new_var->xy = nir_variable_clone(old_var, b->shader);
   new_var->zw = nir_variable_clone(old_var, b->shader);
   new_var->xy->type = glsl_dvec_type(2);
            if (glsl_type_is_array_or_matrix(old_var->type)) {
      const struct glsl_type *element_type = glsl_without_array(old_var->type);
   unsigned array_size = glsl_get_aoa_size(old_var->type) * glsl_get_matrix_columns(element_type);
   new_var->xy->type = glsl_array_type(new_var->xy->type,
         new_var->zw->type = glsl_array_type(new_var->zw->type,
               exec_list_push_tail(&b->impl->locals, &new_var->xy->node);
               } else
            }
      static nir_def *
   split_load_deref(nir_builder *b, nir_intrinsic_instr *intr,
         {
      nir_variable *old_var = nir_intrinsic_get_var(intr, 0);
   unsigned old_components = glsl_get_components(
                     nir_deref_instr *deref1 = nir_build_deref_var(b, vars->xy);
            if (offset) {
      deref1 = nir_build_deref_array(b, deref1, offset);
               nir_def *load1 = nir_build_load_deref(b, 2, 64, &deref1->def, 0);
               }
      static nir_def *
   split_store_deref(nir_builder *b, nir_intrinsic_instr *intr,
         {
                        nir_deref_instr *deref_xy = nir_build_deref_var(b, vars->xy);
            if (offset) {
      deref_xy = nir_build_deref_array(b, deref_xy, offset);
               int write_mask_xy = nir_intrinsic_write_mask(intr) & 3;
   if (write_mask_xy) {
      nir_def *src_xy = nir_trim_vector(b, intr->src[1].ssa, 2);
               int write_mask_zw = nir_intrinsic_write_mask(intr) & 0xc;
   if (write_mask_zw) {
      nir_def *src_zw = nir_channels(b, intr->src[1].ssa,
                        }
      static nir_def *
   split_phi(nir_builder *b, nir_phi_instr *phi)
   {
               nir_alu_instr *vec = nir_alu_instr_create(b->shader, vec_op);
   nir_def_init(&vec->instr, &vec->def,
                              for (unsigned i = 0; i < 2; i++) {
      new_phi[i] = nir_phi_instr_create(b->shader);
   nir_def_init(&new_phi[i]->instr, &new_phi[i]->def, num_comp[i],
            nir_foreach_phi_src(src, phi) {
      /* Insert at the end of the predecessor but before the jump
                  if (pred_last_instr && pred_last_instr->type == nir_instr_type_jump)
                                          }
               b->cursor = nir_after_instr(&phi->instr);
      };
      static nir_def *
   nir_split_64bit_vec3_and_vec4_impl(nir_builder *b, nir_instr *instr, void *d)
   {
                                    nir_intrinsic_instr *intr = nir_instr_as_intrinsic(instr);
            case nir_intrinsic_load_deref: {
      nir_deref_instr *deref =
         if (deref->deref_type == nir_deref_type_var)
         else if (deref->deref_type == nir_deref_type_array) {
         } else
               case nir_intrinsic_store_deref: {
      nir_deref_instr *deref =
         if (deref->deref_type == nir_deref_type_var)
         else if (deref->deref_type == nir_deref_type_array)
         else
               default:
                     case nir_instr_type_phi: {
      nir_phi_instr *phi = nir_instr_as_phi(instr);
               default:
                     }
      bool
   nir_split_64bit_vec3_and_vec4(nir_shader *sh)
   {
               bool progress =
      nir_shader_lower_instructions(sh,
                     _mesa_hash_table_destroy(split_vars, NULL);
      }
