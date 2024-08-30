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
   #include "d3d12_context.h"
   #include "d3d12_compiler.h"
   #include "d3d12_nir_passes.h"
   #include "d3d12_screen.h"
      static uint32_t
   hash_tcs_variant_key(const void *key)
   {
      d3d12_tcs_variant_key *v = (d3d12_tcs_variant_key*)key;
   uint32_t hash = _mesa_hash_data(v, offsetof(d3d12_tcs_variant_key, varyings));
   if (v->varyings)
            }
      static bool
   equals_tcs_variant_key(const void *a, const void *b)
   {
         }
      void
   d3d12_tcs_variant_cache_init(struct d3d12_context *ctx)
   {
         }
      static void
   delete_entry(struct hash_entry *entry)
   {
         }
      void
   d3d12_tcs_variant_cache_destroy(struct d3d12_context *ctx)
   {
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
      static struct d3d12_shader_selector *
   create_tess_ctrl_shader_variant(struct d3d12_context *ctx, struct d3d12_tcs_variant_key *key)
   {
      nir_builder b = nir_builder_init_simple_shader(MESA_SHADER_TESS_CTRL, &d3d12_screen(ctx->base.screen)->nir_options, "passthrough");
            nir_def *invocation_id = nir_load_invocation_id(&b);
            while(varying_mask) {
      int var_idx = u_bit_scan64(&varying_mask);
   auto slot = &key->varyings->slots[var_idx];
   unsigned frac_mask = slot->location_frac_mask;
   while (frac_mask) {
      int frac = u_bit_scan(&frac_mask);
                  char buf[1024];
   snprintf(buf, sizeof(buf), "in_%d", var->driver_location);
   nir_variable *in = nir_variable_create(nir, nir_var_shader_in, type, buf);
   snprintf(buf, sizeof(buf), "out_%d", var->driver_location);
   nir_variable *out = nir_variable_create(nir, nir_var_shader_out, type, buf);
   out->data.location = in->data.location = var_idx;
                  for (unsigned i = 0; i < key->vertices_out; i++) {
      nir_if *start_block = nir_push_if(&b, nir_ieq_imm(&b, invocation_id, i));
   nir_deref_instr *in_array_var = nir_build_deref_array(&b, nir_build_deref_var(&b, in), invocation_id);
   nir_deref_instr *out_array_var = nir_build_deref_array_imm(&b, nir_build_deref_var(&b, out), i);
   copy_vars(&b, out_array_var, in_array_var);
            }
   nir_variable *gl_TessLevelInner = nir_variable_create(nir, nir_var_shader_out, glsl_array_type(glsl_float_type(), 2, 0), "gl_TessLevelInner");
   gl_TessLevelInner->data.location = VARYING_SLOT_TESS_LEVEL_INNER;
   gl_TessLevelInner->data.patch = 1;
   gl_TessLevelInner->data.compact = 1;
   nir_variable *gl_TessLevelOuter = nir_variable_create(nir, nir_var_shader_out, glsl_array_type(glsl_float_type(), 4, 0), "gl_TessLevelOuter");
   gl_TessLevelOuter->data.location = VARYING_SLOT_TESS_LEVEL_OUTER;
   gl_TessLevelOuter->data.patch = 1;
            nir_variable *state_var_inner = NULL, *state_var_outer = NULL;
   nir_def *load_inner = d3d12_get_state_var(&b, D3D12_STATE_VAR_DEFAULT_INNER_TESS_LEVEL, "d3d12_TessLevelInner", glsl_vec_type(2), &state_var_inner);
            for (unsigned i = 0; i < 2; i++) {
      nir_deref_instr *store_idx = nir_build_deref_array_imm(&b, nir_build_deref_var(&b, gl_TessLevelInner), i);
      }
   for (unsigned i = 0; i < 4; i++) {
      nir_deref_instr *store_idx = nir_build_deref_array_imm(&b, nir_build_deref_var(&b, gl_TessLevelOuter), i);
               nir->info.tess.tcs_vertices_out = key->vertices_out;
   nir_validate_shader(nir, "created");
                     templ.type = PIPE_SHADER_IR_NIR;
   templ.ir.nir = nir;
            d3d12_shader_selector *tcs = d3d12_create_shader(ctx, PIPE_SHADER_TESS_CTRL, &templ);
   if (tcs) {
      tcs->is_variant = true;
      }
      }
      d3d12_shader_selector *
   d3d12_get_tcs_variant(struct d3d12_context *ctx, struct d3d12_tcs_variant_key *key)
   {
      uint32_t hash = hash_tcs_variant_key(key);
   struct hash_entry *entry = _mesa_hash_table_search_pre_hashed(ctx->tcs_variant_cache,
         if (!entry) {
      d3d12_shader_selector *tcs = create_tess_ctrl_shader_variant(ctx, key);
   entry = _mesa_hash_table_insert_pre_hashed(ctx->tcs_variant_cache,
                        }
