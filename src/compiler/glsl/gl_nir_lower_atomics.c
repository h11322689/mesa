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
   *
   * Authors:
   *    Connor Abbott (cwabbott0@gmail.com)
   *
   */
      #include "compiler/nir/nir.h"
   #include "compiler/nir/nir_builder.h"
   #include "gl_nir.h"
   #include "ir_uniform.h"
   #include "main/config.h"
   #include "main/shader_types.h"
   #include <assert.h>
      /*
   * replace atomic counter intrinsics that use a variable with intrinsics
   * that directly store the buffer index and byte offset
   */
      static bool
   lower_deref_instr(nir_builder *b, nir_intrinsic_instr *instr,
               {
      nir_intrinsic_op op;
   switch (instr->intrinsic) {
   case nir_intrinsic_atomic_counter_read_deref:
      op = nir_intrinsic_atomic_counter_read;
         case nir_intrinsic_atomic_counter_inc_deref:
      op = nir_intrinsic_atomic_counter_inc;
         case nir_intrinsic_atomic_counter_pre_dec_deref:
      op = nir_intrinsic_atomic_counter_pre_dec;
         case nir_intrinsic_atomic_counter_post_dec_deref:
      op = nir_intrinsic_atomic_counter_post_dec;
         case nir_intrinsic_atomic_counter_add_deref:
      op = nir_intrinsic_atomic_counter_add;
         case nir_intrinsic_atomic_counter_min_deref:
      op = nir_intrinsic_atomic_counter_min;
         case nir_intrinsic_atomic_counter_max_deref:
      op = nir_intrinsic_atomic_counter_max;
         case nir_intrinsic_atomic_counter_and_deref:
      op = nir_intrinsic_atomic_counter_and;
         case nir_intrinsic_atomic_counter_or_deref:
      op = nir_intrinsic_atomic_counter_or;
         case nir_intrinsic_atomic_counter_xor_deref:
      op = nir_intrinsic_atomic_counter_xor;
         case nir_intrinsic_atomic_counter_exchange_deref:
      op = nir_intrinsic_atomic_counter_exchange;
         case nir_intrinsic_atomic_counter_comp_swap_deref:
      op = nir_intrinsic_atomic_counter_comp_swap;
         default:
                  nir_deref_instr *deref = nir_src_as_deref(instr->src[0]);
            if (var->data.mode != nir_var_uniform &&
      var->data.mode != nir_var_mem_ssbo &&
   var->data.mode != nir_var_mem_shared)
         const unsigned uniform_loc = var->data.location;
   const unsigned idx = use_binding_as_idx ? var->data.binding :
                     int offset_value = 0;
   int range_base = 0;
   if (!b->shader->options->lower_atomic_offset_to_range_base)
         else
            nir_def *offset = nir_imm_int(b, offset_value);
   for (nir_deref_instr *d = deref; d->deref_type != nir_deref_type_var;
      d = nir_deref_instr_parent(d)) {
            unsigned array_stride = ATOMIC_COUNTER_SIZE;
   if (glsl_type_is_array(d->type))
            offset = nir_iadd(b, offset, nir_imul(b, d->arr.index.ssa,
               /* Since the first source is a deref and the first source in the lowered
   * instruction is the offset, we can just swap it out and change the
   * opcode.
   */
   instr->intrinsic = op;
            nir_src_rewrite(&instr->src[0], offset);
                        }
      struct lower_atomics_data {
      bool use_binding_as_idx;
   nir_shader *shader;
      };
      static bool
   gl_nir_lower_atomics_instr(nir_builder *b, nir_instr *instr, void *cb_data)
   {
      if (instr->type != nir_instr_type_intrinsic)
                     return lower_deref_instr(b,
                        }
      bool
   gl_nir_lower_atomics(nir_shader *shader,
               {
      struct lower_atomics_data data = {
         .use_binding_as_idx = use_binding_as_idx,
   .shader = shader,
            return nir_shader_instructions_pass(shader, gl_nir_lower_atomics_instr,
                  }
