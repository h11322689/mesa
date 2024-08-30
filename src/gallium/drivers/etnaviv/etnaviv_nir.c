   /*
   * Copyright (c) 2019 Zodiac Inflight Innovations
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sub license,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the
   * next paragraph) shall be included in all copies or substantial portions
   * of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   * DEALINGS IN THE SOFTWARE.
   *
   * Authors:
   *    Jonathan Marek <jonathan@marek.ca>
   */
      #include "etnaviv_nir.h"
      /* io related lowering
   * run after lower_int_to_float because it adds i2f/f2i ops
   */
   void
   etna_lower_io(nir_shader *shader, struct etna_shader_variant *v)
   {
      nir_foreach_function_impl(impl, shader) {
               nir_foreach_block(block, impl) {
      nir_foreach_instr_safe(instr, block) {
                        switch (intr->intrinsic) {
   case nir_intrinsic_load_front_face: {
      /* HW front_face is 0.0/1.0, not 0/~0u for bool
                                                      nir_def_rewrite_uses_after(&intr->def,
            } break;
   case nir_intrinsic_store_deref: {
                                                                  nir_def *ssa = nir_mov(&b, intr->src[1].ssa);
   nir_alu_instr *alu = nir_instr_as_alu(ssa->parent_instr);
   alu->src[0].swizzle[0] = 2;
   alu->src[0].swizzle[2] = 0;
      } break;
   case nir_intrinsic_load_vertex_id:
   case nir_intrinsic_load_instance_id:
      /* detect use of vertex_id/instance_id */
   v->vs_id_in_reg = v->infile.num_reg;
      default:
                                    nir_tex_instr *tex = nir_instr_as_tex(instr);
   nir_src *coord = NULL;
                           for (unsigned i = 0; i < tex->num_srcs; i++) {
      switch (tex->src[i].src_type) {
   case nir_tex_src_coord:
      coord = &tex->src[i].src;
      case nir_tex_src_bias:
   case nir_tex_src_lod:
      assert(!src1);
   src1 = &tex->src[i].src;
   src1_idx = i;
      case nir_tex_src_ddx:
   case nir_tex_src_ddy:
   case nir_tex_src_comparator:
         default:
      assert(0);
                                                   nir_alu_instr *vec = nir_alu_instr_create(shader, nir_op_vec4);
   for (unsigned i = 0; i < tex->coord_components; i++) {
      vec->src[i].src = nir_src_for_ssa(coord->ssa);
      }
                           nir_tex_instr_remove_src(tex, src1_idx);
                              }
      static void
   etna_lower_alu_impl(nir_function_impl *impl, bool has_new_transcendentals)
   {
                        /* in a seperate loop so we can apply the multiple-uniform logic to the new fmul */
   nir_foreach_block(block, impl) {
      nir_foreach_instr_safe(instr, block) {
                     nir_alu_instr *alu = nir_instr_as_alu(instr);
   /* multiply sin/cos src by constant
   * TODO: do this earlier (but it breaks const_prop opt)
   */
                     nir_def *imm = has_new_transcendentals ?
                  nir_src_rewrite(&alu->src[0].src,
               /* change transcendental ops to vec2 and insert vec1 mul for the result
   * TODO: do this earlier (but it breaks with optimizations)
   */
   if (has_new_transcendentals && (
      alu->op == nir_op_fdiv || alu->op == nir_op_flog2 ||
                           nir_alu_instr *mul = nir_alu_instr_create(shader, nir_op_fmul);
                                                   nir_def_rewrite_uses_after(ssa, &mul->def,
               }
      void
   etna_lower_alu(nir_shader *shader, bool has_new_transcendentals)
   {
      nir_foreach_function_impl(impl, shader) {
            }
