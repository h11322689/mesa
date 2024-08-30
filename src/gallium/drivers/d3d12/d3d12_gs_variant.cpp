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
      #include "d3d12_compiler.h"
   #include "d3d12_context.h"
   #include "d3d12_debug.h"
   #include "d3d12_screen.h"
      #include "nir.h"
   #include "compiler/nir/nir_builder.h"
   #include "compiler/nir/nir_builtin_builder.h"
      #include "util/u_memory.h"
   #include "util/u_simple_shaders.h"
      static nir_def *
   nir_cull_face(nir_builder *b, nir_variable *vertices, bool ccw)
   {
      nir_def *v0 =
         nir_def *v1 =
         nir_def *v2 =
            nir_def *dir = nir_fdot(b, nir_cross4(b, nir_fsub(b, v1, v0),
               if (ccw)
         else
      }
      static void
   copy_vars(nir_builder *b, nir_deref_instr *dst, nir_deref_instr *src)
   {
      assert(glsl_get_bare_type(dst->type) == glsl_get_bare_type(src->type));
   if (glsl_type_is_struct(dst->type)) {
      for (unsigned i = 0; i < glsl_get_length(dst->type); ++i) {
            } else if (glsl_type_is_array_or_matrix(dst->type)) {
         } else {
            }
      static d3d12_shader_selector*
   d3d12_make_passthrough_gs(struct d3d12_context *ctx, struct d3d12_gs_variant_key *key)
   {
      struct d3d12_shader_selector *gs;
   uint64_t varyings = key->varyings->mask;
   nir_shader *nir;
            nir_builder b = nir_builder_init_simple_shader(MESA_SHADER_GEOMETRY,
                  nir = b.shader;
   nir->info.inputs_read = varyings;
   nir->info.outputs_written = varyings;
   nir->info.gs.input_primitive = GL_POINTS;
   nir->info.gs.output_primitive = GL_POINTS;
   nir->info.gs.vertices_in = 1;
   nir->info.gs.vertices_out = 1;
   nir->info.gs.invocations = 1;
            /* Copy inputs to outputs. */
   while (varyings) {
      char tmp[100];
            unsigned frac_slots = key->varyings->slots[i].location_frac_mask;
   while (frac_slots) {
                     snprintf(tmp, ARRAY_SIZE(tmp), "in_%d", key->varyings->slots[i].vars[j].driver_location);
   in = nir_variable_create(nir,
                     in->data.location = i;
   in->data.location_frac = j;
   in->data.driver_location = key->varyings->slots[i].vars[j].driver_location;
                  snprintf(tmp, ARRAY_SIZE(tmp), "out_%d", key->varyings->slots[i].vars[j].driver_location);
   out = nir_variable_create(nir,
                     out->data.location = i;
   out->data.location_frac = j;
   out->data.driver_location = key->varyings->slots[i].vars[j].driver_location;
                  nir_deref_instr *in_value = nir_build_deref_array(&b, nir_build_deref_var(&b, in),
                        nir_emit_vertex(&b, 0);
            NIR_PASS_V(nir, nir_lower_var_copies);
            templ.type = PIPE_SHADER_IR_NIR;
   templ.ir.nir = nir;
                        }
      struct emit_primitives_context
   {
      struct d3d12_context *ctx;
            unsigned num_vars;
   nir_variable *in[VARYING_SLOT_MAX];
   nir_variable *out[VARYING_SLOT_MAX];
            nir_loop *loop;
   nir_deref_instr *loop_index_deref;
   nir_def *loop_index;
   nir_def *edgeflag_cmp;
      };
      static bool
   d3d12_begin_emit_primitives_gs(struct emit_primitives_context *emit_ctx,
                           {
      nir_builder *b = &emit_ctx->b;
   nir_variable *edgeflag_var = NULL;
   nir_variable *pos_var = NULL;
                     emit_ctx->b = nir_builder_init_simple_shader(MESA_SHADER_GEOMETRY,
                  nir_shader *nir = b->shader;
   nir->info.inputs_read = varyings;
   nir->info.outputs_written = varyings;
   nir->info.gs.input_primitive = GL_TRIANGLES;
   nir->info.gs.output_primitive = output_primitive;
   nir->info.gs.vertices_in = 3;
   nir->info.gs.vertices_out = vertices_out;
   nir->info.gs.invocations = 1;
            while (varyings) {
      char tmp[100];
            unsigned frac_slots = key->varyings->slots[i].location_frac_mask;
   while (frac_slots) {
      int j = u_bit_scan(&frac_slots);
   snprintf(tmp, ARRAY_SIZE(tmp), "in_%d", emit_ctx->num_vars);
   emit_ctx->in[emit_ctx->num_vars] = nir_variable_create(nir,
                     emit_ctx->in[emit_ctx->num_vars]->data.location = i;
   emit_ctx->in[emit_ctx->num_vars]->data.location_frac = j;
   emit_ctx->in[emit_ctx->num_vars]->data.driver_location = key->varyings->slots[i].vars[j].driver_location;
                  /* Don't create an output for the edge flag variable */
   if (i == VARYING_SLOT_EDGE) {
      edgeflag_var = emit_ctx->in[emit_ctx->num_vars];
      } else if (i == VARYING_SLOT_POS) {
                  snprintf(tmp, ARRAY_SIZE(tmp), "out_%d", emit_ctx->num_vars);
   emit_ctx->out[emit_ctx->num_vars] = nir_variable_create(nir,
                     emit_ctx->out[emit_ctx->num_vars]->data.location = i;
   emit_ctx->out[emit_ctx->num_vars]->data.location_frac = j;
   emit_ctx->out[emit_ctx->num_vars]->data.driver_location = key->varyings->slots[i].vars[j].driver_location;
                                 if (key->has_front_face) {
      emit_ctx->front_facing_var = nir_variable_create(nir,
                     emit_ctx->front_facing_var->data.location = VARYING_SLOT_VAR12;
   emit_ctx->front_facing_var->data.driver_location = emit_ctx->num_vars;
               /* Temporary variable "loop_index" to loop over input vertices */
   nir_function_impl *impl = nir_shader_get_entrypoint(nir);
   nir_variable *loop_index_var =
         emit_ctx->loop_index_deref = nir_build_deref_var(b, loop_index_var);
            nir_def *diagonal_vertex = NULL;
   if (key->edge_flag_fix) {
      nir_def *prim_id = nir_load_primitive_id(b);
   nir_def *odd = nir_build_alu(b, nir_op_imod,
                     diagonal_vertex = nir_bcsel(b, nir_i2b(b, odd),
                     if (key->cull_mode != PIPE_FACE_NONE || key->has_front_face) {
      if (key->cull_mode == PIPE_FACE_BACK)
         else if (key->cull_mode == PIPE_FACE_FRONT)
            if (key->has_front_face) {
      if (key->cull_mode == PIPE_FACE_BACK)
         else
                        /**
   *  while {
   *     if (loop_index >= 3)
   *        break;
   */
            emit_ctx->loop_index = nir_load_deref(b, emit_ctx->loop_index_deref);
   nir_def *cmp = nir_ige_imm(b, emit_ctx->loop_index, 3);
   nir_if *loop_check = nir_push_if(b, cmp);
   nir_jump(b, nir_jump_break);
            if (edgeflag_var) {
      nir_def *edge_flag =
         nir_def *is_edge = nir_feq_imm(b, nir_channel(b, edge_flag, 0), 1.0);
   if (emit_ctx->edgeflag_cmp)
         else
               if (key->edge_flag_fix) {
      nir_def *is_edge = nir_ine(b, emit_ctx->loop_index, diagonal_vertex);
   if (emit_ctx->edgeflag_cmp)
         else
                  }
      static struct d3d12_shader_selector *
   d3d12_finish_emit_primitives_gs(struct emit_primitives_context *emit_ctx, bool end_primitive)
   {
      struct pipe_shader_state templ;
   nir_builder *b = &emit_ctx->b;
            /**
   *     loop_index++;
   *  }
   */
   nir_store_deref(b, emit_ctx->loop_index_deref, nir_iadd_imm(b, emit_ctx->loop_index, 1), 1);
            if (end_primitive)
                              templ.type = PIPE_SHADER_IR_NIR;
   templ.ir.nir = nir;
               }
      static d3d12_shader_selector*
   d3d12_emit_points(struct d3d12_context *ctx, struct d3d12_gs_variant_key *key)
   {
      struct emit_primitives_context emit_ctx = {0};
                     /**
   *  if (edge_flag)
   *     out_position = in_position;
   *  else
   *     out_position = vec4(-2.0, -2.0, 0.0, 1.0); // Invalid position
   *
   *  [...] // Copy other variables
   *
   *  EmitVertex();
   */
   for (unsigned i = 0; i < emit_ctx.num_vars; ++i) {
      nir_def *index = (key->flat_varyings & (1ull << emit_ctx.in[i]->data.location))  ?
         nir_deref_instr *in_value = nir_build_deref_array(b, nir_build_deref_var(b, emit_ctx.in[i]), index);
   if (emit_ctx.in[i]->data.location == VARYING_SLOT_POS && emit_ctx.edgeflag_cmp) {
      nir_if *edge_check = nir_push_if(b, emit_ctx.edgeflag_cmp);
   copy_vars(b, nir_build_deref_var(b, emit_ctx.out[i]), in_value);
   nir_if *edge_else = nir_push_else(b, edge_check);
   nir_store_deref(b, nir_build_deref_var(b, emit_ctx.out[i]),
            } else {
            }
   if (key->has_front_face)
                     }
      static d3d12_shader_selector*
   d3d12_emit_lines(struct d3d12_context *ctx, struct d3d12_gs_variant_key *key)
   {
      struct emit_primitives_context emit_ctx = {0};
                              /* First vertex */
   for (unsigned i = 0; i < emit_ctx.num_vars; ++i) {
      nir_def *index = (key->flat_varyings & (1ull << emit_ctx.in[i]->data.location)) ?
         nir_deref_instr *in_value = nir_build_deref_array(b, nir_build_deref_var(b, emit_ctx.in[i]), index);
      }
   if (key->has_front_face)
                  /* Second vertex. If not an edge, use same position as first vertex */
   for (unsigned i = 0; i < emit_ctx.num_vars; ++i) {
      nir_def *index = next_index;
   if (emit_ctx.in[i]->data.location == VARYING_SLOT_POS)
         else if (key->flat_varyings & (1ull << emit_ctx.in[i]->data.location))
         copy_vars(b, nir_build_deref_var(b, emit_ctx.out[i]),
      }
   if (key->has_front_face)
                              }
      static d3d12_shader_selector*
   d3d12_emit_triangles(struct d3d12_context *ctx, struct d3d12_gs_variant_key *key)
   {
      struct emit_primitives_context emit_ctx = {0};
                     /**
   *  [...] // Copy variables
   *
   *  EmitVertex();
                     if (key->provoking_vertex > 0)
         else
            if (key->alternate_tri) {
      nir_def *odd = nir_imod_imm(b, nir_load_primitive_id(b), 2);
               assert(incr != NULL);
   nir_def *index = nir_imod_imm(b, nir_iadd(b, emit_ctx.loop_index, incr), 3);
   for (unsigned i = 0; i < emit_ctx.num_vars; ++i) {
      nir_deref_instr *in_value = nir_build_deref_array(b, nir_build_deref_var(b, emit_ctx.in[i]), index);
      }
               }
      static uint32_t
   hash_gs_variant_key(const void *key)
   {
      d3d12_gs_variant_key *v = (d3d12_gs_variant_key*)key;
   uint32_t hash = _mesa_hash_data(v, offsetof(d3d12_gs_variant_key, varyings));
   if (v->varyings)
            }
      static bool
   equals_gs_variant_key(const void *a, const void *b)
   {
         }
      void
   d3d12_gs_variant_cache_init(struct d3d12_context *ctx)
   {
         }
      static void
   delete_entry(struct hash_entry *entry)
   {
         }
      void
   d3d12_gs_variant_cache_destroy(struct d3d12_context *ctx)
   {
         }
      static struct d3d12_shader_selector *
   create_geometry_shader_variant(struct d3d12_context *ctx, struct d3d12_gs_variant_key *key)
   {
               if (key->passthrough)
         else if (key->provoking_vertex > 0 || key->alternate_tri)
         else if (key->fill_mode == PIPE_POLYGON_MODE_POINT)
         else if (key->fill_mode == PIPE_POLYGON_MODE_LINE)
            if (gs) {
      gs->is_variant = true;
                  }
      d3d12_shader_selector *
   d3d12_get_gs_variant(struct d3d12_context *ctx, struct d3d12_gs_variant_key *key)
   {
      uint32_t hash = hash_gs_variant_key(key);
   struct hash_entry *entry = _mesa_hash_table_search_pre_hashed(ctx->gs_variant_cache,
         if (!entry) {
      d3d12_shader_selector *gs = create_geometry_shader_variant(ctx, key);
   entry = _mesa_hash_table_insert_pre_hashed(ctx->gs_variant_cache,
                        }
