   /*
   * Copyright (c) 2020 Lima Project
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
   #include "lima_ir.h"
      static bool
   lima_nir_duplicate_load_const(nir_builder *b, nir_load_const_instr *load)
   {
      nir_load_const_instr *last_dupl = NULL;
            nir_foreach_use_safe(use_src, &load->def) {
               if (last_parent_instr != nir_src_parent_instr(use_src)) {
                     dupl = nir_load_const_instr_create(b->shader, load->def.num_components,
                  dupl->instr.pass_flags = 1;
      }
   else {
                  nir_src_rewrite(use_src, &dupl->def);
   last_parent_instr = nir_src_parent_instr(use_src);
               last_dupl = NULL;
            nir_foreach_if_use_safe(use_src, &load->def) {
      nir_load_const_instr *dupl;
            if (last_parent_if != nif) {
                     dupl = nir_load_const_instr_create(b->shader, load->def.num_components,
                  dupl->instr.pass_flags = 1;
      }
   else {
                  nir_src_rewrite(&nir_src_parent_if(use_src)->condition, &dupl->def);
   last_parent_if = nif;
               nir_instr_remove(&load->instr);
      }
      static void
   lima_nir_duplicate_load_consts_impl(nir_shader *shader, nir_function_impl *impl)
   {
               nir_foreach_block(block, impl) {
      nir_foreach_instr(instr, block) {
                  nir_foreach_instr_safe(instr, block) {
                                                            nir_metadata_preserve(impl, nir_metadata_block_index |
      }
      /* Duplicate load consts for every user.
   * Helps by utilizing the load const instruction slots that would
   * otherwise stay empty, and reduces register pressure. */
   void
   lima_nir_duplicate_load_consts(nir_shader *shader)
   {
      nir_foreach_function_impl(impl, shader) {
            }
