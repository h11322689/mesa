   /* -*- mesa-c++  -*-
   *
   * Copyright (c) 2021 Collabora LTD
   *
   * Author: Gert Wollny <gert.wollny@collabora.com>
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * on the rights to use, copy, modify, merge, publish, distribute, sub
   * license, and/or sell copies of the Software, and to permit persons to whom
   * the Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the next
   * paragraph) shall be included in all copies or substantial portions of the
   * Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
   * THE AUTHOR(S) AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
   * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
   * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
   * USE OR OTHER DEALINGS IN THE SOFTWARE.
   */
      #include "nir.h"
   #include "nir_builder.h"
   #include "nir_intrinsics.h"
   #include "nir_intrinsics_indices.h"
   #include "sfn_nir.h"
      static nir_def *
   r600_legalize_image_load_store_impl(nir_builder *b,
               {
      b->cursor = nir_before_instr(instr);
                                       if (load_value)
      default_value =
         auto image_exists =
                                          /*  Image exists start */
   auto new_index =
      nir_umin(b,
                              unsigned num_components = 2;
   switch (dim) {
   case GLSL_SAMPLER_DIM_BUF:
   case GLSL_SAMPLER_DIM_1D:
      num_components = 1;
      case GLSL_SAMPLER_DIM_2D:
   case GLSL_SAMPLER_DIM_MS:
   case GLSL_SAMPLER_DIM_RECT:
   case GLSL_SAMPLER_DIM_CUBE:
      num_components = 2;
      case GLSL_SAMPLER_DIM_3D:
      num_components = 3;
      default:
                  if (num_components < 3 && nir_intrinsic_image_array(ir))
            auto img_size = nir_image_size(b,
                                 num_components,
   32,
   ir->src[0].ssa,
            unsigned mask = (1 << num_components) - 1;
   unsigned num_src1_comp = MIN2(ir->src[1].ssa->num_components, num_components);
            auto in_range = nir_ult(b,
                  switch (num_components) {
   case 2:
      in_range = nir_iand(b, nir_channel(b, in_range, 0), nir_channel(b, in_range, 1));
      case 3: {
      auto tmp = nir_iand(b, nir_channel(b, in_range, 0), nir_channel(b, in_range, 1));
   in_range = nir_iand(b, tmp, nir_channel(b, in_range, 2));
      }
            /*  Access is in range start */
               auto new_load = nir_instr_clone(b->shader, instr);
                     if (load_value)
            if (ir->intrinsic != nir_intrinsic_image_size) {
      /*  Access is out of range start */
            nir_pop_if(b, load_else);
            if (load_value)
               /* Start image doesn't exists */
            /* Nothing to do, default is already set */
            if (load_value)
            if (load_value)
         else
               }
      static bool
   r600_legalize_image_load_store_filter(const nir_instr *instr, UNUSED const void *_options)
   {
      if (instr->type != nir_instr_type_intrinsic)
            auto ir = nir_instr_as_intrinsic(instr);
   switch (ir->intrinsic) {
   case nir_intrinsic_image_store:
   case nir_intrinsic_image_load:
   case nir_intrinsic_image_atomic:
   case nir_intrinsic_image_atomic_swap:
   case nir_intrinsic_image_size:
         default:
            }
      /* This pass makes sure only existing images are accessd and
   * the access is within range, if not zero is returned by all
   * image ops that return a value.
   */
   bool
   r600_legalize_image_load_store(nir_shader *shader)
   {
      bool progress = nir_shader_lower_instructions(shader,
                        };
