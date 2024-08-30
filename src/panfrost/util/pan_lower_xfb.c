   /*
   * Copyright (C) 2022 Collabora Ltd.
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
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   * SOFTWARE.
   */
      #include "compiler/nir/nir_builder.h"
   #include "pan_ir.h"
      static void
   lower_xfb_output(nir_builder *b, nir_intrinsic_instr *intr,
               {
      assert(buffer < MAX_XFB_BUFFERS);
            /* Transform feedback info in units of words, convert to bytes. */
   uint16_t stride = b->shader->info.xfb_stride[buffer] * 4;
                     nir_def *index = nir_iadd(
      b, nir_imul(b, nir_load_instance_id(b), nir_load_num_vertices(b)),
         BITSET_SET(b->shader->info.system_values_read,
                  nir_def *buf = nir_load_xfb_address(b, 64, .base = buffer);
   nir_def *addr = nir_iadd(
      b, buf,
         nir_def *src = intr->src[0].ssa;
   nir_def *value =
            }
      static bool
   lower_xfb(nir_builder *b, nir_intrinsic_instr *intr, UNUSED void *data)
   {
      /* In transform feedback programs, vertex ID becomes zero-based, so apply
   * that lowering even on Valhall.
   */
   if (intr->intrinsic == nir_intrinsic_load_vertex_id) {
               nir_def *repl =
            nir_def_rewrite_uses(&intr->def, repl);
               if (intr->intrinsic != nir_intrinsic_store_output)
                              for (unsigned i = 0; i < 2; ++i) {
      nir_io_xfb xfb =
         for (unsigned j = 0; j < 2; ++j) {
                     lower_xfb_output(b, intr, i * 2 + j, xfb.out[j].num_components,
                        nir_instr_remove(&intr->instr);
      }
      bool
   pan_lower_xfb(nir_shader *nir)
   {
      return nir_shader_intrinsics_pass(
      }
