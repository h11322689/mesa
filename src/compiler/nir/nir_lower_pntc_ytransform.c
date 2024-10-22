   /*
   * Copyright © 2020 Igalia S.L.
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
      #include "nir.h"
   #include "nir_builder.h"
      /* Lower gl_PointCoord to account for user requested point-coord origin
   * and for whether draw buffer is flipped.
   */
      typedef struct {
      const gl_state_index16 *pntc_state_tokens;
   nir_shader *shader;
   nir_builder b;
      } lower_pntc_ytransform_state;
      static nir_def *
   get_pntc_transform(lower_pntc_ytransform_state *state)
   {
      if (state->pntc_transform == NULL) {
      /* NOTE: name must be prefixed w/ "gl_" to trigger slot based
   * special handling in uniform setup:
   */
   nir_variable *var = nir_state_variable_create(state->shader,
                        var->data.how_declared = nir_var_hidden;
      }
      }
      static void
   lower_load_pointcoord(lower_pntc_ytransform_state *state,
         {
      nir_builder *b = &state->b;
            nir_def *pntc = &intr->def;
   nir_def *transform = get_pntc_transform(state);
   nir_def *y = nir_channel(b, pntc, 1);
   /* The offset is 1 if we're flipping, 0 otherwise. */
   nir_def *offset = nir_channel(b, transform, 1);
   /* Flip the sign of y if we're flipping. */
            /* Reassemble the vector. */
   nir_def *flipped_pntc = nir_vec2(b,
                  nir_def_rewrite_uses_after(&intr->def, flipped_pntc,
      }
      static void
   lower_pntc_ytransform_block(lower_pntc_ytransform_state *state,
         {
      nir_foreach_instr_safe(instr, block) {
      if (instr->type == nir_instr_type_intrinsic) {
      nir_intrinsic_instr *intr = nir_instr_as_intrinsic(instr);
   if (intr->intrinsic == nir_intrinsic_load_deref) {
                     if ((var->data.mode == nir_var_shader_in &&
      var->data.location == VARYING_SLOT_PNTC) ||
   (var->data.mode == nir_var_system_value &&
   var->data.location == SYSTEM_VALUE_POINT_COORD)) {
                  }
      bool
   nir_lower_pntc_ytransform(nir_shader *shader,
         {
      if (!shader->options->lower_wpos_pntc)
            lower_pntc_ytransform_state state = {
      .pntc_state_tokens = *pntc_state_tokens,
   .shader = shader,
                        nir_foreach_function_impl(impl, shader) {
               nir_foreach_block(block, impl) {
         }
   nir_metadata_preserve(impl, nir_metadata_block_index |
                  }
