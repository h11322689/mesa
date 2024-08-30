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
      #ifdef _GAMING_XBOX
   #ifdef _GAMING_XBOX_SCARLETT
   #include <d3dx12_xs.h>
   #else
   #include <d3dx12_x.h>
   #endif
   #endif
      #include "d3d12_pipeline_state.h"
   #include "d3d12_compiler.h"
   #include "d3d12_context.h"
   #include "d3d12_screen.h"
   #ifndef _GAMING_XBOX
   #include <directx/d3dx12_pipeline_state_stream.h>
   #endif
      #include "util/hash_table.h"
   #include "util/set.h"
   #include "util/u_memory.h"
   #include "util/u_prim.h"
      #include <dxguids/dxguids.h>
      struct d3d12_gfx_pso_entry {
      struct d3d12_gfx_pipeline_state key;
      };
      struct d3d12_compute_pso_entry {
      struct d3d12_compute_pipeline_state key;
      };
      static const char *
   get_semantic_name(int location, int driver_location, unsigned *index)
   {
                        case VARYING_SLOT_POS:
            case VARYING_SLOT_FACE:
            case VARYING_SLOT_CLIP_DIST1:
      *index = 1;
      case VARYING_SLOT_CLIP_DIST0:
            case VARYING_SLOT_PRIMITIVE_ID:
            case VARYING_SLOT_VIEWPORT:
            case VARYING_SLOT_LAYER:
            default: {
         *index = driver_location;
            }
      static nir_variable *
   find_so_variable(nir_shader *s, int location, unsigned location_frac, unsigned num_components)
   {
      nir_foreach_variable_with_modes(var, s, nir_var_shader_out) {
      if (var->data.location != location || var->data.location_frac > location_frac)
         unsigned var_num_components = var->data.compact ?
         if (var->data.location_frac <= location_frac &&
      var->data.location_frac + var_num_components >= location_frac + num_components)
   }
      }
      static void
   fill_so_declaration(const struct pipe_stream_output_info *info,
                     {
                        for (unsigned i = 0; i < info->num_outputs; i++) {
      const struct pipe_stream_output *output = &info->output[i];
   const int buffer = output->output_buffer;
            /* Mesa doesn't store entries for gl_SkipComponents in the Outputs[]
   * array.  Instead, it simply increments DstOffset for the following
   * input by the number of components that should be skipped.
   *
   * DirectX12 requires that we create gap entries.
   */
            if (skip_components > 0) {
      entries[*num_entries].Stream = output->stream;
   entries[*num_entries].SemanticName = NULL;
   entries[*num_entries].ComponentCount = skip_components;
   entries[*num_entries].OutputSlot = buffer;
                        entries[*num_entries].Stream = output->stream;
   nir_variable *var = find_so_variable(last_vertex_stage,
         assert((var->data.stream & ~NIR_STREAM_PACKED) == output->stream);
   entries[*num_entries].SemanticName = get_semantic_name(var->data.location,
         entries[*num_entries].SemanticIndex = index;
   entries[*num_entries].StartComponent = output->start_component - var->data.location_frac;
   entries[*num_entries].ComponentCount = output->num_components;
   entries[*num_entries].OutputSlot = buffer;
               for (unsigned i = 0; i < PIPE_MAX_VERTEX_STREAMS; i++)
            }
      static bool
   depth_bias(struct d3d12_rasterizer_state *state, enum mesa_prim reduced_prim)
   {
      /* glPolygonOffset is supposed to be only enabled when rendering polygons.
   * In d3d12 case, all polygons (and quads) are lowered to triangles */
   if (reduced_prim != MESA_PRIM_TRIANGLES)
            unsigned fill_mode = state->base.cull_face == PIPE_FACE_FRONT ? state->base.fill_back
            switch (fill_mode) {
   case PIPE_POLYGON_MODE_FILL:
            case PIPE_POLYGON_MODE_LINE:
            case PIPE_POLYGON_MODE_POINT:
            default:
            }
      static D3D12_PRIMITIVE_TOPOLOGY_TYPE
   topology_type(enum mesa_prim reduced_prim)
   {
      switch (reduced_prim) {
   case MESA_PRIM_POINTS:
            case MESA_PRIM_LINES:
            case MESA_PRIM_TRIANGLES:
            case MESA_PRIM_PATCHES:
            default:
      debug_printf("mesa_prim: %s\n", u_prim_name(reduced_prim));
         }
      DXGI_FORMAT
   d3d12_rtv_format(struct d3d12_context *ctx, unsigned index)
   {
               if (ctx->gfx_pipeline_state.blend->desc.RenderTarget[0].LogicOpEnable &&
      !ctx->gfx_pipeline_state.has_float_rtv) {
   switch (fmt) {
   case DXGI_FORMAT_R8G8B8A8_SNORM:
   case DXGI_FORMAT_R8G8B8A8_UNORM:
   case DXGI_FORMAT_B8G8R8A8_UNORM:
   case DXGI_FORMAT_B8G8R8X8_UNORM:
         default:
                        }
      static ID3D12PipelineState *
   create_gfx_pipeline_state(struct d3d12_context *ctx)
   {
      struct d3d12_screen *screen = d3d12_screen(ctx->base.screen);
   struct d3d12_gfx_pipeline_state *state = &ctx->gfx_pipeline_state;
   enum mesa_prim reduced_prim = state->prim_type == MESA_PRIM_PATCHES ?
         D3D12_SO_DECLARATION_ENTRY entries[PIPE_MAX_SO_OUTPUTS] = {};
   UINT strides[PIPE_MAX_SO_OUTPUTS] = { 0 };
            CD3DX12_PIPELINE_STATE_STREAM3 pso_desc;
                     if (state->stages[PIPE_SHADER_VERTEX]) {
      auto shader = state->stages[PIPE_SHADER_VERTEX];
   pso_desc.VS = D3D12_SHADER_BYTECODE { shader->bytecode, shader->bytecode_length };
               if (state->stages[PIPE_SHADER_TESS_CTRL]) {
      auto shader = state->stages[PIPE_SHADER_TESS_CTRL];
   pso_desc.HS = D3D12_SHADER_BYTECODE{ shader->bytecode, shader->bytecode_length };
               if (state->stages[PIPE_SHADER_TESS_EVAL]) {
      auto shader = state->stages[PIPE_SHADER_TESS_EVAL];
   pso_desc.DS = D3D12_SHADER_BYTECODE{ shader->bytecode, shader->bytecode_length };
               if (state->stages[PIPE_SHADER_GEOMETRY]) {
      auto shader = state->stages[PIPE_SHADER_GEOMETRY];
   pso_desc.GS = D3D12_SHADER_BYTECODE{ shader->bytecode, shader->bytecode_length };
               bool last_vertex_stage_writes_pos = (last_vertex_stage_nir->info.outputs_written & VARYING_BIT_POS) != 0;
   if (last_vertex_stage_writes_pos && state->stages[PIPE_SHADER_FRAGMENT] &&
      !state->rast->base.rasterizer_discard) {
   auto shader = state->stages[PIPE_SHADER_FRAGMENT];
               if (state->num_so_targets)
            D3D12_STREAM_OUTPUT_DESC& stream_output_desc = (D3D12_STREAM_OUTPUT_DESC&)pso_desc.StreamOutput;
   stream_output_desc.NumEntries = num_entries;
   stream_output_desc.pSODeclaration = entries;
   stream_output_desc.RasterizedStream = state->rast->base.rasterizer_discard ? D3D12_SO_NO_RASTERIZED_STREAM : 0;
   stream_output_desc.NumStrides = num_strides;
   stream_output_desc.pBufferStrides = strides;
            D3D12_BLEND_DESC& blend_state = (D3D12_BLEND_DESC&)pso_desc.BlendState;
   blend_state = state->blend->desc;
   if (state->has_float_rtv)
            (d3d12_depth_stencil_desc_type&)pso_desc.DepthStencilState = state->zsa->desc;
            D3D12_RASTERIZER_DESC& rast = (D3D12_RASTERIZER_DESC&)pso_desc.RasterizerState;
            if (reduced_prim != MESA_PRIM_TRIANGLES)
            if (depth_bias(state->rast, reduced_prim)) {
      rast.DepthBias = state->rast->base.offset_units * 2;
   rast.DepthBiasClamp = state->rast->base.offset_clamp;
      }
   D3D12_INPUT_LAYOUT_DESC& input_layout = (D3D12_INPUT_LAYOUT_DESC&)pso_desc.InputLayout;
   input_layout.pInputElementDescs = state->ves->elements;
                              D3D12_RT_FORMAT_ARRAY& render_targets = (D3D12_RT_FORMAT_ARRAY&)pso_desc.RTVFormats;
   render_targets.NumRenderTargets = state->num_cbufs;
   for (unsigned i = 0; i < state->num_cbufs; ++i)
                  DXGI_SAMPLE_DESC& samples = (DXGI_SAMPLE_DESC&)pso_desc.SampleDesc;
   samples.Count = state->samples;
   if (state->num_cbufs || state->dsv_format != DXGI_FORMAT_UNKNOWN) {
      if (!state->zsa->desc.DepthEnable &&
      !state->zsa->desc.StencilEnable &&
   !state->rast->desc.MultisampleEnable &&
   state->samples > 1) {
   rast.ForcedSampleCount = 1;
            #ifndef _GAMING_XBOX
      else if (state->samples > 1 &&
            samples.Count = 1;
         #endif
                        D3D12_CACHED_PIPELINE_STATE& cached_pso = (D3D12_CACHED_PIPELINE_STATE&)pso_desc.CachedPSO;
   cached_pso.pCachedBlob = NULL;
                              if (screen->opts14.IndependentFrontAndBackStencilRefMaskSupported) {
      D3D12_PIPELINE_STATE_STREAM_DESC pso_stream_desc{
      sizeof(pso_desc),
               if (FAILED(screen->dev->CreatePipelineState(&pso_stream_desc,
            debug_printf("D3D12: CreateGraphicsPipelineState failed!\n");
         } 
   else {
      D3D12_GRAPHICS_PIPELINE_STATE_DESC v0desc = pso_desc.GraphicsDescV0();
   if (FAILED(screen->dev->CreateGraphicsPipelineState(&v0desc,
            debug_printf("D3D12: CreateGraphicsPipelineState failed!\n");
                     }
      static uint32_t
   hash_gfx_pipeline_state(const void *key)
   {
         }
      static bool
   equals_gfx_pipeline_state(const void *a, const void *b)
   {
         }
      ID3D12PipelineState *
   d3d12_get_gfx_pipeline_state(struct d3d12_context *ctx)
   {
      uint32_t hash = hash_gfx_pipeline_state(&ctx->gfx_pipeline_state);
   struct hash_entry *entry = _mesa_hash_table_search_pre_hashed(ctx->pso_cache, hash,
         if (!entry) {
      struct d3d12_gfx_pso_entry *data = (struct d3d12_gfx_pso_entry *)MALLOC(sizeof(struct d3d12_gfx_pso_entry));
   if (!data)
            data->key = ctx->gfx_pipeline_state;
   data->pso = create_gfx_pipeline_state(ctx);
   if (!data->pso) {
      FREE(data);
               entry = _mesa_hash_table_insert_pre_hashed(ctx->pso_cache, hash, &data->key, data);
                  }
      void
   d3d12_gfx_pipeline_state_cache_init(struct d3d12_context *ctx)
   {
         }
      static void
   delete_gfx_entry(struct hash_entry *entry)
   {
      struct d3d12_gfx_pso_entry *data = (struct d3d12_gfx_pso_entry *)entry->data;
   data->pso->Release();
      }
      static void
   remove_gfx_entry(struct d3d12_context *ctx, struct hash_entry *entry)
   {
               if (ctx->current_gfx_pso == data->pso)
         _mesa_hash_table_remove(ctx->pso_cache, entry);
      }
      void
   d3d12_gfx_pipeline_state_cache_destroy(struct d3d12_context *ctx)
   {
         }
      void
   d3d12_gfx_pipeline_state_cache_invalidate(struct d3d12_context *ctx, const void *state)
   {
      hash_table_foreach(ctx->pso_cache, entry) {
      const struct d3d12_gfx_pipeline_state *key = (struct d3d12_gfx_pipeline_state *)entry->key;
   if (key->blend == state || key->zsa == state || key->rast == state)
         }
      void
   d3d12_gfx_pipeline_state_cache_invalidate_shader(struct d3d12_context *ctx,
               {
               while (shader) {
      hash_table_foreach(ctx->pso_cache, entry) {
      const struct d3d12_gfx_pipeline_state *key = (struct d3d12_gfx_pipeline_state *)entry->key;
   if (key->stages[stage] == shader)
      }
         }
      static ID3D12PipelineState *
   create_compute_pipeline_state(struct d3d12_context *ctx)
   {
      struct d3d12_screen *screen = d3d12_screen(ctx->base.screen);
            D3D12_COMPUTE_PIPELINE_STATE_DESC pso_desc = { 0 };
            if (state->stage) {
      auto shader = state->stage;
   pso_desc.CS.BytecodeLength = shader->bytecode_length;
                        pso_desc.CachedPSO.pCachedBlob = NULL;
                     ID3D12PipelineState *ret;
   if (FAILED(screen->dev->CreateComputePipelineState(&pso_desc,
            debug_printf("D3D12: CreateComputePipelineState failed!\n");
                  }
      static uint32_t
   hash_compute_pipeline_state(const void *key)
   {
         }
      static bool
   equals_compute_pipeline_state(const void *a, const void *b)
   {
         }
      ID3D12PipelineState *
   d3d12_get_compute_pipeline_state(struct d3d12_context *ctx)
   {
      uint32_t hash = hash_compute_pipeline_state(&ctx->compute_pipeline_state);
   struct hash_entry *entry = _mesa_hash_table_search_pre_hashed(ctx->compute_pso_cache, hash,
         if (!entry) {
      struct d3d12_compute_pso_entry *data = (struct d3d12_compute_pso_entry *)MALLOC(sizeof(struct d3d12_compute_pso_entry));
   if (!data)
            data->key = ctx->compute_pipeline_state;
   data->pso = create_compute_pipeline_state(ctx);
   if (!data->pso) {
      FREE(data);
               entry = _mesa_hash_table_insert_pre_hashed(ctx->compute_pso_cache, hash, &data->key, data);
                  }
      void
   d3d12_compute_pipeline_state_cache_init(struct d3d12_context *ctx)
   {
         }
      static void
   delete_compute_entry(struct hash_entry *entry)
   {
      struct d3d12_compute_pso_entry *data = (struct d3d12_compute_pso_entry *)entry->data;
   data->pso->Release();
      }
      static void
   remove_compute_entry(struct d3d12_context *ctx, struct hash_entry *entry)
   {
               if (ctx->current_compute_pso == data->pso)
         _mesa_hash_table_remove(ctx->compute_pso_cache, entry);
      }
      void
   d3d12_compute_pipeline_state_cache_destroy(struct d3d12_context *ctx)
   {
         }
      void
   d3d12_compute_pipeline_state_cache_invalidate_shader(struct d3d12_context *ctx,
         {
               while (shader) {
      hash_table_foreach(ctx->compute_pso_cache, entry) {
      const struct d3d12_compute_pipeline_state *key = (struct d3d12_compute_pipeline_state *)entry->key;
   if (key->stage == shader)
      }
         }
