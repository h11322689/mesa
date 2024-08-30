   /*
   * Copyright © Microsoft Corporation
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
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   * IN THE SOFTWARE.
   */
      #include "nir.h"
   #include "nir_builder.h"
   #include "d3d12_compiler.h"
   #include "d3d12_nir_passes.h"
   #include "dxil_nir.h"
   #include "program/prog_statevars.h"
      struct lower_state {
      nir_variable *uniform; /* (1/w, 1/h, pt_sz, max_sz) */
   nir_variable *pos_out;
   nir_variable *psiz_out;
   nir_variable *point_coord_out[10];
   unsigned num_point_coords;
            nir_def *point_dir_imm[4];
            /* Current point primitive */
   nir_def *point_pos;
   nir_def *point_size;
   nir_def *varying[VARYING_SLOT_MAX];
            bool sprite_origin_lower_left;
   bool point_size_per_vertex;
      };
      static void
   find_outputs(nir_shader *shader, struct lower_state *state)
   {
      nir_foreach_variable_with_modes(var, shader, nir_var_shader_out) {
      switch (var->data.location) {
   case VARYING_SLOT_POS:
      state->pos_out = var;
      case VARYING_SLOT_PSIZ:
      state->psiz_out = var;
      default:
      state->varying_out[var->data.location] = var;
            }
      static nir_def *
   get_point_dir(nir_builder *b, struct lower_state *state, unsigned i)
   {
      if (state->point_dir_imm[0] == NULL) {
      state->point_dir_imm[0] = nir_imm_vec2(b, -1, -1);
   state->point_dir_imm[1] = nir_imm_vec2(b, -1, 1);
   state->point_dir_imm[2] = nir_imm_vec2(b, 1, -1);
                  }
      static nir_def *
   get_point_coord(nir_builder *b, struct lower_state *state, unsigned i)
   {
      if (state->point_coord_imm[0] == NULL) {
      if (state->sprite_origin_lower_left) {
      state->point_coord_imm[0] = nir_imm_vec4(b, 0, 0, 0, 1);
   state->point_coord_imm[1] = nir_imm_vec4(b, 0, 1, 0, 1);
   state->point_coord_imm[2] = nir_imm_vec4(b, 1, 0, 0, 1);
      } else {
      state->point_coord_imm[0] = nir_imm_vec4(b, 0, 1, 0, 1);
   state->point_coord_imm[1] = nir_imm_vec4(b, 0, 0, 0, 1);
   state->point_coord_imm[2] = nir_imm_vec4(b, 1, 1, 0, 1);
                     }
      /**
   * scaled_point_size = pointSize * pos.w * ViewportSizeRcp
   */
   static void
   get_scaled_point_size(nir_builder *b, struct lower_state *state,
         {
      /* State uniform contains: (1/ViewportWidth, 1/ViewportHeight, PointSize, MaxPointSize) */
   nir_def *uniform = nir_load_var(b, state->uniform);
            /* clamp point-size to valid range */
   if (point_size && state->point_size_per_vertex) {
      point_size = nir_fmax(b, point_size, nir_imm_float(b, 1.0f));
      } else {
      /* Use static point size (from uniform) if the shader output was not set */
               point_size = nir_fmul(b, point_size, nir_channel(b, state->point_pos, 3));
   *x = nir_fmul(b, point_size, nir_channel(b, uniform, 0));
      }
      static bool
   lower_store(nir_intrinsic_instr *instr, nir_builder *b, struct lower_state *state)
   {
      nir_deref_instr *deref = nir_src_as_deref(instr->src[0]);
   if (nir_deref_mode_is(deref, nir_var_shader_out)) {
               switch (var->data.location) {
   case VARYING_SLOT_POS:
      state->point_pos = instr->src[1].ssa;
      case VARYING_SLOT_PSIZ:
      state->point_size = instr->src[1].ssa;
      default:
      state->varying[var->data.location] = instr->src[1].ssa;
   state->varying_write_mask[var->data.location] = nir_intrinsic_write_mask(instr);
               nir_instr_remove(&instr->instr);
                  }
      static bool
   lower_emit_vertex(nir_intrinsic_instr *instr, nir_builder *b, struct lower_state *state)
   {
               nir_def *point_width, *point_height;
            nir_instr_remove(&instr->instr);
   if (stream_id == 0) {
      for (unsigned i = 0; i < 4; i++) {
      /* All outputs need to be emitted for each vertex */
   for (unsigned slot = 0; slot < VARYING_SLOT_MAX; ++slot) {
      if (state->varying[slot] != NULL) {
      nir_store_var(b, state->varying_out[slot], state->varying[slot],
                  /* pos = scaled_point_size * point_dir + point_pos */
   nir_def *point_dir = get_point_dir(b, state, i);
   nir_def *pos = nir_vec4(b,
                              nir_ffma(b,
            point_width,
      nir_ffma(b,
               /* point coord */
   nir_def *point_coord = get_point_coord(b, state, i);
   for (unsigned j = 0; j < state->num_point_coords; ++j) {
      unsigned num_channels = glsl_get_components(state->point_coord_out[j]->type);
   unsigned mask = (1 << num_channels) - 1;
               /* EmitVertex */
               /* EndPrimitive */
               /* Reset everything */
   state->point_pos = NULL;
   state->point_size = NULL;
   for (unsigned i = 0; i < VARYING_SLOT_MAX; ++i)
               }
      static bool
   lower_instr(nir_intrinsic_instr *instr, nir_builder *b, struct lower_state *state)
   {
               if (instr->intrinsic == nir_intrinsic_store_deref) {
         } else if (instr->intrinsic == nir_intrinsic_emit_vertex) {
         } else if (instr->intrinsic == nir_intrinsic_end_primitive) {
      nir_instr_remove(&instr->instr);
                  }
      bool
   d3d12_lower_point_sprite(nir_shader *shader,
                           {
      const gl_state_index16 tokens[4] = { STATE_INTERNAL_DRIVER,
         struct lower_state state;
                     memset(&state, 0, sizeof(state));
   find_outputs(shader, &state);
   state.sprite_origin_lower_left = sprite_origin_lower_left;
            /* Create uniform to retrieve inverse of viewport size and point size:
   * (1/ViewportWidth, 1/ViewportHeight, PointSize, MaxPointSize) */
   state.uniform = nir_state_variable_create(shader, glsl_vec4_type(),
            /* Create new outputs for point tex coordinates */
   unsigned count = 0;
   for (unsigned int sem = 0; sem < ARRAY_SIZE(state.point_coord_out); sem++) {
      if (point_coord_enable & BITFIELD64_BIT(sem)) {
                              nir_variable *var = nir_variable_create(shader,
                     var->data.location = location;
         }
   if (next_inputs_read & VARYING_BIT_PNTC) {
      nir_variable *pntcoord_var = nir_variable_create(shader,
                     pntcoord_var->data.location = VARYING_SLOT_PNTC;
               state.num_point_coords = count;
   if (count) {
      dxil_reassign_driver_locations(shader, nir_var_shader_out,
               nir_foreach_function_impl(impl, shader) {
      nir_builder builder = nir_builder_create(impl);
   nir_foreach_block(block, impl) {
      nir_foreach_instr_safe(instr, block) {
      if (instr->type == nir_instr_type_intrinsic)
      progress |= lower_instr(nir_instr_as_intrinsic(instr),
                     nir_metadata_preserve(impl, nir_metadata_block_index |
               shader->info.gs.output_primitive = GL_TRIANGLE_STRIP;
   shader->info.gs.vertices_out = shader->info.gs.vertices_out * 4 /
                     }
