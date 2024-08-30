   /*
   * Copyright © 2020 Google LLC
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
      /**
   * @file
   *
   * Removes unused trailing components from store data.
   *
   */
      #include "nir.h"
   #include "nir_builder.h"
      static bool
   opt_shrink_vectors_image_store(nir_builder *b, nir_intrinsic_instr *instr)
   {
      enum pipe_format format;
   if (instr->intrinsic == nir_intrinsic_image_deref_store) {
      nir_deref_instr *deref = nir_src_as_deref(instr->src[0]);
      } else {
         }
   if (format == PIPE_FORMAT_NONE)
            unsigned components = util_format_get_nr_components(format);
   if (components >= instr->num_components)
            nir_def *data = nir_trim_vector(b, instr->src[3].ssa, components);
   nir_src_rewrite(&instr->src[3], data);
               }
      static bool
   opt_shrink_store_instr(nir_builder *b, nir_intrinsic_instr *instr, bool shrink_image_store)
   {
               switch (instr->intrinsic) {
   case nir_intrinsic_store_output:
   case nir_intrinsic_store_per_vertex_output:
   case nir_intrinsic_store_ssbo:
   case nir_intrinsic_store_shared:
   case nir_intrinsic_store_global:
   case nir_intrinsic_store_scratch:
         case nir_intrinsic_bindless_image_store:
   case nir_intrinsic_image_deref_store:
   case nir_intrinsic_image_store:
         default:
                  /* Must be a vectorized intrinsic that we can resize. */
            /* Trim the num_components stored according to the write mask. */
   unsigned write_mask = nir_intrinsic_write_mask(instr);
   unsigned last_bit = util_last_bit(write_mask);
   if (last_bit < instr->num_components) {
      nir_def *def = nir_trim_vector(b, instr->src[0].ssa, last_bit);
   nir_src_rewrite(&instr->src[0], def);
                           }
      bool
   nir_opt_shrink_stores(nir_shader *shader, bool shrink_image_store)
   {
               nir_foreach_function_impl(impl, shader) {
               nir_foreach_block(block, impl) {
      nir_foreach_instr(instr, block) {
      if (instr->type != nir_instr_type_intrinsic)
         nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(instr);
                  if (progress) {
      nir_metadata_preserve(impl,
            } else {
                        }
