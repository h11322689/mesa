   /*
   * Copyright © 2016 Intel Corporation
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
      #include "anv_nir.h"
   #include "nir/nir_builder.h"
   #include "util/u_debug.h"
      /**
   * This file implements the lowering required for VK_KHR_multiview.
   *
   * When possible, Primitive Replication is used and the shader is modified to
   * make gl_Position an array and fill it with values for each view.
   *
   * Otherwise we implement multiview using instanced rendering.  The number of
   * instances in each draw call is multiplied by the number of views in the
   * subpass.  Then, in the shader, we divide gl_InstanceId by the number of
   * views and use gl_InstanceId % view_count to compute the actual ViewIndex.
   */
      struct lower_multiview_state {
                        nir_def *instance_id;
      };
      static nir_def *
   build_instance_id(struct lower_multiview_state *state)
   {
               if (state->instance_id == NULL) {
                        /* We use instancing for implementing multiview.  The actual instance id
   * is given by dividing instance_id by the number of views in this
   * subpass.
   */
   state->instance_id =
      nir_idiv(b, nir_load_instance_id(b),
               }
      static nir_def *
   build_view_index(struct lower_multiview_state *state)
   {
               if (state->view_index == NULL) {
                        assert(state->view_mask != 0);
   if (util_bitcount(state->view_mask) == 1) {
      /* Set the view index directly. */
      } else if (state->builder.shader->info.stage == MESA_SHADER_VERTEX) {
                     /* We use instancing for implementing multiview.  The compacted view
   * id is given by instance_id % view_count.  We then have to convert
   * that to an actual view id.
   */
   nir_def *compacted =
                  if (util_is_power_of_two_or_zero(state->view_mask + 1)) {
      /* If we have a full view mask, then compacted is what we want */
      } else {
      /* Now we define a map from compacted view index to the actual
   * view index that's based on the view_mask.  The map is given by
   * 16 nibbles, each of which is a value from 0 to 15.
   */
   uint64_t remap = 0;
   uint32_t i = 0;
   u_foreach_bit(bit, state->view_mask) {
                                 /* One of these days, when we have int64 everywhere, this will be
   * easier.
   */
   nir_def *shifted;
   if (remap <= UINT32_MAX) {
         } else {
      nir_def *shifted_low =
         nir_def *shifted_high =
      nir_ushr(b, nir_imm_int(b, remap >> 32),
      shifted = nir_bcsel(b, nir_ilt_imm(b, shift, 32),
      }
         } else {
      const struct glsl_type *type = glsl_int_type();
   if (b->shader->info.stage == MESA_SHADER_TESS_CTRL ||
                  nir_variable *idx_var =
      nir_variable_create(b->shader, nir_var_shader_in,
      idx_var->data.location = VARYING_SLOT_VIEW_INDEX;
                  nir_deref_instr *deref = nir_build_deref_var(b, idx_var);
                                    }
      static bool
   is_load_view_index(const nir_instr *instr, const void *data)
   {
      return instr->type == nir_instr_type_intrinsic &&
      }
      static nir_def *
   replace_load_view_index_with_zero(struct nir_builder *b,
         {
      assert(is_load_view_index(instr, data));
      }
      static nir_def *
   replace_load_view_index_with_layer_id(struct nir_builder *b,
         {
      assert(is_load_view_index(instr, data));
      }
      bool
   anv_nir_lower_multiview(nir_shader *shader, uint32_t view_mask)
   {
               /* If multiview isn't enabled, just lower the ViewIndex builtin to zero. */
   if (view_mask == 0) {
      return nir_shader_lower_instructions(shader, is_load_view_index,
               if (shader->info.stage == MESA_SHADER_FRAGMENT) {
      return nir_shader_lower_instructions(shader, is_load_view_index,
               /* This pass assumes a single entrypoint */
            struct lower_multiview_state state = {
                           nir_foreach_block(block, entrypoint) {
      nir_foreach_instr_safe(instr, block) {
                              if (load->intrinsic != nir_intrinsic_load_instance_id &&
                  nir_def *value;
   if (load->intrinsic == nir_intrinsic_load_instance_id) {
         } else {
      assert(load->intrinsic == nir_intrinsic_load_view_index);
                                       /* The view index is available in all stages but the instance id is only
   * available in the VS.  If it's not a fragment shader, we need to pass
   * the view index on to the next stage.
   */
                     assert(view_index->parent_instr->block == nir_start_block(entrypoint));
            /* Unless there is only one possible view index (that would be set
   * directly), pass it to the next stage. */
   if (util_bitcount(state.view_mask) != 1) {
      nir_variable *view_index_out =
      nir_variable_create(shader, nir_var_shader_out,
      view_index_out->data.location = VARYING_SLOT_VIEW_INDEX;
               nir_variable *layer_id_out =
      nir_variable_create(shader, nir_var_shader_out,
      layer_id_out->data.location = VARYING_SLOT_LAYER;
            nir_metadata_preserve(entrypoint, nir_metadata_block_index |
               }
