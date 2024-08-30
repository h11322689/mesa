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
      #include "d3d12_compute_transforms.h"
   #include "d3d12_nir_passes.h"
   #include "d3d12_query.h"
   #include "d3d12_screen.h"
      #include "nir.h"
   #include "nir_builder.h"
      #include "util/u_memory.h"
      nir_shader *
   get_indirect_draw_base_vertex_transform(const nir_shader_compiler_options *options, const d3d12_compute_transform_key *args)
   {
               if (args->base_vertex.dynamic_count) {
      nir_variable *count_ubo = nir_variable_create(b.shader, nir_var_mem_ubo,
                     nir_variable *input_ssbo = nir_variable_create(b.shader, nir_var_mem_ssbo,
         nir_variable *output_ssbo = nir_variable_create(b.shader, nir_var_mem_ssbo,
         input_ssbo->data.driver_location = 0;
            nir_def *draw_id = nir_channel(&b, nir_load_global_invocation_id(&b, 32), 0);
   if (args->base_vertex.dynamic_count) {
      nir_def *count = nir_load_ubo(&b, 1, 32, nir_imm_int(&b, 1), nir_imm_int(&b, 0),
                     nir_variable *stride_ubo = NULL;
   nir_def *in_stride_offset_and_base_drawid = d3d12_get_state_var(&b, D3D12_STATE_VAR_TRANSFORM_GENERIC0, "d3d12_Stride",
         nir_def *in_offset = nir_iadd(&b, nir_channel(&b, in_stride_offset_and_base_drawid, 1),
                  nir_def *in_data1 = NULL;
   nir_def *base_vertex = NULL, *base_instance = NULL;
   if (args->base_vertex.indexed) {
      nir_def *in_offset1 = nir_iadd(&b, in_offset, nir_imm_int(&b, 16));
   in_data1 = nir_load_ssbo(&b, 1, 32, nir_imm_int(&b, 0), in_offset1, (gl_access_qualifier)0, 4, 0);
   base_vertex = nir_channel(&b, in_data0, 3);
      } else {
      base_vertex = nir_channel(&b, in_data0, 2);
               /* 4 additional uints for base vertex, base instance, draw ID, and a bool for indexed draw */
            nir_def *out_offset = nir_imul(&b, draw_id, nir_imm_int(&b, out_stride));
   nir_def *out_data0 = nir_vec4(&b, base_vertex, base_instance,
      nir_iadd(&b, draw_id, nir_channel(&b, in_stride_offset_and_base_drawid, 2)),
               nir_store_ssbo(&b, out_data0, nir_imm_int(&b, 1), out_offset, 0xf, (gl_access_qualifier)0, 4, 0);
   nir_store_ssbo(&b, out_data1, nir_imm_int(&b, 1), nir_iadd(&b, out_offset, nir_imm_int(&b, 16)),
         if (args->base_vertex.indexed)
            if (args->base_vertex.dynamic_count)
            nir_validate_shader(b.shader, "creation");
   b.shader->info.num_ssbos = 2;
               }
      static struct nir_shader *
   get_fake_so_buffer_copy_back(const nir_shader_compiler_options *options, const d3d12_compute_transform_key *key)
   {
               nir_variable *output_so_data_var = nir_variable_create(b.shader, nir_var_mem_ssbo,
         nir_variable *input_so_data_var = nir_variable_create(b.shader, nir_var_mem_ssbo, output_so_data_var->type, "input_data");
   output_so_data_var->data.driver_location = 0;
            /* UBO is [fake SO filled size, fake SO vertex count, 1, 1, original SO filled size] */
   nir_variable *input_ubo = nir_variable_create(b.shader, nir_var_mem_ubo,
                  nir_def *original_so_filled_size = nir_load_ubo(&b, 1, 32, nir_imm_int(&b, 0), nir_imm_int(&b, 4 * sizeof(uint32_t)),
            nir_variable *state_var = nullptr;
            nir_def *vertex_offset = nir_imul(&b, nir_imm_int(&b, key->fake_so_buffer_copy_back.stride),
            nir_def *output_offset_base = nir_iadd(&b, original_so_filled_size, vertex_offset);
            for (unsigned i = 0; i < key->fake_so_buffer_copy_back.num_ranges; ++i) {
      auto& output = key->fake_so_buffer_copy_back.ranges[i];
   assert(output.size % 4 == 0 && output.offset % 4 == 0);
   nir_def *field_offset = nir_imm_int(&b, output.offset);
   nir_def *output_offset = nir_iadd(&b, output_offset_base, field_offset);
            for (unsigned loaded = 0; loaded < output.size; loaded += 16) {
      unsigned to_load = MIN2(output.size, 16);
   unsigned components = to_load / 4;
   nir_def *loaded_data = nir_load_ssbo(&b, components, 32, nir_imm_int(&b, 1),
         nir_store_ssbo(&b, loaded_data, nir_imm_int(&b, 0),
                  nir_validate_shader(b.shader, "creation");
   b.shader->info.num_ssbos = 2;
               }
      static struct nir_shader *
   get_fake_so_buffer_vertex_count(const nir_shader_compiler_options *options)
   {
               nir_variable_create(b.shader, nir_var_mem_ssbo, glsl_array_type(glsl_uint_type(), 0, 0), "fake_so");
            nir_variable *real_so_var = nir_variable_create(b.shader, nir_var_mem_ssbo,
         real_so_var->data.driver_location = 1;
            nir_variable *state_var = nullptr;
   nir_def *state_var_data = d3d12_get_state_var(&b, D3D12_STATE_VAR_TRANSFORM_GENERIC0, "state_var", glsl_uvec4_type(), &state_var);
   nir_def *stride = nir_channel(&b, state_var_data, 0);
            nir_def *real_so_bytes_added = nir_idiv(&b, fake_buffer_filled_size, fake_so_multiplier);
   nir_def *vertex_count = nir_idiv(&b, real_so_bytes_added, stride);
   nir_def *to_write_to_fake_buffer = nir_vec4(&b, vertex_count, nir_imm_int(&b, 1), nir_imm_int(&b, 1), real_buffer_filled_size);
            nir_def *updated_filled_size = nir_iadd(&b, real_buffer_filled_size, real_so_bytes_added);
            nir_validate_shader(b.shader, "creation");
   b.shader->info.num_ssbos = 2;
               }
      static struct nir_shader *
   get_draw_auto(const nir_shader_compiler_options *options)
   {
               nir_variable_create(b.shader, nir_var_mem_ssbo, glsl_array_type(glsl_uint_type(), 0, 0), "ssbo");
            nir_variable *state_var = nullptr;
   nir_def *state_var_data = d3d12_get_state_var(&b, D3D12_STATE_VAR_TRANSFORM_GENERIC0, "state_var", glsl_uvec4_type(), &state_var);
   nir_def *stride = nir_channel(&b, state_var_data, 0);
            nir_def *vb_bytes = nir_bcsel(&b, nir_ilt(&b, vb_offset, buffer_filled_size),
            nir_def *vertex_count = nir_idiv(&b, vb_bytes, stride);
   nir_def *to_write = nir_vec4(&b, vertex_count, nir_imm_int(&b, 1), nir_imm_int(&b, 0), nir_imm_int(&b, 0));
            nir_validate_shader(b.shader, "creation");
   b.shader->info.num_ssbos = 1;
               }
      static struct nir_shader *
   create_compute_transform(const nir_shader_compiler_options *options, const d3d12_compute_transform_key *key)
   {
      switch (key->type) {
   case d3d12_compute_transform_type::base_vertex:
         case d3d12_compute_transform_type::fake_so_buffer_copy_back:
         case d3d12_compute_transform_type::fake_so_buffer_vertex_count:
         case d3d12_compute_transform_type::draw_auto:
         default:
            }
      struct compute_transform
   {
      d3d12_compute_transform_key key;
      };
      d3d12_shader_selector *
   d3d12_get_compute_transform(struct d3d12_context *ctx, const d3d12_compute_transform_key *key)
   {
      struct hash_entry *entry = _mesa_hash_table_search(ctx->compute_transform_cache, key);
   if (!entry) {
      compute_transform *data = (compute_transform *)MALLOC(sizeof(compute_transform));
   if (!data)
                     memcpy(&data->key, key, sizeof(*key));
   nir_shader *s = create_compute_transform(options, key);
   if (!s) {
      FREE(data);
      }
   struct pipe_compute_state shader_args = { PIPE_SHADER_IR_NIR, s };
   data->shader = d3d12_create_compute_shader(ctx, &shader_args);
   if (!data->shader) {
      ralloc_free(s);
   FREE(data);
               data->shader->is_variant = true;
   entry = _mesa_hash_table_insert(ctx->compute_transform_cache, &data->key, data);
                  }
      static uint32_t
   hash_compute_transform_key(const void *key)
   {
         }
      static bool
   equals_compute_transform_key(const void *a, const void *b)
   {
         }
      void
   d3d12_compute_transform_cache_init(struct d3d12_context *ctx)
   {
      ctx->compute_transform_cache = _mesa_hash_table_create(NULL,
            }
      static void
   delete_entry(struct hash_entry *entry)
   {
      struct compute_transform *data = (struct compute_transform *)entry->data;
   d3d12_shader_free(data->shader);
      }
      void
   d3d12_compute_transform_cache_destroy(struct d3d12_context *ctx)
   {
         }
      void
   d3d12_save_compute_transform_state(struct d3d12_context *ctx, d3d12_compute_transform_save_restore *save)
   {
      if (ctx->current_predication)
            memset(save, 0, sizeof(*save));
            pipe_resource_reference(&save->cbuf0.buffer, ctx->cbufs[PIPE_SHADER_COMPUTE][1].buffer);
            for (unsigned i = 0; i < ARRAY_SIZE(save->ssbos); ++i) {
      pipe_resource_reference(&save->ssbos[i].buffer, ctx->ssbo_views[PIPE_SHADER_COMPUTE][i].buffer);
         }
      void
   d3d12_restore_compute_transform_state(struct d3d12_context *ctx, d3d12_compute_transform_save_restore *save)
   {
               ctx->base.set_constant_buffer(&ctx->base, PIPE_SHADER_COMPUTE, 1, true, &save->cbuf0);
            if (ctx->current_predication)
      }
