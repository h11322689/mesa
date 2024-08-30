   /*
   * Copyright © 2023 Intel Corporation
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
      #include "compiler/nir/nir_builder.h"
   #include "util/u_dynarray.h"
      #include "brw_nir.h"
      static bool
   nir_instr_is_resource_intel(nir_instr *instr)
   {
      return instr->type == nir_instr_type_intrinsic &&
      }
      static bool
   add_src_instr(nir_src *src, void *state)
   {
      struct util_dynarray *inst_array = state;
   util_dynarray_foreach(inst_array, nir_instr *, instr_ptr) {
      if (*instr_ptr == src->ssa->parent_instr)
                           }
      static nir_intrinsic_instr *
   find_resource_intel(struct util_dynarray *inst_array,
         {
      /* If resouce_intel is already directly in front of the instruction, there
   * is nothing to do.
   */
   if (nir_instr_is_resource_intel(def->parent_instr))
                     unsigned idx = 0, scan_index = 0;
   while (idx < util_dynarray_num_elements(inst_array, nir_instr *)) {
               for (; scan_index < util_dynarray_num_elements(inst_array, nir_instr *); scan_index++) {
      nir_instr *scan_instr = *util_dynarray_element(inst_array, nir_instr *, scan_index);
   if (nir_instr_is_resource_intel(scan_instr))
                              }
      static bool
   brw_nir_lower_non_uniform_intrinsic(nir_builder *b,
               {
      unsigned source;
   switch (intrin->intrinsic) {
   case nir_intrinsic_load_ubo:
   case nir_intrinsic_load_ssbo:
   case nir_intrinsic_get_ssbo_size:
   case nir_intrinsic_ssbo_atomic:
   case nir_intrinsic_ssbo_atomic_swap:
   case nir_intrinsic_load_ssbo_block_intel:
   case nir_intrinsic_store_ssbo_block_intel:
   case nir_intrinsic_load_ubo_uniform_block_intel:
   case nir_intrinsic_load_ssbo_uniform_block_intel:
   case nir_intrinsic_image_load_raw_intel:
   case nir_intrinsic_image_store_raw_intel:
   case nir_intrinsic_image_load:
   case nir_intrinsic_image_store:
   case nir_intrinsic_image_atomic:
   case nir_intrinsic_image_atomic_swap:
   case nir_intrinsic_bindless_image_load:
   case nir_intrinsic_bindless_image_store:
   case nir_intrinsic_bindless_image_atomic:
   case nir_intrinsic_bindless_image_atomic_swap:
   case nir_intrinsic_image_size:
   case nir_intrinsic_bindless_image_size:
      source = 0;
         case nir_intrinsic_store_ssbo:
      source = 1;
         default:
                                    nir_intrinsic_instr *old_resource_intel =
         if (old_resource_intel == NULL)
            nir_instr *new_instr =
                     nir_intrinsic_instr *new_resource_intel =
            nir_src_rewrite(&new_resource_intel->src[1], intrin->src[source].ssa);
               }
      static bool
   brw_nir_lower_non_uniform_tex(nir_builder *b,
               {
               bool progress = false;
   for (unsigned s = 0; s < tex->num_srcs; s++) {
      if (tex->src[s].src_type != nir_tex_src_texture_handle &&
                           nir_intrinsic_instr *old_resource_intel =
         if (old_resource_intel == NULL)
            nir_instr *new_instr =
                     nir_intrinsic_instr *new_resource_intel =
            nir_src_rewrite(&new_resource_intel->src[1], tex->src[s].src.ssa);
                           }
      static bool
   brw_nir_lower_non_uniform_instr(nir_builder *b,
               {
               switch (instr->type) {
   case nir_instr_type_intrinsic:
      return brw_nir_lower_non_uniform_intrinsic(b,
               case nir_instr_type_tex:
      return brw_nir_lower_non_uniform_tex(b,
               default:
            }
      /** This pass rematerializes resource_intel intrinsics closer to their use.
   *
   * For example will turn this :
   *    ssa_1 = iadd ...
   *    ssa_2 = resource_intel ..., ssa_1, ...
   *    ssa_3 = read_first_invocation ssa_2
   *    ssa_4 = load_ssbo ssa_3, ...
   *
   * into this :
   *    ssa_1 = iadd ...
   *    ssa_3 = read_first_invocation ssa_1
   *    ssa_5 = resource_intel ..., ssa_3, ...
   *    ssa_4 = load_ssbo ssa_5, ...
   *
   * The goal is to have the resource_intel immediately before its use so that
   * the backend compiler can know how the load_ssbo should be compiled (binding
   * table or bindless access, etc...).
   */
   bool
   brw_nir_lower_non_uniform_resource_intel(nir_shader *shader)
   {
               struct util_dynarray inst_array;
            bool ret = nir_shader_instructions_pass(shader,
                                          }
      static bool
   skip_resource_intel_cleanup(nir_instr *instr)
   {
      switch (instr->type) {
   case nir_instr_type_tex:
            case nir_instr_type_intrinsic: {
      nir_intrinsic_instr *intrin =
         switch (intrin->intrinsic) {
   case nir_intrinsic_load_ubo:
   case nir_intrinsic_load_ssbo:
   case nir_intrinsic_store_ssbo:
   case nir_intrinsic_get_ssbo_size:
   case nir_intrinsic_ssbo_atomic:
   case nir_intrinsic_ssbo_atomic_swap:
   case nir_intrinsic_load_ssbo_block_intel:
   case nir_intrinsic_store_ssbo_block_intel:
   case nir_intrinsic_load_ssbo_uniform_block_intel:
   case nir_intrinsic_image_load_raw_intel:
   case nir_intrinsic_image_store_raw_intel:
   case nir_intrinsic_image_load:
   case nir_intrinsic_image_store:
   case nir_intrinsic_image_atomic:
   case nir_intrinsic_image_atomic_swap:
   case nir_intrinsic_bindless_image_load:
   case nir_intrinsic_bindless_image_store:
   case nir_intrinsic_bindless_image_atomic:
   case nir_intrinsic_bindless_image_atomic_swap:
   case nir_intrinsic_image_size:
   case nir_intrinsic_bindless_image_size:
            default:
                     default:
            }
      static bool
   brw_nir_cleanup_resource_intel_instr(nir_builder *b,
               {
      if (intrin->intrinsic != nir_intrinsic_resource_intel)
            bool progress = false;
   nir_foreach_use_safe(src, &intrin->def) {
      if (!nir_src_is_if(src) && skip_resource_intel_cleanup(nir_src_parent_instr(src)))
            progress = true;
                  }
      /** This pass removes unnecessary resource_intel intrinsics
   *
   * This pass must not be run before brw_nir_lower_non_uniform_resource_intel.
   */
   bool
   brw_nir_cleanup_resource_intel(nir_shader *shader)
   {
               bool ret = nir_shader_intrinsics_pass(shader,
                                          }
