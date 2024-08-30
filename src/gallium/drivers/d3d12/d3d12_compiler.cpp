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
   #include "d3d12_nir_passes.h"
   #include "nir_to_dxil.h"
   #include "dxil_nir.h"
   #include "dxil_nir_lower_int_cubemaps.h"
      #include "pipe/p_state.h"
      #include "nir.h"
   #include "nir/nir_draw_helpers.h"
   #include "nir/tgsi_to_nir.h"
   #include "compiler/nir/nir_builder.h"
      #include "util/hash_table.h"
   #include "util/u_memory.h"
   #include "util/u_prim.h"
   #include "util/u_simple_shaders.h"
   #include "util/u_dl.h"
      #include <dxguids/dxguids.h>
      #ifdef _WIN32
   #include "dxil_validator.h"
   #endif
      const void *
   d3d12_get_compiler_options(struct pipe_screen *screen,
               {
      assert(ir == PIPE_SHADER_IR_NIR);
      }
      static uint32_t
   resource_dimension(enum glsl_sampler_dim dim)
   {
      switch (dim) {
   case GLSL_SAMPLER_DIM_1D:
         case GLSL_SAMPLER_DIM_2D:
         case GLSL_SAMPLER_DIM_3D:
         case GLSL_SAMPLER_DIM_CUBE:
         default:
            }
      static bool
   can_remove_dead_sampler(nir_variable *var, void *data)
   {
      const struct glsl_type *base_type = glsl_without_array(var->type);
      }
      static struct d3d12_shader *
   compile_nir(struct d3d12_context *ctx, struct d3d12_shader_selector *sel,
         {
      struct d3d12_screen *screen = d3d12_screen(ctx->base.screen);
   struct d3d12_shader *shader = rzalloc(sel, d3d12_shader);
            if (shader->key.n_texture_states > 0) {
      shader->key.tex_wrap_states = (dxil_wrap_sampler_state*)ralloc_size(sel, sizeof(dxil_wrap_sampler_state) * shader->key.n_texture_states);
      }
   else
            shader->output_vars_fs = nullptr;
   shader->output_vars_gs = nullptr;
            shader->input_vars_vs = nullptr;
            shader->tess_eval_output_vars = nullptr;
   shader->tess_ctrl_input_vars = nullptr;
   shader->nir = nir;
            NIR_PASS_V(nir, nir_lower_samplers);
            NIR_PASS_V(nir, nir_opt_dce);
   struct nir_remove_dead_variables_options dead_var_opts = {};
   dead_var_opts.can_remove_var = can_remove_dead_sampler;
            if (key->samples_int_textures)
      NIR_PASS_V(nir, dxil_lower_sample_to_txf_for_integer_tex,
               if (key->stage == PIPE_SHADER_VERTEX && key->vs.needs_format_emulation)
            uint32_t num_ubos_before_lower_to_ubo = nir->info.num_ubos;
   uint32_t num_uniforms_before_lower_to_ubo = nir->num_uniforms;
   NIR_PASS_V(nir, nir_lower_uniforms_to_ubo, false, false);
   shader->has_default_ubo0 = num_uniforms_before_lower_to_ubo > 0 &&
            if (key->last_vertex_processing_stage) {
      if (key->invert_depth)
         if (!key->halfz)
            }
   NIR_PASS_V(nir, d3d12_lower_load_draw_params);
   NIR_PASS_V(nir, d3d12_lower_load_patch_vertices_in);
   NIR_PASS_V(nir, d3d12_lower_state_vars, shader);
   const struct dxil_nir_lower_loads_stores_options loads_stores_options = {};
   NIR_PASS_V(nir, dxil_nir_lower_loads_stores_to_dxil, &loads_stores_options);
            if (key->stage == PIPE_SHADER_FRAGMENT && key->fs.multisample_disabled)
            struct nir_to_dxil_options opts = {};
   opts.interpolate_at_vertex = screen->have_load_at_vertex;
   opts.lower_int16 = !screen->opts4.Native16BitShaderOpsSupported;
   opts.no_ubo0 = !shader->has_default_ubo0;
   opts.last_ubo_is_not_arrayed = shader->num_state_vars > 0;
   if (key->stage == PIPE_SHADER_FRAGMENT)
         opts.input_clip_size = key->input_clip_size;
   opts.environment = DXIL_ENVIRONMENT_GL;
      #ifdef _WIN32
         #endif
         struct blob tmp;
   if (!nir_to_dxil(nir, &opts, NULL, &tmp)) {
      debug_printf("D3D12: nir_to_dxil failed\n");
               // Non-ubo variables
   shader->begin_srv_binding = (UINT_MAX);
   nir_foreach_variable_with_modes(var, nir, nir_var_uniform) {
      auto type_no_array = glsl_without_array(var->type);
   if (glsl_type_is_texture(type_no_array)) {
      unsigned count = glsl_type_is_array(var->type) ? glsl_get_aoa_size(var->type) : 1;
   for (unsigned i = 0; i < count; ++i) {
         }
   shader->begin_srv_binding = MIN2(var->data.binding, shader->begin_srv_binding);
                  nir_foreach_image_variable(var, nir) {
      auto type_no_array = glsl_without_array(var->type);
   unsigned count = glsl_type_is_array(var->type) ? glsl_get_aoa_size(var->type) : 1;
   for (unsigned i = 0; i < count; ++i) {
                     // Ubo variables
   if(nir->info.num_ubos) {
      // Ignore state_vars ubo as it is bound as root constants
   unsigned num_ubo_bindings = nir->info.num_ubos - (shader->state_vars_used ? 1 : 0);
   for(unsigned i = shader->has_default_ubo0 ? 0 : 1; i < num_ubo_bindings; ++i) {
                  #ifdef _WIN32
      if (ctx->dxil_validator) {
      if (!(d3d12_debug & D3D12_DEBUG_EXPERIMENTAL)) {
      char *err;
   if (!dxil_validate_module(ctx->dxil_validator, tmp.data,
            debug_printf(
      "== VALIDATION ERROR =============================================\n"
   "%s\n"
   "== END ==========================================================\n",
                     if (d3d12_debug & D3D12_DEBUG_DISASS) {
      char *str = dxil_disasm_module(ctx->dxil_validator, tmp.data,
         fprintf(stderr,
         "== BEGIN SHADER ============================================\n"
   "%s\n"
   "== END SHADER ==============================================\n",
            #endif
                  if (d3d12_debug & D3D12_DEBUG_DXIL) {
      char buf[256];
   static int i;
   snprintf(buf, sizeof(buf), "dump%02d.dxil", i++);
   FILE *fp = fopen(buf, "wb");
   fwrite(shader->bytecode, sizeof(char), shader->bytecode_length, fp);
   fclose(fp);
      }
      }
      struct d3d12_selection_context {
      struct d3d12_context *ctx;
   bool needs_point_sprite_lowering;
   bool needs_vertex_reordering;
   unsigned provoking_vertex;
   bool alternate_tri;
   unsigned fill_mode_lowered;
   unsigned cull_mode_lowered;
   bool manual_depth_range;
   unsigned missing_dual_src_outputs;
   unsigned frag_result_color_lowering;
      };
      unsigned
   missing_dual_src_outputs(struct d3d12_context *ctx)
   {
      if (!ctx->gfx_pipeline_state.blend || !ctx->gfx_pipeline_state.blend->is_dual_src)
            struct d3d12_shader_selector *fs = ctx->gfx_stages[PIPE_SHADER_FRAGMENT];
   if (!fs)
                     unsigned indices_seen = 0;
   nir_foreach_function_impl(impl, s) {
      nir_foreach_block(block, impl) {
      nir_foreach_instr(instr, block) {
                     nir_intrinsic_instr *intr = nir_instr_as_intrinsic(instr);
                  nir_variable *var = nir_intrinsic_get_var(intr, 0);
                  unsigned index = var->data.index;
   if (var->data.location > FRAG_RESULT_DATA0)
         else if (var->data.location != FRAG_RESULT_COLOR &&
                  indices_seen |= 1u << index;
   if ((indices_seen & 3) == 3)
                        }
      static unsigned
   frag_result_color_lowering(struct d3d12_context *ctx)
   {
      struct d3d12_shader_selector *fs = ctx->gfx_stages[PIPE_SHADER_FRAGMENT];
            if (fs->initial->info.outputs_written & BITFIELD64_BIT(FRAG_RESULT_COLOR))
               }
      bool
   manual_depth_range(struct d3d12_context *ctx)
   {
      if (!d3d12_need_zero_one_depth_range(ctx))
            /**
   * If we can't use the D3D12 zero-one depth-range, we might have to apply
   * depth-range ourselves.
   *
   * Because we only need to override the depth-range to zero-one range in
   * the case where we write frag-depth, we only need to apply manual
   * depth-range to gl_FragCoord.z.
   *
   * No extra care is needed to be taken in the case where gl_FragDepth is
   * written conditionally, because the GLSL 4.60 spec states:
   *
   *    If a shader statically assigns a value to gl_FragDepth, and there
   *    is an execution path through the shader that does not set
   *    gl_FragDepth, then the value of the fragment’s depth may be
   *    undefined for executions of the shader that take that path. That
   *    is, if the set of linked fragment shaders statically contain a
   *    write to gl_FragDepth, then it is responsible for always writing
   *    it.
            struct d3d12_shader_selector *fs = ctx->gfx_stages[PIPE_SHADER_FRAGMENT];
      }
      static bool
   needs_edge_flag_fix(enum mesa_prim mode)
   {
      return (mode == MESA_PRIM_QUADS ||
            }
      static unsigned
   fill_mode_lowered(struct d3d12_context *ctx, const struct pipe_draw_info *dinfo)
   {
               if ((ctx->gfx_stages[PIPE_SHADER_GEOMETRY] != NULL &&
      !ctx->gfx_stages[PIPE_SHADER_GEOMETRY]->is_variant) ||
   ctx->gfx_pipeline_state.rast == NULL ||
   (dinfo->mode != MESA_PRIM_TRIANGLES &&
   dinfo->mode != MESA_PRIM_TRIANGLE_STRIP))
         /* D3D12 supports line mode (wireframe) but doesn't support edge flags */
   if (((ctx->gfx_pipeline_state.rast->base.fill_front == PIPE_POLYGON_MODE_LINE &&
            (ctx->gfx_pipeline_state.rast->base.fill_back == PIPE_POLYGON_MODE_LINE &&
         (vs->initial->info.outputs_written & VARYING_BIT_EDGE ||
   needs_edge_flag_fix(ctx->initial_api_prim)))
         if (ctx->gfx_pipeline_state.rast->base.fill_front == PIPE_POLYGON_MODE_POINT)
               }
      static bool
   has_stream_out_for_streams(struct d3d12_context *ctx)
   {
      unsigned mask = ctx->gfx_stages[PIPE_SHADER_GEOMETRY]->initial->info.gs.active_stream_mask & ~1;
   for (unsigned i = 0; i < ctx->gfx_pipeline_state.so_info.num_outputs; ++i) {
      unsigned stream = ctx->gfx_pipeline_state.so_info.output[i].stream;
   if (((1 << stream) & mask) &&
      ctx->so_buffer_views[stream].SizeInBytes)
   }
      }
      static bool
   needs_point_sprite_lowering(struct d3d12_context *ctx, const struct pipe_draw_info *dinfo)
   {
      struct d3d12_shader_selector *vs = ctx->gfx_stages[PIPE_SHADER_VERTEX];
            if (gs != NULL && !gs->is_variant) {
      /* There is an user GS; Check if it outputs points with PSIZE */
   return (gs->initial->info.gs.output_primitive == GL_POINTS &&
         (gs->initial->info.outputs_written & VARYING_BIT_PSIZ ||
            } else {
      /* No user GS; check if we are drawing wide points */
   return ((dinfo->mode == MESA_PRIM_POINTS ||
               (ctx->gfx_pipeline_state.rast->base.point_size > 1.0 ||
      ctx->gfx_pipeline_state.rast->base.offset_point ||
   (ctx->gfx_pipeline_state.rast->base.point_size_per_vertex &&
      }
      static unsigned
   cull_mode_lowered(struct d3d12_context *ctx, unsigned fill_mode)
   {
      if ((ctx->gfx_stages[PIPE_SHADER_GEOMETRY] != NULL &&
      !ctx->gfx_stages[PIPE_SHADER_GEOMETRY]->is_variant) ||
   ctx->gfx_pipeline_state.rast == NULL ||
   ctx->gfx_pipeline_state.rast->base.cull_face == PIPE_FACE_NONE)
            }
      static unsigned
   get_provoking_vertex(struct d3d12_selection_context *sel_ctx, bool *alternate, const struct pipe_draw_info *dinfo)
   {
      if (dinfo->mode == GL_PATCHES) {
      *alternate = false;
               struct d3d12_shader_selector *vs = sel_ctx->ctx->gfx_stages[PIPE_SHADER_VERTEX];
   struct d3d12_shader_selector *gs = sel_ctx->ctx->gfx_stages[PIPE_SHADER_GEOMETRY];
            /* Make sure GL prims match Gallium prims */
   STATIC_ASSERT(GL_POINTS == MESA_PRIM_POINTS);
   STATIC_ASSERT(GL_LINES == MESA_PRIM_LINES);
            enum mesa_prim mode;
   switch (last_vertex_stage->stage) {
   case PIPE_SHADER_GEOMETRY:
      mode = (enum mesa_prim)last_vertex_stage->current->nir->info.gs.output_primitive;
      case PIPE_SHADER_VERTEX:
      mode = (enum mesa_prim)dinfo->mode;
      default:
                  bool flatshade_first = sel_ctx->ctx->gfx_pipeline_state.rast &&
         *alternate = (mode == GL_TRIANGLE_STRIP || mode == GL_TRIANGLE_STRIP_ADJACENCY) &&
                  }
      bool
   has_flat_varyings(struct d3d12_context *ctx)
   {
               if (!fs || !fs->current)
            nir_foreach_variable_with_modes(input, fs->current->nir,
            if (input->data.interpolation == INTERP_MODE_FLAT &&
      /* Disregard sysvals */
   (input->data.location >= VARYING_SLOT_VAR0 ||
                     }
      static bool
   needs_vertex_reordering(struct d3d12_selection_context *sel_ctx, const struct pipe_draw_info *dinfo)
   {
      struct d3d12_context *ctx = sel_ctx->ctx;
   bool flat = ctx->has_flat_varyings;
            if (fill_mode_lowered(ctx, dinfo) != PIPE_POLYGON_MODE_FILL)
                     /* When flat shading a triangle and provoking vertex is not the first one, we use load_at_vertex.
         if (flat && sel_ctx->provoking_vertex >= 2 && (!d3d12_screen(ctx->base.screen)->have_load_at_vertex ||
                  /* When transform feedback is enabled and the output is alternating (triangle strip or triangle
      strip with adjacency), we need to reorder vertices to get the order expected by OpenGL. This
   only works when there is no flat shading involved. In that scenario, we don't care about
      if (xfb && !flat && sel_ctx->alternate_tri) {
      sel_ctx->provoking_vertex = 0;
                  }
      static nir_variable *
   create_varying_from_info(nir_shader *nir, const struct d3d12_varying_info *info,
         {
      nir_variable *var;
            snprintf(tmp, ARRAY_SIZE(tmp),
               var = nir_variable_create(nir, mode, info->slots[slot].types[slot_frac], tmp);
   var->data.location = slot;
   var->data.location_frac = slot_frac;
   var->data.driver_location = info->slots[slot].vars[slot_frac].driver_location;
   var->data.interpolation = info->slots[slot].vars[slot_frac].interpolation;
   var->data.patch = info->slots[slot].patch;
   var->data.compact = info->slots[slot].vars[slot_frac].compact;
   if (patch)
            if (mode == nir_var_shader_out)
               }
      void
   create_varyings_from_info(nir_shader *nir, const struct d3d12_varying_info *info,
         {
      unsigned mask = info->slots[slot].location_frac_mask;
   while (mask)
      }
      static d3d12_varying_info*
   fill_varyings(struct d3d12_context *ctx, const nir_shader *s,
         {
               info.max = 0;
   info.mask = 0;
            nir_foreach_variable_with_modes(var, s, modes) {
      unsigned slot = var->data.location;
   bool is_generic_patch = slot >= VARYING_SLOT_PATCH0;
   if (patch ^ is_generic_patch)
         if (is_generic_patch)
                  if (!(mask & slot_bit))
            if ((info.mask & slot_bit) == 0) {
      memset(info.slots + slot, 0, sizeof(info.slots[0]));
               const struct glsl_type *type = var->type;
   if ((s->info.stage == MESA_SHADER_GEOMETRY ||
      s->info.stage == MESA_SHADER_TESS_CTRL) &&
   (modes & nir_var_shader_in) &&
   glsl_type_is_array(type))
               info.slots[slot].patch = var->data.patch;
   auto& var_slot = info.slots[slot].vars[var->data.location_frac];
   var_slot.driver_location = var->data.driver_location;
   var_slot.interpolation = var->data.interpolation;
   var_slot.compact = var->data.compact;
   info.mask |= slot_bit;
               for (uint32_t i = 0; i <= info.max; ++i) {
      if (((1llu << i) & info.mask) == 0)
         else
      }
   info.hash = _mesa_hash_data_with_seed(&info.mask, sizeof(info.mask), info.hash);
               mtx_lock(&screen->varying_info_mutex);
   set_entry *pentry = _mesa_set_search_pre_hashed(screen->varying_info_set, info.hash, &info);
   if (pentry != nullptr) {
      mtx_unlock(&screen->varying_info_mutex);
      }
   else {
      d3d12_varying_info *key = MALLOC_STRUCT(d3d12_varying_info);
                     mtx_unlock(&screen->varying_info_mutex);
         }
      static void
   fill_flat_varyings(struct d3d12_gs_variant_key *key, d3d12_shader_selector *fs)
   {
      if (!fs || !fs->current)
            nir_foreach_variable_with_modes(input, fs->current->nir,
            if (input->data.interpolation == INTERP_MODE_FLAT)
         }
      bool
   d3d12_compare_varying_info(const d3d12_varying_info *expect, const d3d12_varying_info *have)
   {
      if (expect == have)
            if (expect == nullptr || have == nullptr)
            if (expect->mask != have->mask
      || expect->max != have->max)
         if (!expect->mask)
            /* 6 is a rough (wild) guess for a bulk memcmp cross-over point.  When there
   * are a small number of slots present, individual   is much faster. */
   if (util_bitcount64(expect->mask) < 6) {
      uint64_t mask = expect->mask;
   while (mask) {
      int slot = u_bit_scan64(&mask);
   if (memcmp(&expect->slots[slot], &have->slots[slot], sizeof(have->slots[slot])))
                              }
         uint32_t varying_info_hash(const void *info) {
         }
   bool varying_info_compare(const void *a, const void *b) {
         }
   void varying_info_entry_destroy(set_entry *entry) {
      if (entry->key)
      }
      void
   d3d12_varying_cache_init(struct d3d12_screen *screen) {
         }
      void
   d3d12_varying_cache_destroy(struct d3d12_screen *screen) {
         }
         static void
   validate_geometry_shader_variant(struct d3d12_selection_context *sel_ctx)
   {
      struct d3d12_context *ctx = sel_ctx->ctx;
            /* Nothing to do if there is a user geometry shader bound */
   if (gs != NULL && !gs->is_variant)
            d3d12_shader_selector* vs = ctx->gfx_stages[PIPE_SHADER_VERTEX];
            struct d3d12_gs_variant_key key;
   key.all = 0;
            /* Fill the geometry shader variant key */
   if (sel_ctx->fill_mode_lowered != PIPE_POLYGON_MODE_FILL) {
      key.fill_mode = sel_ctx->fill_mode_lowered;
   key.cull_mode = sel_ctx->cull_mode_lowered;
   key.has_front_face = BITSET_TEST(fs->initial->info.system_values_read, SYSTEM_VALUE_FRONT_FACE);
   if (key.cull_mode != PIPE_FACE_NONE || key.has_front_face)
         key.edge_flag_fix = needs_edge_flag_fix(ctx->initial_api_prim);
   fill_flat_varyings(&key, fs);
   if (key.flat_varyings != 0)
      } else if (sel_ctx->needs_point_sprite_lowering) {
         } else if (sel_ctx->needs_vertex_reordering) {
      /* TODO support cases where flat shading (pv != 0) and xfb are enabled */
   key.provoking_vertex = sel_ctx->provoking_vertex;
               if (vs->initial_output_vars == nullptr) {
      vs->initial_output_vars = fill_varyings(sel_ctx->ctx, vs->initial, nir_var_shader_out,
      }
   key.varyings = vs->initial_output_vars;
   gs = d3d12_get_gs_variant(ctx, &key);
      }
      static void
   validate_tess_ctrl_shader_variant(struct d3d12_selection_context *sel_ctx)
   {
      struct d3d12_context *ctx = sel_ctx->ctx;
            /* Nothing to do if there is a user tess ctrl shader bound */
   if (tcs != NULL && !tcs->is_variant)
            d3d12_shader_selector *vs = ctx->gfx_stages[PIPE_SHADER_VERTEX];
   d3d12_shader_selector *tes = ctx->gfx_stages[PIPE_SHADER_TESS_EVAL];
                     /* Fill the variant key */
   if (variant_needed) {
      if (vs->initial_output_vars == nullptr) {
      vs->initial_output_vars = fill_varyings(sel_ctx->ctx, vs->initial, nir_var_shader_out,
      }
   key.varyings = vs->initial_output_vars;
               /* Find/create the proper variant and bind it */
   tcs = variant_needed ? d3d12_get_tcs_variant(ctx, &key) : NULL;
      }
      static bool
   d3d12_compare_shader_keys(struct d3d12_selection_context* sel_ctx, const d3d12_shader_key *expect, const d3d12_shader_key *have)
   {
      assert(expect->stage == have->stage);
   assert(expect);
            if (expect->hash != have->hash)
            switch (expect->stage) {
   case PIPE_SHADER_VERTEX:
      if (expect->vs.needs_format_emulation != have->vs.needs_format_emulation)
            if (expect->vs.needs_format_emulation) {
      if (memcmp(expect->vs.format_conversion, have->vs.format_conversion,
      sel_ctx->ctx->gfx_pipeline_state.ves->num_elements * sizeof(enum pipe_format)))
   }
      case PIPE_SHADER_GEOMETRY:
      if (expect->gs.all != have->gs.all)
            case PIPE_SHADER_TESS_CTRL:
      if (expect->hs.all != have->hs.all ||
      expect->hs.required_patch_outputs != have->hs.required_patch_outputs)
         case PIPE_SHADER_TESS_EVAL:
      if (expect->ds.tcs_vertices_out != have->ds.tcs_vertices_out ||
      expect->ds.prev_patch_outputs != have->ds.prev_patch_outputs ||
   expect->ds.required_patch_inputs != have->ds.required_patch_inputs)
         case PIPE_SHADER_FRAGMENT:
      if (expect->fs.all != have->fs.all)
            case PIPE_SHADER_COMPUTE:
      if (memcmp(expect->cs.workgroup_size, have->cs.workgroup_size,
                  default:
         }
      if (expect->n_texture_states != have->n_texture_states)
            if (expect->n_images != have->n_images)
            if (expect->n_texture_states > 0 && 
      memcmp(expect->tex_wrap_states, have->tex_wrap_states,
               if (memcmp(expect->swizzle_state, have->swizzle_state,
                  if (memcmp(expect->sampler_compare_funcs, have->sampler_compare_funcs,
                  if (memcmp(expect->image_format_conversion, have->image_format_conversion,
      expect->n_images * sizeof(struct d3d12_image_format_conversion_info)))
         return
      expect->required_varying_inputs == have->required_varying_inputs &&
   expect->required_varying_outputs == have->required_varying_outputs &&
   expect->next_varying_inputs == have->next_varying_inputs &&
   expect->prev_varying_outputs == have->prev_varying_outputs &&
   expect->common_all == have->common_all &&
   expect->tex_saturate_s == have->tex_saturate_s &&
   expect->tex_saturate_r == have->tex_saturate_r &&
   }
      static uint32_t
   d3d12_shader_key_hash(const d3d12_shader_key *key)
   {
               hash = (uint32_t)key->stage;
   hash += ((uint64_t)key->required_varying_inputs) +
         hash += ((uint64_t)key->required_varying_outputs) +
            hash += key->next_varying_inputs;
   hash += key->prev_varying_outputs;
   switch (key->stage) {
   case PIPE_SHADER_VERTEX:
      /* (Probably) not worth the bit extraction for needs_format_emulation and
   * the rest of the the format_conversion data is large.  Don't bother
   * hashing for now until this is shown to be worthwhile. */
      case PIPE_SHADER_GEOMETRY:
      hash += key->gs.all;
      case PIPE_SHADER_FRAGMENT:
      hash += key->fs.all;
      case PIPE_SHADER_COMPUTE:
      hash = _mesa_hash_data_with_seed(&key->cs, sizeof(key->cs), hash);
      case PIPE_SHADER_TESS_CTRL:
      hash += key->hs.all;
   hash += ((uint64_t)key->hs.required_patch_outputs) +
            case PIPE_SHADER_TESS_EVAL:
      hash += key->ds.tcs_vertices_out;
   hash += key->ds.prev_patch_outputs;
   hash += ((uint64_t)key->ds.required_patch_inputs) +
            default:
      /* No type specific information to hash for other stages. */
               hash += key->n_texture_states;
   hash += key->n_images;
      }
      static void
   d3d12_fill_shader_key(struct d3d12_selection_context *sel_ctx,
               {
               uint64_t system_generated_in_values =
                  uint64_t system_out_values =
                  memset(key, 0, offsetof(d3d12_shader_key, vs));
            switch (stage)
   {
   case PIPE_SHADER_VERTEX:
      key->vs.needs_format_emulation = 0;
      case PIPE_SHADER_FRAGMENT:
      key->fs.all = 0;
      case PIPE_SHADER_GEOMETRY:
      key->gs.all = 0;
      case PIPE_SHADER_TESS_CTRL:
      key->hs.all = 0;
   key->hs.required_patch_outputs = nullptr;
      case PIPE_SHADER_TESS_EVAL:
      key->ds.tcs_vertices_out = 0;
   key->ds.prev_patch_outputs = 0;
   key->ds.required_patch_inputs = nullptr;
      case PIPE_SHADER_COMPUTE:
      memset(key->cs.workgroup_size, 0, sizeof(key->cs.workgroup_size));
      default: unreachable("Invalid stage type");
            key->n_texture_states = 0;
   key->tex_wrap_states = sel_ctx->ctx->tex_wrap_states_shader_key;
            if (prev) {
      /* We require as inputs what the previous stage has written,
                     switch (stage) {
   case PIPE_SHADER_FRAGMENT:
      system_out_values |= VARYING_BIT_POS | VARYING_BIT_PSIZ | VARYING_BIT_VIEWPORT | VARYING_BIT_LAYER;
   output_vars = &prev->current->output_vars_fs;
      case PIPE_SHADER_GEOMETRY:
      system_out_values |= VARYING_BIT_POS;
   output_vars = &prev->current->output_vars_gs;
      default:
      output_vars = &prev->current->output_vars_default;
                        if (*output_vars == nullptr) {
      *output_vars = fill_varyings(sel_ctx->ctx, prev->current->nir,
                                 if (stage == PIPE_SHADER_TESS_EVAL) {
               if (prev->current->tess_eval_output_vars == nullptr) {
      prev->current->tess_eval_output_vars = fill_varyings(sel_ctx->ctx, prev->current->nir,
               key->ds.required_patch_inputs = prev->current->tess_eval_output_vars;
               /* Set the provoking vertex based on the previous shader output. Only set the
   * key value if the driver actually supports changing the provoking vertex though */
   if (stage == PIPE_SHADER_FRAGMENT && sel_ctx->ctx->gfx_pipeline_state.rast &&
      !sel_ctx->needs_vertex_reordering &&
               /* Get the input clip distance size. The info's clip_distance_array_size corresponds
   * to the output, and in cases of TES or GS you could have differently-sized inputs
   * and outputs. For FS, there is no output, so it's repurposed to mean input.
   */
   if (stage != PIPE_SHADER_FRAGMENT)
               /* We require as outputs what the next stage reads,
   * except certain system values */
   if (next) {
                           if (stage == PIPE_SHADER_VERTEX) {
      system_generated_in_values |= VARYING_BIT_POS;
                        if (*input_vars == nullptr) {
      *input_vars = fill_varyings(sel_ctx->ctx, next->current->nir,
                                             if (prev->current->tess_ctrl_input_vars == nullptr){
                        key->hs.required_patch_outputs = prev->current->tess_ctrl_input_vars;
         }
                  if (stage == PIPE_SHADER_GEOMETRY ||
      ((stage == PIPE_SHADER_VERTEX || stage == PIPE_SHADER_TESS_EVAL) &&
         key->last_vertex_processing_stage = 1;
   key->invert_depth = sel_ctx->ctx->reverse_depth_range;
   key->halfz = sel_ctx->ctx->gfx_pipeline_state.rast ?
         if (sel_ctx->ctx->pstipple.enabled &&
      sel_ctx->ctx->gfx_pipeline_state.rast->base.poly_stipple_enable)
            if (stage == PIPE_SHADER_GEOMETRY && sel_ctx->ctx->gfx_pipeline_state.rast) {
      struct pipe_rasterizer_state *rast = &sel_ctx->ctx->gfx_pipeline_state.rast->base;
   if (sel_ctx->needs_point_sprite_lowering) {
      key->gs.writes_psize = 1;
   key->gs.point_size_per_vertex = rast->point_size_per_vertex;
   key->gs.sprite_coord_enable = rast->sprite_coord_enable;
   key->gs.sprite_origin_upper_left = (rast->sprite_coord_mode != PIPE_SPRITE_COORD_LOWER_LEFT);
   if (sel_ctx->ctx->flip_y < 0)
         key->gs.aa_point = rast->point_smooth;
      } else if (sel_ctx->fill_mode_lowered == PIPE_POLYGON_MODE_LINE) {
         } else if (sel_ctx->needs_vertex_reordering && !sel->is_variant) {
                  if (sel->is_variant && next && next->initial->info.inputs_read & VARYING_BIT_PRIMITIVE_ID)
      } else if (stage == PIPE_SHADER_FRAGMENT) {
      key->fs.missing_dual_src_outputs = sel_ctx->missing_dual_src_outputs;
   key->fs.frag_result_color_lowering = sel_ctx->frag_result_color_lowering;
   key->fs.manual_depth_range = sel_ctx->manual_depth_range;
   key->fs.polygon_stipple = sel_ctx->ctx->pstipple.enabled &&
         key->fs.multisample_disabled = sel_ctx->ctx->gfx_pipeline_state.rast &&
         if (sel_ctx->ctx->gfx_pipeline_state.blend &&
      sel_ctx->ctx->gfx_pipeline_state.blend->desc.RenderTarget[0].LogicOpEnable &&
   !sel_ctx->ctx->gfx_pipeline_state.has_float_rtv) {
   key->fs.cast_to_uint = util_format_is_unorm(sel_ctx->ctx->fb.cbufs[0]->format);
         } else if (stage == PIPE_SHADER_TESS_CTRL) {
      if (next && next->current->nir->info.stage == MESA_SHADER_TESS_EVAL) {
      key->hs.primitive_mode = next->current->nir->info.tess._primitive_mode;
   key->hs.ccw = next->current->nir->info.tess.ccw;
   key->hs.point_mode = next->current->nir->info.tess.point_mode;
      } else {
      key->hs.primitive_mode = TESS_PRIMITIVE_QUADS;
   key->hs.ccw = true;
   key->hs.point_mode = false;
      }
      } else if (stage == PIPE_SHADER_TESS_EVAL) {
      if (prev && prev->current->nir->info.stage == MESA_SHADER_TESS_CTRL)
         else
               if (sel->samples_int_textures) {
      key->samples_int_textures = sel->samples_int_textures;
   key->n_texture_states = sel_ctx->ctx->num_sampler_views[stage];
   /* Copy only states with integer textures */
   for(int i = 0; i < key->n_texture_states; ++i) {
      auto& wrap_state = sel_ctx->ctx->tex_wrap_states[stage][i];
   if (wrap_state.is_int_sampler) {
      memcpy(&key->tex_wrap_states[i], &wrap_state, sizeof(wrap_state));
      }
   else
                  for (unsigned i = 0, e = sel_ctx->ctx->num_samplers[stage]; i < e; ++i) {
      if (!sel_ctx->ctx->samplers[stage][i] ||
                  if (sel_ctx->ctx->samplers[stage][i]->wrap_r == PIPE_TEX_WRAP_CLAMP)
         if (sel_ctx->ctx->samplers[stage][i]->wrap_s == PIPE_TEX_WRAP_CLAMP)
         if (sel_ctx->ctx->samplers[stage][i]->wrap_t == PIPE_TEX_WRAP_CLAMP)
               if (sel->compare_with_lod_bias_grad) {
      key->n_texture_states = sel_ctx->ctx->num_sampler_views[stage];
   memcpy(key->sampler_compare_funcs, sel_ctx->ctx->tex_compare_func[stage],
         memcpy(key->swizzle_state, sel_ctx->ctx->tex_swizzle_state[stage],
         if (!sel->samples_int_textures) 
               if (stage == PIPE_SHADER_VERTEX && sel_ctx->ctx->gfx_pipeline_state.ves) {
      key->vs.needs_format_emulation = sel_ctx->ctx->gfx_pipeline_state.ves->needs_format_emulation;
   if (key->vs.needs_format_emulation) {
               memset(key->vs.format_conversion + num_elements,
                  memcpy(key->vs.format_conversion, sel_ctx->ctx->gfx_pipeline_state.ves->format_conversion,
                  if (stage == PIPE_SHADER_FRAGMENT &&
      sel_ctx->ctx->gfx_stages[PIPE_SHADER_GEOMETRY] &&
   sel_ctx->ctx->gfx_stages[PIPE_SHADER_GEOMETRY]->is_variant &&
   sel_ctx->ctx->gfx_stages[PIPE_SHADER_GEOMETRY]->gs_key.has_front_face) {
               if (stage == PIPE_SHADER_COMPUTE && sel_ctx->variable_workgroup_size) {
                  key->n_images = sel_ctx->ctx->num_image_views[stage];
   for (int i = 0; i < key->n_images; ++i) {
      key->image_format_conversion[i].emulated_format = sel_ctx->ctx->image_view_emulation_formats[stage][i];
   if (key->image_format_conversion[i].emulated_format != PIPE_FORMAT_NONE)
                  }
      static void
   select_shader_variant(struct d3d12_selection_context *sel_ctx, d3d12_shader_selector *sel,
         {
      struct d3d12_context *ctx = sel_ctx->ctx;
   d3d12_shader_key key;
   nir_shader *new_nir_variant;
                     /* Check for an existing variant */
   for (d3d12_shader *variant = sel->first; variant;
               if (d3d12_compare_shader_keys(sel_ctx, &key, &variant->key)) {
      sel->current = variant;
                  /* Clone the NIR shader */
            /* Apply any needed lowering passes */
   if (key.stage == PIPE_SHADER_GEOMETRY) {
      if (key.gs.writes_psize) {
      NIR_PASS_V(new_nir_variant, d3d12_lower_point_sprite,
            !key.gs.sprite_origin_upper_left,
               nir_function_impl *impl = nir_shader_get_entrypoint(new_nir_variant);
               if (key.gs.primitive_id) {
               nir_function_impl *impl = nir_shader_get_entrypoint(new_nir_variant);
               if (key.gs.triangle_strip)
      }
   else if (key.stage == PIPE_SHADER_FRAGMENT)
   {
      if (key.fs.polygon_stipple) {
                     nir_function_impl *impl = nir_shader_get_entrypoint(new_nir_variant);
               if (key.fs.remap_front_facing) {
               nir_function_impl *impl = nir_shader_get_entrypoint(new_nir_variant);
               if (key.fs.missing_dual_src_outputs) {
      NIR_PASS_V(new_nir_variant, d3d12_add_missing_dual_src_target,
      } else if (key.fs.frag_result_color_lowering) {
      NIR_PASS_V(new_nir_variant, nir_lower_fragcolor,
               if (key.fs.manual_depth_range)
                  if (sel->compare_with_lod_bias_grad) {
      STATIC_ASSERT(sizeof(dxil_texture_swizzle_state) ==
            NIR_PASS_V(new_nir_variant, nir_lower_tex_shadow, key.n_texture_states,
               if (key.stage == PIPE_SHADER_FRAGMENT) {
      if (key.fs.cast_to_uint)
         if (key.fs.cast_to_int)
               if (key.n_images) {
      d3d12_image_format_conversion_info_arr image_format_arr = { key.n_images, key.image_format_conversion };
               if (key.stage == PIPE_SHADER_COMPUTE && sel->workgroup_size_variable) {
      new_nir_variant->info.workgroup_size[0] = key.cs.workgroup_size[0];
   new_nir_variant->info.workgroup_size[1] = key.cs.workgroup_size[1];
               if (new_nir_variant->info.stage == MESA_SHADER_TESS_CTRL) {
      new_nir_variant->info.tess._primitive_mode = (tess_primitive_mode)key.hs.primitive_mode;
   new_nir_variant->info.tess.ccw = key.hs.ccw;
   new_nir_variant->info.tess.point_mode = key.hs.point_mode;
               } else if (new_nir_variant->info.stage == MESA_SHADER_TESS_EVAL) {
                  {
      struct nir_lower_tex_options tex_options = { };
   tex_options.lower_txp = ~0u; /* No equivalent for textureProj */
   tex_options.lower_rect = true;
   tex_options.lower_rect_offset = true;
   tex_options.saturate_s = key.tex_saturate_s;
   tex_options.saturate_r = key.tex_saturate_r;
   tex_options.saturate_t = key.tex_saturate_t;
   tex_options.lower_invalid_implicit_lod = true;
                        /* Add the needed in and outputs, and re-sort */
   if (prev) {
      if (key.required_varying_inputs != nullptr) {
      uint64_t mask = key.required_varying_inputs->mask & ~new_nir_variant->info.inputs_read;
   new_nir_variant->info.inputs_read |= mask;
   while (mask) {
      int slot = u_bit_scan64(&mask);
                  if (sel->stage == PIPE_SHADER_TESS_EVAL) {
      uint32_t patch_mask = (uint32_t)key.ds.required_patch_inputs->mask & ~new_nir_variant->info.patch_inputs_read;
   new_nir_variant->info.patch_inputs_read |= patch_mask;
   while (patch_mask) {
      int slot = u_bit_scan(&patch_mask);
         }
   dxil_reassign_driver_locations(new_nir_variant, nir_var_shader_in,
                  if (next) {
      if (key.required_varying_outputs != nullptr) {
      uint64_t mask = key.required_varying_outputs->mask & ~new_nir_variant->info.outputs_written;
   new_nir_variant->info.outputs_written |= mask;
   while (mask) {
      int slot = u_bit_scan64(&mask);
                  if (sel->stage == PIPE_SHADER_TESS_CTRL &&
            uint32_t patch_mask = (uint32_t)key.hs.required_patch_outputs->mask & ~new_nir_variant->info.patch_outputs_written;
   new_nir_variant->info.patch_outputs_written |= patch_mask;
   while (patch_mask) {
      int slot = u_bit_scan(&patch_mask);
         }
   dxil_reassign_driver_locations(new_nir_variant, nir_var_shader_out,
               d3d12_shader *new_variant = compile_nir(ctx, sel, &key, new_nir_variant);
            /* keep track of polygon stipple texture binding */
            /* prepend the new shader in the selector chain and pick it */
   new_variant->next_variant = sel->first;
      }
      static d3d12_shader_selector *
   get_prev_shader(struct d3d12_context *ctx, pipe_shader_type current)
   {
      switch (current) {
   case PIPE_SHADER_VERTEX:
         case PIPE_SHADER_FRAGMENT:
      if (ctx->gfx_stages[PIPE_SHADER_GEOMETRY])
            case PIPE_SHADER_GEOMETRY:
      if (ctx->gfx_stages[PIPE_SHADER_TESS_EVAL])
            case PIPE_SHADER_TESS_EVAL:
      if (ctx->gfx_stages[PIPE_SHADER_TESS_CTRL])
            case PIPE_SHADER_TESS_CTRL:
         default:
            }
      static d3d12_shader_selector *
   get_next_shader(struct d3d12_context *ctx, pipe_shader_type current)
   {
      switch (current) {
   case PIPE_SHADER_VERTEX:
      if (ctx->gfx_stages[PIPE_SHADER_TESS_CTRL])
            case PIPE_SHADER_TESS_CTRL:
      if (ctx->gfx_stages[PIPE_SHADER_TESS_EVAL])
            case PIPE_SHADER_TESS_EVAL:
      if (ctx->gfx_stages[PIPE_SHADER_GEOMETRY])
            case PIPE_SHADER_GEOMETRY:
         case PIPE_SHADER_FRAGMENT:
         default:
            }
      enum tex_scan_flags {
      TEX_SAMPLE_INTEGER_TEXTURE = 1 << 0,
   TEX_CMP_WITH_LOD_BIAS_GRAD = 1 << 1,
      };
      static unsigned
   scan_texture_use(nir_shader *nir)
   {
      unsigned result = 0;
   nir_foreach_function_impl(impl, nir) {
      nir_foreach_block(block, impl) {
      nir_foreach_instr(instr, block) {
      if (instr->type == nir_instr_type_tex) {
      auto tex = nir_instr_as_tex(instr);
   switch (tex->op) {
   case nir_texop_txb:
   case nir_texop_txl:
   case nir_texop_txd:
      if (tex->is_shadow)
            case nir_texop_tex:
      if (tex->dest_type & (nir_type_int | nir_type_uint))
      default:
            }
   if (TEX_SCAN_ALL_FLAGS == result)
            }
      }
      static uint64_t
   update_so_info(struct pipe_stream_output_info *so_info,
         {
      uint64_t so_outputs = 0;
   uint8_t reverse_map[64] = {0};
            while (outputs_written)
            for (unsigned i = 0; i < so_info->num_outputs; i++) {
               /* Map Gallium's condensed "slots" back to real VARYING_SLOT_* enums */
                           }
      static struct d3d12_shader_selector *
   d3d12_create_shader_impl(struct d3d12_context *ctx,
                           {
      unsigned tex_scan_result = scan_texture_use(nir);
   sel->samples_int_textures = (tex_scan_result & TEX_SAMPLE_INTEGER_TEXTURE) != 0;
   sel->compare_with_lod_bias_grad = (tex_scan_result & TEX_CMP_WITH_LOD_BIAS_GRAD) != 0;
   sel->workgroup_size_variable = nir->info.workgroup_size_variable;
      /* Integer cube maps are not supported in DirectX because sampling is not supported
   * on integer textures and TextureLoad is not supported for cube maps, so we have to
   * lower integer cube maps to be handled like 2D textures arrays*/
            /* Keep this initial shader as the blue print for possible variants */
   sel->initial = nir;
   sel->initial_output_vars = nullptr;
   sel->gs_key.varyings = nullptr;
            /*
   * We must compile some shader here, because if the previous or a next shaders exists later
   * when the shaders are bound, then the key evaluation in the shader selector will access
   * the current variant of these  prev and next shader, and we can only assign
   * a current variant when it has been successfully compiled.
   *
   * For shaders that require lowering because certain instructions are not available
   * and their emulation is state depended (like sampling an integer texture that must be
   * emulated and needs handling of boundary conditions, or shadow compare sampling with LOD),
   * we must go through the shader selector here to create a compilable variant.
   * For shaders that are not depended on the state this is just compiling the original
   * shader.
   *
   * TODO: get rid of having to compiling the shader here if it can be forseen that it will
   * be thrown away (i.e. it depends on states that are likely to change before the shader is
   * used for the first time)
   */
   struct d3d12_selection_context sel_ctx = {0};
   sel_ctx.ctx = ctx;
            if (!sel->current) {
      ralloc_free(sel);
                  }
      struct d3d12_shader_selector *
   d3d12_create_shader(struct d3d12_context *ctx,
               {
      struct d3d12_shader_selector *sel = rzalloc(nullptr, d3d12_shader_selector);
                     if (shader->type == PIPE_SHADER_IR_NIR) {
         } else {
      assert(shader->type == PIPE_SHADER_IR_TGSI);
               nir_shader_gather_info(nir, nir_shader_get_entrypoint(nir));
   memcpy(&sel->so_info, &shader->stream_output, sizeof(sel->so_info));
            assert(nir != NULL);
   d3d12_shader_selector *prev = get_prev_shader(ctx, sel->stage);
            NIR_PASS_V(nir, dxil_nir_split_clip_cull_distance);
            if (nir->info.stage != MESA_SHADER_VERTEX)
      nir->info.inputs_read =
            else
            if (nir->info.stage != MESA_SHADER_FRAGMENT) {
      nir->info.outputs_written =
            } else {
      NIR_PASS_V(nir, nir_lower_fragcoord_wtrans);
   NIR_PASS_V(nir, dxil_nir_lower_sample_pos);
                  }
      struct d3d12_shader_selector *
   d3d12_create_compute_shader(struct d3d12_context *ctx,
         {
      struct d3d12_shader_selector *sel = rzalloc(nullptr, d3d12_shader_selector);
                     if (shader->ir_type == PIPE_SHADER_IR_NIR) {
         } else {
      assert(shader->ir_type == PIPE_SHADER_IR_TGSI);
                                    }
      void
   d3d12_select_shader_variants(struct d3d12_context *ctx, const struct pipe_draw_info *dinfo)
   {
               sel_ctx.ctx = ctx;
   sel_ctx.needs_point_sprite_lowering = needs_point_sprite_lowering(ctx, dinfo);
   sel_ctx.fill_mode_lowered = fill_mode_lowered(ctx, dinfo);
   sel_ctx.cull_mode_lowered = cull_mode_lowered(ctx, sel_ctx.fill_mode_lowered);
   sel_ctx.provoking_vertex = get_provoking_vertex(&sel_ctx, &sel_ctx.alternate_tri, dinfo);
   sel_ctx.needs_vertex_reordering = needs_vertex_reordering(&sel_ctx, dinfo);
   sel_ctx.missing_dual_src_outputs = ctx->missing_dual_src_outputs;
   sel_ctx.frag_result_color_lowering = frag_result_color_lowering(ctx);
            d3d12_shader_selector* gs = ctx->gfx_stages[PIPE_SHADER_GEOMETRY];
   if (gs == nullptr || gs->is_variant) {
      if (sel_ctx.fill_mode_lowered != PIPE_POLYGON_MODE_FILL || sel_ctx.needs_point_sprite_lowering || sel_ctx.needs_vertex_reordering)
         else if (gs != nullptr) {
                              auto* stages = ctx->gfx_stages;
   d3d12_shader_selector* prev;
   d3d12_shader_selector* next;
   if (stages[PIPE_SHADER_VERTEX]) {
      next = get_next_shader(ctx, PIPE_SHADER_VERTEX);
      }
   if (stages[PIPE_SHADER_TESS_CTRL]) {
      prev = get_prev_shader(ctx, PIPE_SHADER_TESS_CTRL);
   next = get_next_shader(ctx, PIPE_SHADER_TESS_CTRL);
      }
   if (stages[PIPE_SHADER_TESS_EVAL]) {
      prev = get_prev_shader(ctx, PIPE_SHADER_TESS_EVAL);
   next = get_next_shader(ctx, PIPE_SHADER_TESS_EVAL);
      }
   if (stages[PIPE_SHADER_GEOMETRY]) {
      prev = get_prev_shader(ctx, PIPE_SHADER_GEOMETRY);
   next = get_next_shader(ctx, PIPE_SHADER_GEOMETRY);
      }
   if (stages[PIPE_SHADER_FRAGMENT]) {
      prev = get_prev_shader(ctx, PIPE_SHADER_FRAGMENT);
         }
      static const unsigned *
   workgroup_size_variable(struct d3d12_context *ctx,
         {
      if (ctx->compute_state->workgroup_size_variable)
            }
      void
   d3d12_select_compute_shader_variants(struct d3d12_context *ctx, const struct pipe_grid_info *info)
   {
               sel_ctx.ctx = ctx;
               }
      void
   d3d12_shader_free(struct d3d12_shader_selector *sel)
   {
      auto shader = sel->first;
   while (shader) {
      free(shader->bytecode);
               ralloc_free((void*)sel->initial);
      }
