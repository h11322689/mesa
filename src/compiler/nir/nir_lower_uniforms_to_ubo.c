   /*
   * Copyright 2017 Advanced Micro Devices, Inc.
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
      /*
   * Remap load_uniform intrinsics to nir_load_ubo or nir_load_ubo_vec4 accesses
   * of UBO binding point 0. Simultaneously, remap existing UBO accesses by
   * increasing their binding point by 1.
   *
   * For PIPE_CAP_PACKED_UNIFORMS, dword_packed should be set to indicate that
   * nir_intrinsic_load_uniform is in increments of dwords instead of vec4s.
   *
   * If load_vec4 is set, then nir_intrinsic_load_ubo_vec4 will be generated
   * instead of nir_intrinsic_load_ubo, saving addressing math for hardawre
   * needing aligned vec4 loads in increments of vec4s (such as TGSI CONST file
   * loads).
   */
      #include "nir.h"
   #include "nir_builder.h"
      struct nir_lower_uniforms_to_ubo_state {
      bool dword_packed;
      };
      static bool
   nir_lower_uniforms_to_ubo_instr(nir_builder *b, nir_instr *instr, void *data)
   {
               if (instr->type != nir_instr_type_intrinsic)
                              /* Increase all UBO binding points by 1. */
   if (intr->intrinsic == nir_intrinsic_load_ubo &&
      !b->shader->info.first_ubo_is_default_ubo) {
   nir_def *old_idx = intr->src[0].ssa;
   nir_def *new_idx = nir_iadd_imm(b, old_idx, 1);
   nir_src_rewrite(&intr->src[0], new_idx);
               if (intr->intrinsic == nir_intrinsic_load_uniform) {
      nir_def *ubo_idx = nir_imm_int(b, 0);
            assert(intr->def.bit_size >= 8);
   nir_def *load_result;
   if (state->load_vec4) {
      /* No asking us to generate load_vec4 when you've packed your uniforms
   * as dwords instead of vec4s.
   */
   assert(!state->dword_packed);
   load_result = nir_load_ubo_vec4(b, intr->num_components, intr->def.bit_size,
      } else {
      /* For PIPE_CAP_PACKED_UNIFORMS, the uniforms are packed with the
   * base/offset in dword units instead of vec4 units.
   */
   int multiplier = state->dword_packed ? 4 : 16;
   load_result = nir_load_ubo(b, intr->num_components, intr->def.bit_size,
                              /* If it's const, set the alignment to our known constant offset.  If
   * not, set it to a pessimistic value based on the multiplier (or the
   * scalar size, for qword loads).
   *
   * We could potentially set up stricter alignments for indirects by
   * knowing what features are enabled in the APIs (see comment in
   * nir_lower_ubo_vec4.c)
   */
   if (nir_src_is_const(intr->src[0])) {
      nir_intrinsic_set_align(load, NIR_ALIGN_MUL_MAX,
                  } else {
                  nir_intrinsic_set_range_base(load, nir_intrinsic_base(intr) * multiplier);
      }
            nir_instr_remove(&intr->instr);
                  }
      bool
   nir_lower_uniforms_to_ubo(nir_shader *shader, bool dword_packed, bool load_vec4)
   {
               struct nir_lower_uniforms_to_ubo_state state = {
      .dword_packed = dword_packed,
               progress = nir_shader_instructions_pass(shader,
                              if (progress) {
      if (!shader->info.first_ubo_is_default_ubo) {
      nir_foreach_variable_with_modes(var, shader, nir_var_mem_ubo) {
      var->data.binding++;
   if (var->data.driver_location != -1)
         /* only increment location for ubo arrays */
   if (glsl_without_array(var->type) == var->interface_type &&
      glsl_type_is_array(var->type))
      }
            if (shader->num_uniforms > 0) {
      const struct glsl_type *type = glsl_array_type(glsl_vec4_type(),
         nir_variable *ubo = nir_variable_create(shader, nir_var_mem_ubo, type,
                        struct glsl_struct_field field = {
      .type = type,
   .name = "data",
      };
   ubo->interface_type =
      glsl_interface_type(&field, 1, GLSL_INTERFACE_PACKING_STD430,
               shader->info.first_ubo_is_default_ubo = true;
      }
