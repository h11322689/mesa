   /*
   * Copyright © 2019 Collabora, Ltd.
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
   * Authors (Collabora):
   *    Alyssa Rosenzweig <alyssa.rosenzweig@collabora.com>
   */
      #include "nir.h"
   #include "nir_builder.h"
      /*
   * Lowers SSBOs to globals, for hardware that lack native SSBO support. When
   * lowering, *_ssbo_* instructions will become *_global_* instructions,
   * augmented with load_ssbo_address.
   *
   * DOES NOT PERFORM BOUNDS CHECKING. DO NOT USE IN PRODUCTION ON UNTRUSTED
   * CONTEXTS INCLUDING WEBGL 2.
   */
      static nir_intrinsic_op
   lower_ssbo_op(nir_intrinsic_op op)
   {
      switch (op) {
   case nir_intrinsic_load_ssbo:
            case nir_intrinsic_store_ssbo:
            case nir_intrinsic_ssbo_atomic:
         case nir_intrinsic_ssbo_atomic_swap:
            default:
            }
      /* Like SSBO property sysvals, though SSBO index may be indirect. C.f.
   * nir_load_system_value */
      static inline nir_def *
   nir_load_ssbo_prop(nir_builder *b, nir_intrinsic_op op,
         {
      nir_intrinsic_instr *load = nir_intrinsic_instr_create(b->shader, op);
   load->num_components = 1;
   load->src[0] = nir_src_for_ssa(idx->ssa);
   nir_def_init(&load->instr, &load->def, 1, bitsize);
   nir_builder_instr_insert(b, &load->instr);
      }
      #define nir_ssbo_prop(b, prop, index, bitsize) \
            static nir_def *
   lower_ssbo_instr(nir_builder *b, nir_intrinsic_instr *intr)
   {
      nir_intrinsic_op op = lower_ssbo_op(intr->intrinsic);
   bool is_store = op == nir_intrinsic_store_global;
            /* We have to calculate the address:
   *
   * &(SSBO[offset]) = &SSBO + offset
            nir_src index = intr->src[is_store ? 1 : 0];
   nir_src *offset_src = nir_get_io_offset_src(intr);
            nir_def *address =
      nir_iadd(b,
                        nir_intrinsic_instr *global =
            global->num_components = intr->num_components;
            if (nir_intrinsic_has_atomic_op(intr))
            if (!is_atomic) {
      nir_intrinsic_set_align_mul(global, nir_intrinsic_align_mul(intr));
               if (is_store) {
      global->src[0] = nir_src_for_ssa(intr->src[0].ssa);
      } else {
      nir_def_init(&global->instr, &global->def,
            if (is_atomic) {
      global->src[1] = nir_src_for_ssa(intr->src[2].ssa);
   if (nir_intrinsic_infos[op].num_srcs > 2)
                  nir_builder_instr_insert(b, &global->instr);
      }
      static bool
   should_lower_ssbo_instr(const nir_instr *instr)
   {
      if (instr->type != nir_instr_type_intrinsic)
                     switch (intr->intrinsic) {
   case nir_intrinsic_load_ssbo:
   case nir_intrinsic_store_ssbo:
   case nir_intrinsic_ssbo_atomic:
   case nir_intrinsic_ssbo_atomic_swap:
         default:
                     }
      bool
   nir_lower_ssbo(nir_shader *shader)
   {
               nir_foreach_function_impl(impl, shader) {
               nir_foreach_block(block, impl) {
      nir_foreach_instr_safe(instr, block) {
      if (!should_lower_ssbo_instr(instr))
                                       if (replace) {
                                             }
