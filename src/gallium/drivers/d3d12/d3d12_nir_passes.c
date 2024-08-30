   /*
   * Copyright © Microsoft Corporation
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
      #include "d3d12_nir_passes.h"
   #include "d3d12_compiler.h"
   #include "nir_builder.h"
   #include "nir_builtin_builder.h"
   #include "nir_deref.h"
   #include "nir_format_convert.h"
   #include "program/prog_instruction.h"
   #include "dxil_nir.h"
      /**
   * Lower Y Flip:
   *
   * We can't do a Y flip simply by negating the viewport height,
   * so we need to lower the flip into the NIR shader.
   */
      nir_def *
   d3d12_get_state_var(nir_builder *b,
                     enum d3d12_state_var var_enum,
   {
      const gl_state_index16 tokens[STATE_LENGTH] = { STATE_INTERNAL_DRIVER, var_enum };
   if (*out_var == NULL) {
      nir_variable *var = nir_state_variable_create(b->shader, var_type,
         var->data.how_declared = nir_var_hidden;
      }
      }
      static void
   lower_pos_write(nir_builder *b, struct nir_instr *instr, nir_variable **flip)
   {
      if (instr->type != nir_instr_type_intrinsic)
            nir_intrinsic_instr *intr = nir_instr_as_intrinsic(instr);
   if (intr->intrinsic != nir_intrinsic_store_deref)
            nir_variable *var = nir_intrinsic_get_var(intr, 0);
   if (var->data.mode != nir_var_shader_out ||
      var->data.location != VARYING_SLOT_POS)
                  nir_def *pos = intr->src[1].ssa;
   nir_def *flip_y = d3d12_get_state_var(b, D3D12_STATE_VAR_Y_FLIP, "d3d12_FlipY",
         nir_def *def = nir_vec4(b,
                              }
      void
   d3d12_lower_yflip(nir_shader *nir)
   {
               if (nir->info.stage != MESA_SHADER_VERTEX &&
      nir->info.stage != MESA_SHADER_TESS_EVAL &&
   nir->info.stage != MESA_SHADER_GEOMETRY)
         nir_foreach_function_impl(impl, nir) {
               nir_foreach_block(block, impl) {
      nir_foreach_instr_safe(instr, block) {
                     nir_metadata_preserve(impl, nir_metadata_block_index |
         }
      static void
   lower_pos_read(nir_builder *b, struct nir_instr *instr,
         {
      if (instr->type != nir_instr_type_intrinsic)
            nir_intrinsic_instr *intr = nir_instr_as_intrinsic(instr);
   if (intr->intrinsic != nir_intrinsic_load_deref)
            nir_variable *var = nir_intrinsic_get_var(intr, 0);
   if (var->data.mode != nir_var_shader_in ||
      var->data.location != VARYING_SLOT_POS)
                  nir_def *pos = nir_instr_def(instr);
            assert(depth_transform_var);
   nir_def *depth_transform = d3d12_get_state_var(b, D3D12_STATE_VAR_DEPTH_TRANSFORM,
                     depth = nir_fmad(b, depth, nir_channel(b, depth_transform, 0),
                     nir_def_rewrite_uses_after(&intr->def, pos,
      }
      void
   d3d12_lower_depth_range(nir_shader *nir)
   {
      assert(nir->info.stage == MESA_SHADER_FRAGMENT);
   nir_variable *depth_transform = NULL;
   nir_foreach_function_impl(impl, nir) {
               nir_foreach_block(block, impl) {
      nir_foreach_instr_safe(instr, block) {
                     nir_metadata_preserve(impl, nir_metadata_block_index |
         }
      struct compute_state_vars {
         };
      static bool
   lower_compute_state_vars(nir_builder *b, nir_instr *instr, void *_state)
   {
      if (instr->type != nir_instr_type_intrinsic)
            b->cursor = nir_after_instr(instr);
   nir_intrinsic_instr *intr = nir_instr_as_intrinsic(instr);
   struct compute_state_vars *vars = _state;
   nir_def *result = NULL;
   switch (intr->intrinsic) {
   case nir_intrinsic_load_num_workgroups:
      result = d3d12_get_state_var(b, D3D12_STATE_VAR_NUM_WORKGROUPS, "d3d12_NumWorkgroups",
            default:
                  nir_def_rewrite_uses(&intr->def, result);
   nir_instr_remove(instr);
      }
      bool
   d3d12_lower_compute_state_vars(nir_shader *nir)
   {
      assert(nir->info.stage == MESA_SHADER_COMPUTE);
   struct compute_state_vars vars = { 0 };
   return nir_shader_instructions_pass(nir, lower_compute_state_vars,
      }
      static bool
   is_color_output(nir_variable *var)
   {
      return (var->data.mode == nir_var_shader_out &&
            }
      static void
   lower_uint_color_write(nir_builder *b, struct nir_instr *instr, bool is_signed)
   {
      const unsigned NUM_BITS = 8;
            if (instr->type != nir_instr_type_intrinsic)
            nir_intrinsic_instr *intr = nir_instr_as_intrinsic(instr);
   if (intr->intrinsic != nir_intrinsic_store_deref)
            nir_variable *var = nir_intrinsic_get_var(intr, 0);
   if (!is_color_output(var))
                     nir_def *col = intr->src[1].ssa;
   nir_def *def = is_signed ? nir_format_float_to_snorm(b, col, bits) :
         if (is_signed)
      def = nir_bcsel(b, nir_ilt_imm(b, def, 0),
               }
      void
   d3d12_lower_uint_cast(nir_shader *nir, bool is_signed)
   {
      if (nir->info.stage != MESA_SHADER_FRAGMENT)
            nir_foreach_function_impl(impl, nir) {
               nir_foreach_block(block, impl) {
      nir_foreach_instr_safe(instr, block) {
                     nir_metadata_preserve(impl, nir_metadata_block_index |
         }
      static bool
   lower_load_draw_params(nir_builder *b, nir_intrinsic_instr *intr,
         {
      if (intr->intrinsic != nir_intrinsic_load_first_vertex &&
      intr->intrinsic != nir_intrinsic_load_base_instance &&
   intr->intrinsic != nir_intrinsic_load_draw_id &&
   intr->intrinsic != nir_intrinsic_load_is_indexed_draw)
                  nir_def *load = d3d12_get_state_var(b, D3D12_STATE_VAR_DRAW_PARAMS, "d3d12_DrawParams",
         unsigned channel = intr->intrinsic == nir_intrinsic_load_first_vertex ? 0 :
      intr->intrinsic == nir_intrinsic_load_base_instance ? 1 :
      nir_def_rewrite_uses(&intr->def, nir_channel(b, load, channel));
               }
      bool
   d3d12_lower_load_draw_params(struct nir_shader *nir)
   {
      nir_variable *draw_params = NULL;
   if (nir->info.stage != MESA_SHADER_VERTEX)
            return nir_shader_intrinsics_pass(nir, lower_load_draw_params,
            }
      static bool
   lower_load_patch_vertices_in(nir_builder *b, nir_intrinsic_instr *intr,
         {
      if (intr->intrinsic != nir_intrinsic_load_patch_vertices_in)
            b->cursor = nir_before_instr(&intr->instr);
   nir_def *load = b->shader->info.stage == MESA_SHADER_TESS_CTRL ?
      d3d12_get_state_var(b, D3D12_STATE_VAR_PATCH_VERTICES_IN, "d3d12_FirstVertex", glsl_uint_type(), _state) :
      nir_def_rewrite_uses(&intr->def, load);
   nir_instr_remove(&intr->instr);
      }
      bool
   d3d12_lower_load_patch_vertices_in(struct nir_shader *nir)
   {
               if (nir->info.stage != MESA_SHADER_TESS_CTRL &&
      nir->info.stage != MESA_SHADER_TESS_EVAL)
         return nir_shader_intrinsics_pass(nir, lower_load_patch_vertices_in,
            }
      struct invert_depth_state
   {
      unsigned viewport_mask;
   bool clip_halfz;
   nir_def *viewport_index;
      };
      static void
   invert_depth_impl(nir_builder *b, struct invert_depth_state *state)
   {
               nir_intrinsic_instr *intr = nir_instr_as_intrinsic(state->store_pos_instr);
   if (state->viewport_index) {
      /* Cursor is assigned before calling. Make sure that storing pos comes
   * after computing the viewport.
   */
                                 if (state->viewport_index) {
         }
   nir_def *old_depth = nir_channel(b, pos, 2);
   nir_def *new_depth = nir_fneg(b, old_depth);
   if (state->clip_halfz)
         nir_def *def = nir_vec4(b,
                           if (state->viewport_index) {
      nir_pop_if(b, NULL);
      }
            state->viewport_index = NULL;
      }
      static void
   invert_depth_instr(nir_builder *b, struct nir_instr *instr, struct invert_depth_state *state)
   {
      if (instr->type != nir_instr_type_intrinsic)
            nir_intrinsic_instr *intr = nir_instr_as_intrinsic(instr);
   if (intr->intrinsic == nir_intrinsic_store_deref) {
      nir_variable *var = nir_intrinsic_get_var(intr, 0);
   if (var->data.mode != nir_var_shader_out)
            if (var->data.location == VARYING_SLOT_VIEWPORT)
         if (var->data.location == VARYING_SLOT_POS)
      } else if (intr->intrinsic == nir_intrinsic_emit_vertex) {
      b->cursor = nir_before_instr(instr);
         }
      /* In OpenGL the windows space depth value z_w is evaluated according to "s * z_d + b"
   * with  "s = (far - near) / 2" (depth clip:minus_one_to_one) [OpenGL 3.3, 2.13.1].
   * When we switch the far and near value to satisfy DirectX requirements we have
   * to compensate by inverting "z_d' = -z_d" with this lowering pass.
   * When depth clip is set zero_to_one, we compensate with "z_d' = 1.0f - z_d" instead.
   */
   void
   d3d12_nir_invert_depth(nir_shader *shader, unsigned viewport_mask, bool clip_halfz)
   {
      if (shader->info.stage != MESA_SHADER_VERTEX &&
      shader->info.stage != MESA_SHADER_TESS_EVAL &&
   shader->info.stage != MESA_SHADER_GEOMETRY)
         struct invert_depth_state state = { viewport_mask, clip_halfz };
   nir_foreach_function_impl(impl, shader) {
               nir_foreach_block(block, impl) {
      nir_foreach_instr_safe(instr, block) {
                     if (state.store_pos_instr) {
      b.cursor = nir_after_block(impl->end_block);
               nir_metadata_preserve(impl, nir_metadata_block_index |
         }
         /**
   * Lower State Vars:
   *
   * All uniforms related to internal D3D12 variables are
   * condensed into a UBO that is appended at the end of the
   * current ones.
   */
      static unsigned
   get_state_var_offset(struct d3d12_shader *shader, enum d3d12_state_var var)
   {
      for (unsigned i = 0; i < shader->num_state_vars; ++i) {
      if (shader->state_vars[i].var == var)
               unsigned offset = shader->state_vars_size;
   shader->state_vars[shader->num_state_vars].offset = offset;
   shader->state_vars[shader->num_state_vars].var = var;
   shader->state_vars_size += 4; /* Use 4-words slots no matter the variable size */
               }
      static bool
   lower_instr(nir_intrinsic_instr *instr, nir_builder *b,
         {
      nir_variable *variable = NULL;
                     if (instr->intrinsic == nir_intrinsic_load_uniform) {
      nir_foreach_variable_with_modes(var, b->shader, nir_var_uniform) {
      if (var->data.driver_location == nir_intrinsic_base(instr)) {
      variable = var;
            } else if (instr->intrinsic == nir_intrinsic_load_deref) {
      deref = nir_src_as_deref(instr->src[0]);
               if (variable == NULL ||
      variable->num_state_slots != 1 ||
   variable->state_slots[0].tokens[0] != STATE_INTERNAL_DRIVER)
         enum d3d12_state_var var = variable->state_slots[0].tokens[1];
   nir_def *ubo_idx = nir_imm_int(b, binding);
   nir_def *ubo_offset =  nir_imm_int(b, get_state_var_offset(shader, var) * 4);
   nir_def *load =
      nir_load_ubo(b, instr->num_components, instr->def.bit_size,
               ubo_idx, ubo_offset,
   .align_mul = 16,
   .align_offset = 0,
                  /* Remove the old load_* instruction and any parent derefs */
   nir_instr_remove(&instr->instr);
   for (nir_deref_instr *d = deref; d; d = nir_deref_instr_parent(d)) {
      /* If anyone is using this deref, leave it alone */
   if (!list_is_empty(&d->def.uses))
                           }
      bool
   d3d12_lower_state_vars(nir_shader *nir, struct d3d12_shader *shader)
   {
               /* The state var UBO is added after all the other UBOs if it already
   * exists it will be replaced by using the same binding.
   * In the event there are no other UBO's, use binding slot 1 to
   * be consistent with other non-default UBO's */
            nir_foreach_variable_with_modes_safe(var, nir, nir_var_uniform) {
      if (var->num_state_slots == 1 &&
      var->state_slots[0].tokens[0] == STATE_INTERNAL_DRIVER) {
   if (var->data.mode == nir_var_mem_ubo) {
                        nir_foreach_function_impl(impl, nir) {
      nir_builder builder = nir_builder_create(impl);
   nir_foreach_block(block, impl) {
      nir_foreach_instr_safe(instr, block) {
      if (instr->type == nir_instr_type_intrinsic)
      progress |= lower_instr(nir_instr_as_intrinsic(instr),
                           nir_metadata_preserve(impl, nir_metadata_block_index |
               if (progress) {
                        /* Remove state variables */
   nir_foreach_variable_with_modes_safe(var, nir, nir_var_uniform) {
      if (var->num_state_slots == 1 &&
      var->state_slots[0].tokens[0] == STATE_INTERNAL_DRIVER) {
   exec_node_remove(&var->node);
                  const gl_state_index16 tokens[STATE_LENGTH] = { STATE_INTERNAL_DRIVER };
   const struct glsl_type *type = glsl_array_type(glsl_vec4_type(),
         nir_variable *ubo = nir_variable_create(nir, nir_var_mem_ubo, type,
         if (binding >= nir->info.num_ubos)
         ubo->data.binding = binding;
   ubo->num_state_slots = 1;
   ubo->state_slots = ralloc_array(ubo, nir_state_slot, 1);
   memcpy(ubo->state_slots[0].tokens, tokens,
            struct glsl_struct_field field = {
      .type = type,
   .name = "data",
      };
   ubo->interface_type =
                        }
      void
   d3d12_add_missing_dual_src_target(struct nir_shader *s,
         {
      assert(missing_mask != 0);
   nir_builder b;
   nir_function_impl *impl = nir_shader_get_entrypoint(s);
            nir_def *zero = nir_imm_zero(&b, 4, 32);
               if (!(missing_mask & (1u << i)))
            const char *name = i == 0 ? "gl_FragData[0]" :
         nir_variable *out = nir_variable_create(s, nir_var_shader_out,
         out->data.location = FRAG_RESULT_DATA0;
   out->data.driver_location = i;
               }
   nir_metadata_preserve(impl, nir_metadata_block_index |
      }
      void
   d3d12_lower_primitive_id(nir_shader *shader)
   {
      nir_builder b;
   nir_function_impl *impl = nir_shader_get_entrypoint(shader);
   nir_def *primitive_id;
            nir_variable *primitive_id_var = nir_variable_create(shader, nir_var_shader_out,
         primitive_id_var->data.location = VARYING_SLOT_PRIMITIVE_ID;
            nir_foreach_block(block, impl) {
      b.cursor = nir_before_block(block);
            nir_foreach_instr_safe(instr, block) {
      if (instr->type != nir_instr_type_intrinsic ||
                  b.cursor = nir_before_instr(instr);
                     }
      static void
   lower_triangle_strip_store(nir_builder *b, nir_intrinsic_instr *intr,
               {
      /**
   * tmp_varying[slot][min(vertex_count, 2)] = src
   */
   nir_def *vertex_count = nir_load_var(b, vertex_count_var);
   nir_def *index = nir_imin(b, vertex_count, nir_imm_int(b, 2));
            if (var->data.mode != nir_var_shader_out)
            nir_deref_instr *deref = nir_build_deref_array(b, nir_build_deref_var(b, varyings[var->data.location]), index);
   nir_def *value = intr->src[1].ssa;
   nir_store_deref(b, deref, value, 0xf);
      }
      static void
   lower_triangle_strip_emit_vertex(nir_builder *b, nir_intrinsic_instr *intr,
                     {
      // TODO xfb + flat shading + last_pv
   /**
   * if (vertex_count >= 2) {
   *    for (i = 0; i < 3; i++) {
   *       foreach(slot)
   *          out[slot] = tmp_varying[slot][i];
   *       EmitVertex();
   *    }
   *    EndPrimitive();
   *    foreach(slot)
   *       tmp_varying[slot][vertex_count % 2] = tmp_varying[slot][2];
   * }
   * vertex_count++;
            nir_def *two = nir_imm_int(b, 2);
   nir_def *vertex_count = nir_load_var(b, vertex_count_var);
   nir_def *count_cmp = nir_uge(b, vertex_count, two);
            for (int j = 0; j < 3; ++j) {
      for (int i = 0; i < VARYING_SLOT_MAX; ++i) {
      if (!varyings[i])
         nir_copy_deref(b, nir_build_deref_var(b, out_varyings[i]),
      }
               for (int i = 0; i < VARYING_SLOT_MAX; ++i) {
      if (!varyings[i])
         nir_copy_deref(b, nir_build_deref_array(b, nir_build_deref_var(b, varyings[i]), nir_umod(b, vertex_count, two)),
                                 vertex_count = nir_iadd_imm(b, vertex_count, 1);
               }
      static void
   lower_triangle_strip_end_primitive(nir_builder *b, nir_intrinsic_instr *intr,
         {
      /**
   * vertex_count = 0;
   */
   nir_store_var(b, vertex_count_var, nir_imm_int(b, 0), 0x1);
      }
      void
   d3d12_lower_triangle_strip(nir_shader *shader)
   {
      nir_builder b;
   nir_function_impl *impl = nir_shader_get_entrypoint(shader);
   nir_variable *tmp_vars[VARYING_SLOT_MAX] = {0};
   nir_variable *out_vars[VARYING_SLOT_MAX] = {0};
                     nir_variable *vertex_count_var =
            nir_block *first = nir_start_block(impl);
   b.cursor = nir_before_block(first);
   nir_foreach_variable_with_modes(var, shader, nir_var_shader_out) {
      const struct glsl_type *type = glsl_array_type(var->type, 3, 0);
   tmp_vars[var->data.location] =  nir_local_variable_create(impl, type, "tmp_var");
      }
            nir_foreach_block(block, impl) {
      nir_foreach_instr_safe(instr, block) {
                     nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(instr);
   switch (intrin->intrinsic) {
   case nir_intrinsic_store_deref:
      b.cursor = nir_before_instr(instr);
   lower_triangle_strip_store(&b, intrin, vertex_count_var, tmp_vars);
      case nir_intrinsic_emit_vertex_with_counter:
   case nir_intrinsic_emit_vertex:
      b.cursor = nir_before_instr(instr);
   lower_triangle_strip_emit_vertex(&b, intrin, vertex_count_var,
            case nir_intrinsic_end_primitive:
   case nir_intrinsic_end_primitive_with_counter:
      b.cursor = nir_before_instr(instr);
   lower_triangle_strip_end_primitive(&b, intrin, vertex_count_var);
      default:
                        nir_metadata_preserve(impl, nir_metadata_none);
      }
      static bool
   is_multisampling_instr(const nir_instr *instr, const void *_data)
   {
      if (instr->type != nir_instr_type_intrinsic)
         nir_intrinsic_instr *intr = nir_instr_as_intrinsic(instr);
   if (intr->intrinsic == nir_intrinsic_store_output) {
      nir_io_semantics semantics = nir_intrinsic_io_semantics(intr);
      } else if (intr->intrinsic == nir_intrinsic_store_deref) {
      nir_variable *var = nir_intrinsic_get_var(intr, 0);
      } else if (intr->intrinsic == nir_intrinsic_load_sample_id ||
                  }
      static nir_def *
   lower_multisampling_instr(nir_builder *b, nir_instr *instr, void *_data)
   {
      nir_intrinsic_instr *intr = nir_instr_as_intrinsic(instr);
   switch (intr->intrinsic) {
   case nir_intrinsic_store_output:
   case nir_intrinsic_store_deref:
         case nir_intrinsic_load_sample_id:
         case nir_intrinsic_load_sample_mask_in:
         default:
            }
      bool
   d3d12_disable_multisampling(nir_shader *s)
   {
      if (s->info.stage != MESA_SHADER_FRAGMENT)
                  nir_foreach_variable_with_modes_safe(var, s, nir_var_shader_out) {
      if (var->data.location == FRAG_RESULT_SAMPLE_MASK) {
      exec_node_remove(&var->node);
   s->info.outputs_written &= ~(1ull << FRAG_RESULT_SAMPLE_MASK);
         }
   nir_foreach_variable_with_modes_safe(var, s, nir_var_system_value) {
      if (var->data.location == SYSTEM_VALUE_SAMPLE_MASK_IN ||
      var->data.location == SYSTEM_VALUE_SAMPLE_ID) {
   exec_node_remove(&var->node);
      }
      }
   BITSET_CLEAR(s->info.system_values_read, SYSTEM_VALUE_SAMPLE_ID);
   s->info.fs.uses_sample_qualifier = false;
   s->info.fs.uses_sample_shading = false;
      }
      struct multistream_subvar_state {
      nir_variable *var;
   uint8_t stream;
      };
   struct multistream_var_state {
      unsigned num_subvars;
      };
   struct multistream_state {
         };
      static bool
   split_multistream_varying_stores(nir_builder *b, nir_intrinsic_instr *intr,
         {
      if (intr->intrinsic != nir_intrinsic_store_deref)
            nir_deref_instr *deref = nir_src_as_deref(intr->src[0]);
   if (!nir_deref_mode_is(deref, nir_var_shader_out))
            nir_variable *var = nir_deref_instr_get_variable(deref);
            struct multistream_state *state = _state;
   struct multistream_var_state *var_state = &state->vars[var->data.location];
   if (var_state->num_subvars <= 1)
            nir_deref_path path;
   nir_deref_path_init(&path, deref, b->shader);
   assert(path.path[0]->deref_type == nir_deref_type_var && path.path[0]->var == var);
      unsigned first_channel = 0;
   for (unsigned subvar = 0; subvar < var_state->num_subvars; ++subvar) {
      b->cursor = nir_after_instr(&path.path[0]->instr);
            for (unsigned i = 1; path.path[i]; ++i) {
      b->cursor = nir_after_instr(&path.path[i]->instr);
               b->cursor = nir_before_instr(&intr->instr);
   unsigned mask_num_channels = (1 << var_state->subvars[subvar].num_components) - 1;
   unsigned orig_write_mask = nir_intrinsic_write_mask(intr);
                     unsigned new_write_mask = (orig_write_mask >> first_channel) & mask_num_channels;
               nir_deref_path_finish(&path);
   nir_instr_free_and_dce(&intr->instr);
      }
      bool
   d3d12_split_multistream_varyings(nir_shader *s)
   {
      if (s->info.stage != MESA_SHADER_GEOMETRY)
            struct multistream_state state;
            bool progress = false;
   nir_foreach_variable_with_modes_safe(var, s, nir_var_shader_out) {
      if ((var->data.stream & NIR_STREAM_PACKED) == 0)
            struct multistream_var_state *var_state = &state.vars[var->data.location];
   struct multistream_subvar_state *subvars = var_state->subvars;
   for (unsigned i = 0; i < glsl_get_vector_elements(var->type); ++i) {
      unsigned stream = (var->data.stream >> (2 * (i + var->data.location_frac))) & 0x3;
   if (var_state->num_subvars == 0 || stream != subvars[var_state->num_subvars - 1].stream) {
      subvars[var_state->num_subvars].stream = stream;
   subvars[var_state->num_subvars].num_components = 1;
      } else {
                     var->data.stream = subvars[0].stream;
   if (var_state->num_subvars == 1)
                     subvars[0].var = var;
   var->type = glsl_vector_type(glsl_get_base_type(var->type), subvars[0].num_components);
   unsigned location_frac = var->data.location_frac + subvars[0].num_components;
   for (unsigned subvar = 1; subvar < var_state->num_subvars; ++subvar) {
      char *name = ralloc_asprintf(s, "unpacked:%s_stream%d", var->name, subvars[subvar].stream);
   nir_variable *new_var = nir_variable_create(s, nir_var_shader_out,
                  new_var->data = var->data;
   new_var->data.stream = subvars[subvar].stream;
   new_var->data.location_frac = location_frac;
   location_frac += subvars[subvar].num_components;
                  if (progress) {
      nir_shader_intrinsics_pass(s, split_multistream_varying_stores,
            } else {
                     }
      static void
   write_0(nir_builder *b, nir_deref_instr *deref)
   {
      if (glsl_type_is_array_or_matrix(deref->type)) {
      for (unsigned i = 0; i < glsl_get_length(deref->type); ++i)
      } else if (glsl_type_is_struct(deref->type)) {
      for (unsigned i = 0; i < glsl_get_length(deref->type); ++i)
      } else {
      nir_def *scalar = nir_imm_intN_t(b, 0, glsl_get_bit_size(deref->type));
   nir_def *scalar_arr[NIR_MAX_VEC_COMPONENTS];
   unsigned num_comps = glsl_get_components(deref->type);
   unsigned writemask = (1 << num_comps) - 1;
   for (unsigned i = 0; i < num_comps; ++i)
         nir_def *zero_val = nir_vec(b, scalar_arr, num_comps);
         }
      void
   d3d12_write_0_to_new_varying(nir_shader *s, nir_variable *var)
   {
      /* Skip per-vertex HS outputs */
   if (s->info.stage == MESA_SHADER_TESS_CTRL && !var->data.patch)
            nir_foreach_function_impl(impl, s) {
               nir_foreach_block(block, impl) {
      b.cursor = nir_before_block(block);
   if (s->info.stage != MESA_SHADER_GEOMETRY) {
      write_0(&b, nir_build_deref_var(&b, var));
               nir_foreach_instr_safe(instr, block) {
      if (instr->type != nir_instr_type_intrinsic)
         nir_intrinsic_instr *intr = nir_instr_as_intrinsic(instr);
                  b.cursor = nir_before_instr(instr);
                        }
