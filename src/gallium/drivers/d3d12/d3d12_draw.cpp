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
      #include "d3d12_cmd_signature.h"
   #include "d3d12_compiler.h"
   #include "d3d12_compute_transforms.h"
   #include "d3d12_context.h"
   #include "d3d12_format.h"
   #include "d3d12_query.h"
   #include "d3d12_resource.h"
   #include "d3d12_root_signature.h"
   #include "d3d12_screen.h"
   #include "d3d12_surface.h"
      #include "indices/u_primconvert.h"
   #include "util/u_debug.h"
   #include "util/u_draw.h"
   #include "util/u_helpers.h"
   #include "util/u_inlines.h"
   #include "util/u_prim.h"
   #include "util/u_prim_restart.h"
   #include "util/u_math.h"
      static const D3D12_RECT MAX_SCISSOR = { 0, 0, 16384, 16384 };
      static const D3D12_RECT MAX_SCISSOR_ARRAY[] = {
      MAX_SCISSOR, MAX_SCISSOR, MAX_SCISSOR, MAX_SCISSOR,
   MAX_SCISSOR, MAX_SCISSOR, MAX_SCISSOR, MAX_SCISSOR,
   MAX_SCISSOR, MAX_SCISSOR, MAX_SCISSOR, MAX_SCISSOR,
      };
   static_assert(ARRAY_SIZE(MAX_SCISSOR_ARRAY) == PIPE_MAX_VIEWPORTS, "Wrong scissor count");
      static D3D12_GPU_DESCRIPTOR_HANDLE
   fill_cbv_descriptors(struct d3d12_context *ctx,
               {
      struct d3d12_batch *batch = d3d12_current_batch(ctx);
   struct d3d12_descriptor_handle table_start;
            for (unsigned i = 0; i < shader->num_cb_bindings; i++) {
      unsigned binding = shader->cb_bindings[i].binding;
            D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_desc = {};
   if (buffer && buffer->buffer) {
      struct d3d12_resource *res = d3d12_resource(buffer->buffer);
   d3d12_transition_resource_state(ctx, res, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_TRANSITION_FLAG_ACCUMULATE_STATE);
   cbv_desc.BufferLocation = d3d12_resource_gpu_virtual_address(res) + buffer->buffer_offset;
   cbv_desc.SizeInBytes = MIN2(D3D12_REQ_CONSTANT_BUFFER_ELEMENT_COUNT * 16,
                     struct d3d12_descriptor_handle handle;
   d3d12_descriptor_heap_alloc_handle(batch->view_heap, &handle);
                  }
      static D3D12_GPU_DESCRIPTOR_HANDLE
   fill_srv_descriptors(struct d3d12_context *ctx,
               {
      struct d3d12_batch *batch = d3d12_current_batch(ctx);
   struct d3d12_screen *screen = d3d12_screen(ctx->base.screen);
   D3D12_CPU_DESCRIPTOR_HANDLE descs[PIPE_MAX_SHADER_SAMPLER_VIEWS];
                     for (unsigned i = shader->begin_srv_binding; i < shader->end_srv_binding; i++)
   {
               if (i == shader->pstipple_binding) {
         } else {
                  unsigned desc_idx = i - shader->begin_srv_binding;
   if (view != NULL) {
                     struct d3d12_resource *res = d3d12_resource(view->base.texture);
   /* If this is a buffer that's been replaced, re-create the descriptor */
   if (view->texture_generation_id != res->generation_id) {
      d3d12_init_sampler_view_descriptor(view);
               D3D12_RESOURCE_STATES state = (stage == PIPE_SHADER_FRAGMENT) ?
               if (view->base.texture->target == PIPE_BUFFER) {
      d3d12_transition_resource_state(ctx, d3d12_resource(view->base.texture),
            } else {
      d3d12_transition_subresources_state(ctx, d3d12_resource(view->base.texture),
                                       } else {
                                 }
      static D3D12_GPU_DESCRIPTOR_HANDLE
   fill_ssbo_descriptors(struct d3d12_context *ctx,
               {
      struct d3d12_batch *batch = d3d12_current_batch(ctx);
                     for (unsigned i = 0; i < shader->nir->info.num_ssbos; i++)
   {
               D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc;
   uav_desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
   uav_desc.Format = DXGI_FORMAT_R32_TYPELESS;
   uav_desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
   uav_desc.Buffer.StructureByteStride = 0;
   uav_desc.Buffer.CounterOffsetInBytes = 0;
   uav_desc.Buffer.FirstElement = 0;
   uav_desc.Buffer.NumElements = 0;
   ID3D12Resource *d3d12_res = nullptr;
   if (view->buffer) {
      struct d3d12_resource *res = d3d12_resource(view->buffer);
   uint64_t res_offset = 0;
   d3d12_res = d3d12_resource_underlying(res, &res_offset);
   d3d12_transition_resource_state(ctx, res, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_TRANSITION_FLAG_ACCUMULATE_STATE);
   uav_desc.Buffer.FirstElement = (view->buffer_offset + res_offset) / 4;
   uav_desc.Buffer.NumElements = DIV_ROUND_UP(view->buffer_size, 4);
               struct d3d12_descriptor_handle handle;
   d3d12_descriptor_heap_alloc_handle(batch->view_heap, &handle);
                  }
      static D3D12_GPU_DESCRIPTOR_HANDLE
   fill_sampler_descriptors(struct d3d12_context *ctx,
               {
      const struct d3d12_shader *shader = shader_sel->current;
   struct d3d12_batch *batch = d3d12_current_batch(ctx);
            view.count = 0;
   for (unsigned i = shader->begin_srv_binding; i < shader->end_srv_binding; i++, view.count++) {
               if (i == shader->pstipple_binding) {
         } else {
                  unsigned desc_idx = i - shader->begin_srv_binding;
   if (sampler != NULL) {
      if (sampler->is_shadow_sampler && shader_sel->compare_with_lod_bias_grad)
         else
      } else
               hash_entry* sampler_entry =
            if (!sampler_entry) {
      d3d12_sampler_desc_table_key* sampler_table_key = MALLOC_STRUCT(d3d12_sampler_desc_table_key);
   sampler_table_key->count = view.count;
            d3d12_descriptor_handle* sampler_table_data = MALLOC_STRUCT(d3d12_descriptor_handle);
                                 } else
         }
      static D3D12_UAV_DIMENSION
   image_view_dimension(enum pipe_texture_target target)
   {
      switch (target) {
   case PIPE_BUFFER: return D3D12_UAV_DIMENSION_BUFFER;
   case PIPE_TEXTURE_1D: return D3D12_UAV_DIMENSION_TEXTURE1D;
   case PIPE_TEXTURE_1D_ARRAY: return D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
   case PIPE_TEXTURE_RECT:
   case PIPE_TEXTURE_2D:
         case PIPE_TEXTURE_2D_ARRAY:
   case PIPE_TEXTURE_CUBE:
   case PIPE_TEXTURE_CUBE_ARRAY:
         case PIPE_TEXTURE_3D: return D3D12_UAV_DIMENSION_TEXTURE3D;
   default:
            }
      static D3D12_GPU_DESCRIPTOR_HANDLE
   fill_image_descriptors(struct d3d12_context *ctx,
               {
      struct d3d12_screen *screen = d3d12_screen(ctx->base.screen);
   struct d3d12_batch *batch = d3d12_current_batch(ctx);
                     for (unsigned i = 0; i < shader->nir->info.num_images; i++)
   {
               if (view->resource) {
      D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc;
   struct d3d12_resource *res = d3d12_resource(view->resource);
                  enum pipe_format view_format = ctx->image_view_emulation_formats[stage][i];
   if (view_format == PIPE_FORMAT_NONE)
                        unsigned array_size = view->u.tex.last_layer - view->u.tex.first_layer + 1;
   switch (uav_desc.ViewDimension) {
   case D3D12_UAV_DIMENSION_TEXTURE1D:
      if (view->u.tex.first_layer > 0)
      debug_printf("D3D12: can't create 1D UAV from layer %d\n",
      uav_desc.Texture1D.MipSlice = view->u.tex.level;
      case D3D12_UAV_DIMENSION_TEXTURE1DARRAY:
      uav_desc.Texture1DArray.FirstArraySlice = view->u.tex.first_layer;
   uav_desc.Texture1DArray.ArraySize = array_size;
   uav_desc.Texture1DArray.MipSlice = view->u.tex.level;
      case D3D12_UAV_DIMENSION_TEXTURE2D:
      if (view->u.tex.first_layer > 0)
      debug_printf("D3D12: can't create 2D UAV from layer %d\n",
      uav_desc.Texture2D.MipSlice = view->u.tex.level;
   uav_desc.Texture2D.PlaneSlice = 0;
      case D3D12_UAV_DIMENSION_TEXTURE2DARRAY:
      uav_desc.Texture2DArray.FirstArraySlice = view->u.tex.first_layer;
   uav_desc.Texture2DArray.ArraySize = array_size;
   uav_desc.Texture2DArray.MipSlice = view->u.tex.level;
   uav_desc.Texture2DArray.PlaneSlice = 0;
      case D3D12_UAV_DIMENSION_TEXTURE3D:
      uav_desc.Texture3D.MipSlice = view->u.tex.level;
   uav_desc.Texture3D.FirstWSlice = view->u.tex.first_layer;
   uav_desc.Texture3D.WSize = array_size;
      case D3D12_UAV_DIMENSION_BUFFER: {
      uint format_size = util_format_get_blocksize(view_format);
   offset += view->u.buf.offset;
   uav_desc.Buffer.CounterOffsetInBytes = 0;
   uav_desc.Buffer.FirstElement = offset / format_size;
   uav_desc.Buffer.NumElements = MIN2(view->u.buf.size / format_size,
         uav_desc.Buffer.StructureByteStride = 0;
   uav_desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
      }
   default:
         }
      if (!batch->pending_memory_barrier) {
      if (res->base.b.target == PIPE_BUFFER) {
         } else {
      unsigned transition_first_layer = view->u.tex.first_layer;
   unsigned transition_array_size = array_size;
   if (res->base.b.target == PIPE_TEXTURE_3D) {
      transition_first_layer = 0;
      }
   d3d12_transition_subresources_state(ctx, res,
                                                struct d3d12_descriptor_handle handle;
   d3d12_descriptor_heap_alloc_handle(batch->view_heap, &handle);
      } else {
                        }
      static unsigned
   fill_graphics_state_vars(struct d3d12_context *ctx,
                           const struct pipe_draw_info *dinfo,
   unsigned drawid,
   {
               for (unsigned j = 0; j < shader->num_state_vars; ++j) {
               switch (shader->state_vars[j].var) {
   case D3D12_STATE_VAR_Y_FLIP:
      ptr[0] = fui(ctx->flip_y);
   size += 4;
      case D3D12_STATE_VAR_PT_SPRITE:
      ptr[0] = fui(1.0 / ctx->viewports[0].Width);
   ptr[1] = fui(1.0 / ctx->viewports[0].Height);
   ptr[2] = fui(ctx->gfx_pipeline_state.rast->base.point_size);
   ptr[3] = fui(D3D12_MAX_POINT_SIZE);
   size += 4;
      case D3D12_STATE_VAR_DRAW_PARAMS:
      ptr[0] = dinfo->index_size ? draw->index_bias : draw->start;
   ptr[1] = dinfo->start_instance;
   ptr[2] = drawid;
   ptr[3] = dinfo->index_size ? -1 : 0;
   cmd_sig_key->draw_or_dispatch_params = 1;
   cmd_sig_key->root_sig = ctx->gfx_pipeline_state.root_signature;
   cmd_sig_key->params_root_const_offset = size;
   size += 4;
      case D3D12_STATE_VAR_DEPTH_TRANSFORM:
      ptr[0] = fui(2.0f * ctx->viewport_states[0].scale[2]);
   ptr[1] = fui(ctx->viewport_states[0].translate[2] - ctx->viewport_states[0].scale[2]);
   size += 4;
      case D3D12_STATE_VAR_DEFAULT_INNER_TESS_LEVEL:
      memcpy(ptr, ctx->default_inner_tess_factor, sizeof(ctx->default_inner_tess_factor));
   size += 4;
      case D3D12_STATE_VAR_DEFAULT_OUTER_TESS_LEVEL:
      memcpy(ptr, ctx->default_outer_tess_factor, sizeof(ctx->default_outer_tess_factor));
   size += 4;
      case D3D12_STATE_VAR_PATCH_VERTICES_IN:
      ptr[0] = ctx->patch_vertices;
   size += 4;
      default:
                        }
      static unsigned
   fill_compute_state_vars(struct d3d12_context *ctx,
                           {
               for (unsigned j = 0; j < shader->num_state_vars; ++j) {
               switch (shader->state_vars[j].var) {
   case D3D12_STATE_VAR_NUM_WORKGROUPS:
      ptr[0] = info->grid[0];
   ptr[1] = info->grid[1];
   ptr[2] = info->grid[2];
   cmd_sig_key->draw_or_dispatch_params = 1;
   cmd_sig_key->root_sig = ctx->compute_pipeline_state.root_signature;
   cmd_sig_key->params_root_const_offset = size;
   size += 4;
      case D3D12_STATE_VAR_TRANSFORM_GENERIC0: {
      unsigned idx = shader->state_vars[j].var - D3D12_STATE_VAR_TRANSFORM_GENERIC0;
   ptr[0] = ctx->transform_state_vars[idx * 4];
   ptr[1] = ctx->transform_state_vars[idx * 4 + 1];
   ptr[2] = ctx->transform_state_vars[idx * 4 + 2];
   ptr[3] = ctx->transform_state_vars[idx * 4 + 3];
   size += 4;
      }
   default:
                        }
      static bool
   check_descriptors_left(struct d3d12_context *ctx, bool compute)
   {
      struct d3d12_batch *batch = d3d12_current_batch(ctx);
            unsigned count = compute ? 1 : D3D12_GFX_SHADER_STAGES;
   for (unsigned i = 0; i < count; ++i) {
               if (!shader)
            needed_descs += shader->current->num_cb_bindings;
   needed_descs += shader->current->end_srv_binding - shader->current->begin_srv_binding;
   needed_descs += shader->current->nir->info.num_ssbos;
               if (d3d12_descriptor_heap_get_remaining_handles(batch->view_heap) < needed_descs)
            needed_descs = 0;
   for (unsigned i = 0; i < count; ++i) {
               if (!shader)
                        if (d3d12_descriptor_heap_get_remaining_handles(batch->sampler_heap) < needed_descs)
               }
      #define MAX_DESCRIPTOR_TABLES (D3D12_GFX_SHADER_STAGES * 4)
      static void
   update_shader_stage_root_parameters(struct d3d12_context *ctx,
                                 {
      auto stage = shader_sel->stage;
   struct d3d12_shader *shader = shader_sel->current;
   uint64_t dirty = ctx->shader_dirty[stage];
            if (shader->num_cb_bindings > 0) {
      if (dirty & D3D12_SHADER_DIRTY_CONSTBUF) {
      assert(num_root_descriptors < MAX_DESCRIPTOR_TABLES);
   root_desc_tables[num_root_descriptors] = fill_cbv_descriptors(ctx, shader, stage);
      }
      }
   if (shader->end_srv_binding > 0) {
      if (dirty & D3D12_SHADER_DIRTY_SAMPLER_VIEWS) {
      assert(num_root_descriptors < MAX_DESCRIPTOR_TABLES);
   root_desc_tables[num_root_descriptors] = fill_srv_descriptors(ctx, shader, stage);
      }
   num_params++;
   if (dirty & D3D12_SHADER_DIRTY_SAMPLERS) {
      assert(num_root_descriptors < MAX_DESCRIPTOR_TABLES);
   root_desc_tables[num_root_descriptors] = fill_sampler_descriptors(ctx, shader_sel, stage);
      }
      }
   if (shader->nir->info.num_ssbos > 0) {
      if (dirty & D3D12_SHADER_DIRTY_SSBO) {
      assert(num_root_descriptors < MAX_DESCRIPTOR_TABLES);
   root_desc_tables[num_root_descriptors] = fill_ssbo_descriptors(ctx, shader, stage);
      }
      }
   if (shader->nir->info.num_images > 0) {
      if (dirty & D3D12_SHADER_DIRTY_IMAGE) {
      assert(num_root_descriptors < MAX_DESCRIPTOR_TABLES);
   root_desc_tables[num_root_descriptors] = fill_image_descriptors(ctx, shader, stage);
      }
         }
      static unsigned
   update_graphics_root_parameters(struct d3d12_context *ctx,
                                 const struct pipe_draw_info *dinfo,
   {
      unsigned num_params = 0;
            for (unsigned i = 0; i < D3D12_GFX_SHADER_STAGES; ++i) {
      struct d3d12_shader_selector *shader_sel = ctx->gfx_stages[i];
   if (!shader_sel)
            update_shader_stage_root_parameters(ctx, shader_sel, num_params, num_root_descriptors, root_desc_tables, root_desc_indices);
   /* TODO Don't always update state vars */
   if (shader_sel->current->num_state_vars > 0) {
      uint32_t constants[D3D12_MAX_GRAPHICS_STATE_VARS * 4];
   unsigned size = fill_graphics_state_vars(ctx, dinfo, drawid, draw, shader_sel->current, constants, cmd_sig_key);
   if (cmd_sig_key->draw_or_dispatch_params)
         ctx->cmdlist->SetGraphicsRoot32BitConstants(num_params, size, constants, 0);
         }
      }
      static unsigned
   update_compute_root_parameters(struct d3d12_context *ctx,
                           {
      unsigned num_params = 0;
            struct d3d12_shader_selector *shader_sel = ctx->compute_state;
   if (shader_sel) {
      update_shader_stage_root_parameters(ctx, shader_sel, num_params, num_root_descriptors, root_desc_tables, root_desc_indices);
   /* TODO Don't always update state vars */
   if (shader_sel->current->num_state_vars > 0) {
      uint32_t constants[D3D12_MAX_COMPUTE_STATE_VARS * 4];
   unsigned size = fill_compute_state_vars(ctx, info, shader_sel->current, constants, cmd_sig_key);
   if (cmd_sig_key->draw_or_dispatch_params)
         ctx->cmdlist->SetComputeRoot32BitConstants(num_params, size, constants, 0);
         }
      }
      static bool
   validate_stream_output_targets(struct d3d12_context *ctx)
   {
               if (ctx->gfx_pipeline_state.num_so_targets &&
      ctx->gfx_pipeline_state.stages[PIPE_SHADER_GEOMETRY])
         if (factor > 1)
         else
      }
      static D3D_PRIMITIVE_TOPOLOGY
   topology(enum mesa_prim prim_type, uint8_t patch_vertices)
   {
      switch (prim_type) {
   case MESA_PRIM_POINTS:
            case MESA_PRIM_LINES:
            case MESA_PRIM_LINE_STRIP:
            case MESA_PRIM_TRIANGLES:
            case MESA_PRIM_TRIANGLE_STRIP:
            case MESA_PRIM_LINES_ADJACENCY:
            case MESA_PRIM_LINE_STRIP_ADJACENCY:
            case MESA_PRIM_TRIANGLES_ADJACENCY:
            case MESA_PRIM_TRIANGLE_STRIP_ADJACENCY:
            case MESA_PRIM_PATCHES:
            case MESA_PRIM_QUADS:
   case MESA_PRIM_QUAD_STRIP:
            default:
      debug_printf("mesa_prim: %s\n", u_prim_name(prim_type));
         }
      static DXGI_FORMAT
   ib_format(unsigned index_size)
   {
      switch (index_size) {
   case 1: return DXGI_FORMAT_R8_UINT;
   case 2: return DXGI_FORMAT_R16_UINT;
            default:
            }
      static void
   twoface_emulation(struct d3d12_context *ctx,
                     struct d3d12_rasterizer_state *rast,
   {
      /* draw backfaces */
   ctx->base.bind_rasterizer_state(&ctx->base, rast->twoface_back);
            /* restore real state */
      }
      static void
   transition_surface_subresources_state(struct d3d12_context *ctx,
                     {
      struct d3d12_resource *res = d3d12_resource(pres);
   unsigned start_layer, num_layers;
   if (!d3d12_subresource_id_uses_layer(res->base.b.target)) {
      start_layer = 0;
      } else {
      start_layer = psurf->u.tex.first_layer;
      }
   d3d12_transition_subresources_state(ctx, res,
                                    }
      static bool
   prim_supported(enum mesa_prim prim_type)
   {
      switch (prim_type) {
   case MESA_PRIM_POINTS:
   case MESA_PRIM_LINES:
   case MESA_PRIM_LINE_STRIP:
   case MESA_PRIM_TRIANGLES:
   case MESA_PRIM_TRIANGLE_STRIP:
   case MESA_PRIM_LINES_ADJACENCY:
   case MESA_PRIM_LINE_STRIP_ADJACENCY:
   case MESA_PRIM_TRIANGLES_ADJACENCY:
   case MESA_PRIM_TRIANGLE_STRIP_ADJACENCY:
   case MESA_PRIM_PATCHES:
            default:
            }
      static inline struct d3d12_shader_selector *
   d3d12_last_vertex_stage(struct d3d12_context *ctx)
   {
      struct d3d12_shader_selector *sel = ctx->gfx_stages[PIPE_SHADER_GEOMETRY];
   if (!sel || sel->is_variant)
         if (!sel)
            }
      static bool
   update_draw_indirect_with_sysvals(struct d3d12_context *ctx,
      const struct pipe_draw_info *dinfo,
   unsigned drawid,
   const struct pipe_draw_indirect_info **indirect_inout,
      {
      if (*indirect_inout == nullptr ||
      ctx->gfx_stages[PIPE_SHADER_VERTEX] == nullptr)
         auto sys_values_read = ctx->gfx_stages[PIPE_SHADER_VERTEX]->initial->info.system_values_read;
   bool any =  BITSET_TEST(sys_values_read, SYSTEM_VALUE_VERTEX_ID_ZERO_BASE) ||
               BITSET_TEST(sys_values_read, SYSTEM_VALUE_BASE_VERTEX) ||
            if (!any)
            d3d12_compute_transform_save_restore save;
            auto indirect_in = *indirect_inout;
            d3d12_compute_transform_key key;
   memset(&key, 0, sizeof(key));
   key.type = d3d12_compute_transform_type::base_vertex;
   key.base_vertex.indexed = dinfo->index_size > 0;
   key.base_vertex.dynamic_count = indirect_in->indirect_draw_count != nullptr;
            ctx->transform_state_vars[0] = indirect_in->stride;
   ctx->transform_state_vars[1] = indirect_in->offset;
            if (indirect_in->indirect_draw_count) {
      pipe_constant_buffer draw_count_cbuf;
   draw_count_cbuf.buffer = indirect_in->indirect_draw_count;
   draw_count_cbuf.buffer_offset = indirect_in->indirect_draw_count_offset;
   draw_count_cbuf.buffer_size = 4;
   draw_count_cbuf.user_buffer = nullptr;
      }
      pipe_shader_buffer new_cs_ssbos[2];
   new_cs_ssbos[0].buffer = indirect_in->buffer;
   new_cs_ssbos[0].buffer_offset = 0;
            /* 4 additional uints for base vertex, base instance, draw ID, and a bool for indexed draw */
   unsigned out_stride = sizeof(uint32_t) * ((key.base_vertex.indexed ? 5 : 4) + 4);
   pipe_resource output_buf_templ = {};
   output_buf_templ.target = PIPE_BUFFER;
   output_buf_templ.width0 = out_stride * indirect_in->draw_count;
   output_buf_templ.height0 = output_buf_templ.depth0 = output_buf_templ.array_size =
                  new_cs_ssbos[1].buffer = ctx->base.screen->resource_create(ctx->base.screen, &output_buf_templ);
   new_cs_ssbos[1].buffer_offset = 0;
   new_cs_ssbos[1].buffer_size = output_buf_templ.width0;
            pipe_grid_info grid = {};
   grid.block[0] = grid.block[1] = grid.block[2] = 1;
   grid.grid[0] = indirect_in->draw_count;
   grid.grid[1] = grid.grid[2] = 1;
                     *indirect_out = *indirect_in;
   indirect_out->buffer = new_cs_ssbos[1].buffer;
   indirect_out->offset = 0;
   indirect_out->stride = out_stride;
      }
      static bool
   update_draw_auto(struct d3d12_context *ctx,
      const struct pipe_draw_indirect_info **indirect_inout,
      {
      if (*indirect_inout == nullptr ||
      (*indirect_inout)->count_from_stream_output == nullptr ||
   ctx->gfx_stages[PIPE_SHADER_VERTEX] == nullptr)
         d3d12_compute_transform_save_restore save;
            auto indirect_in = *indirect_inout;
            d3d12_compute_transform_key key;
   memset(&key, 0, sizeof(key));
   key.type = d3d12_compute_transform_type::draw_auto;
            auto so_arg = indirect_in->count_from_stream_output;
            ctx->transform_state_vars[0] = ctx->gfx_pipeline_state.ves->strides[0];
   ctx->transform_state_vars[1] = ctx->vbs[0].buffer_offset - so_arg->buffer_offset;
      pipe_shader_buffer new_cs_ssbo;
   new_cs_ssbo.buffer = target->fill_buffer;
   new_cs_ssbo.buffer_offset = target->fill_buffer_offset;
   new_cs_ssbo.buffer_size = target->fill_buffer->width0 - new_cs_ssbo.buffer_offset;
            pipe_grid_info grid = {};
   grid.block[0] = grid.block[1] = grid.block[2] = 1;
   grid.grid[0] = grid.grid[1] = grid.grid[2] = 1;
                     *indirect_out = *indirect_in;
   pipe_resource_reference(&indirect_out->buffer, target->fill_buffer);
   indirect_out->offset = target->fill_buffer_offset + 4;
   indirect_out->stride = sizeof(D3D12_DRAW_ARGUMENTS);
   indirect_out->count_from_stream_output = nullptr;
      }
      void
   d3d12_draw_vbo(struct pipe_context *pctx,
                  const struct pipe_draw_info *dinfo,
   unsigned drawid_offset,
      {
      if (num_draws > 1) {
      util_draw_multi(pctx, dinfo, drawid_offset, indirect, draws, num_draws);
               if (!indirect && (!draws[0].count || !dinfo->instance_count))
            struct d3d12_context *ctx = d3d12_context(pctx);
   struct d3d12_screen *screen = d3d12_screen(pctx->screen);
   struct d3d12_batch *batch;
   struct pipe_resource *index_buffer = NULL;
   unsigned index_offset = 0;
   enum d3d12_surface_conversion_mode conversion_modes[PIPE_MAX_COLOR_BUFS] = {};
            if (!prim_supported((enum mesa_prim)dinfo->mode) ||
      dinfo->index_size == 1 ||
   (dinfo->primitive_restart && dinfo->restart_index != 0xffff &&
            if (!dinfo->primitive_restart &&
      !indirect &&
               ctx->initial_api_prim = (enum mesa_prim)dinfo->mode;
   util_primconvert_save_rasterizer_state(ctx->primconvert, &ctx->gfx_pipeline_state.rast->base);
   util_primconvert_draw_vbo(ctx->primconvert, dinfo, drawid_offset, indirect, draws, num_draws);
               bool draw_auto = update_draw_auto(ctx, &indirect, &patched_indirect);
   bool indirect_with_sysvals = !draw_auto && update_draw_indirect_with_sysvals(ctx, dinfo, drawid_offset, &indirect, &patched_indirect);
   struct d3d12_cmd_signature_key cmd_sig_key;
            if (indirect) {
      cmd_sig_key.compute = false;
   cmd_sig_key.indexed = dinfo->index_size > 0;
   if (indirect->draw_count > 1 ||
      indirect->indirect_draw_count ||
   indirect_with_sysvals)
      else if (cmd_sig_key.indexed)
         else
               for (int i = 0; i < ctx->fb.nr_cbufs; ++i) {
      if (ctx->fb.cbufs[i]) {
      struct d3d12_surface *surface = d3d12_surface(ctx->fb.cbufs[i]);
   conversion_modes[i] = d3d12_surface_update_pre_draw(pctx, surface, d3d12_rtv_format(ctx, i));
   if (conversion_modes[i] != D3D12_SURFACE_CONVERSION_NONE)
                  struct d3d12_rasterizer_state *rast = ctx->gfx_pipeline_state.rast;
   if (rast->twoface_back) {
      enum mesa_prim saved_mode = ctx->initial_api_prim;
   twoface_emulation(ctx, rast, dinfo, indirect, &draws[0]);
               if (ctx->pstipple.enabled && ctx->gfx_pipeline_state.rast->base.poly_stipple_enable)
      ctx->shader_dirty[PIPE_SHADER_FRAGMENT] |= D3D12_SHADER_DIRTY_SAMPLER_VIEWS |
         /* this should *really* be fixed at a higher level than here! */
   enum mesa_prim reduced_prim = u_reduced_prim((enum mesa_prim)dinfo->mode);
   if (reduced_prim == MESA_PRIM_TRIANGLES &&
      ctx->gfx_pipeline_state.rast->base.cull_face == PIPE_FACE_FRONT_AND_BACK)
         if (ctx->gfx_pipeline_state.prim_type != dinfo->mode) {
      ctx->gfx_pipeline_state.prim_type = (enum mesa_prim)dinfo->mode;
               d3d12_select_shader_variants(ctx, dinfo);
   d3d12_validate_queries(ctx);
   for (unsigned i = 0; i < D3D12_GFX_SHADER_STAGES; ++i) {
      struct d3d12_shader *shader = ctx->gfx_stages[i] ? ctx->gfx_stages[i]->current : NULL;
   if (ctx->gfx_pipeline_state.stages[i] != shader) {
      ctx->gfx_pipeline_state.stages[i] = shader;
                  /* Reset to an invalid value after it's been used */
            /* Copy the stream output info from the current vertex/geometry shader */
   if (ctx->state_dirty & D3D12_DIRTY_SHADER) {
      struct d3d12_shader_selector *sel = d3d12_last_vertex_stage(ctx);
   if (sel) {
         } else {
            }
   if (!validate_stream_output_targets(ctx)) {
      debug_printf("validate_stream_output_targets() failed\n");
               D3D12_INDEX_BUFFER_STRIP_CUT_VALUE ib_strip_cut_value =
         if (dinfo->index_size > 0) {
               if (dinfo->has_user_indices) {
      if (!util_upload_index_buffer(pctx, dinfo, &draws[0], &index_buffer,
      &index_offset, 4)) {
   debug_printf("util_upload_index_buffer() failed\n");
         } else {
                  if (dinfo->primitive_restart) {
      assert(dinfo->restart_index == 0xffff ||
         ib_strip_cut_value = dinfo->restart_index == 0xffff ?
      D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_0xFFFF :
               if (ctx->gfx_pipeline_state.ib_strip_cut_value != ib_strip_cut_value) {
      ctx->gfx_pipeline_state.ib_strip_cut_value = ib_strip_cut_value;
               if (!ctx->gfx_pipeline_state.root_signature || ctx->state_dirty & D3D12_DIRTY_SHADER) {
      ID3D12RootSignature *root_signature = d3d12_get_root_signature(ctx, false);
   if (ctx->gfx_pipeline_state.root_signature != root_signature) {
      ctx->gfx_pipeline_state.root_signature = root_signature;
   ctx->state_dirty |= D3D12_DIRTY_ROOT_SIGNATURE;
   for (int i = 0; i < D3D12_GFX_SHADER_STAGES; ++i)
                  if (!ctx->current_gfx_pso || ctx->state_dirty & D3D12_DIRTY_GFX_PSO) {
      ctx->current_gfx_pso = d3d12_get_gfx_pipeline_state(ctx);
                        if (!check_descriptors_left(ctx, false))
                  if (ctx->cmdlist_dirty & D3D12_DIRTY_ROOT_SIGNATURE) {
      d3d12_batch_reference_object(batch, ctx->gfx_pipeline_state.root_signature);
               if (ctx->cmdlist_dirty & D3D12_DIRTY_GFX_PSO) {
      assert(ctx->current_gfx_pso);
   d3d12_batch_reference_object(batch, ctx->current_gfx_pso);
               D3D12_GPU_DESCRIPTOR_HANDLE root_desc_tables[MAX_DESCRIPTOR_TABLES];
   int root_desc_indices[MAX_DESCRIPTOR_TABLES];
   unsigned num_root_descriptors = update_graphics_root_parameters(ctx, dinfo, drawid_offset, &draws[0],
            bool need_zero_one_depth_range = d3d12_need_zero_one_depth_range(ctx);
   if (need_zero_one_depth_range != ctx->need_zero_one_depth_range) {
      ctx->cmdlist_dirty |= D3D12_DIRTY_VIEWPORT;
               if (ctx->cmdlist_dirty & D3D12_DIRTY_VIEWPORT) {
      D3D12_VIEWPORT viewports[PIPE_MAX_VIEWPORTS];
   for (unsigned i = 0; i < ctx->num_viewports; ++i) {
      viewports[i] = ctx->viewports[i];
   if (ctx->need_zero_one_depth_range) {
      viewports[i].MinDepth = 0.0f;
      }
   if (ctx->fb.nr_cbufs == 0 && !ctx->fb.zsbuf) {
      viewports[i].TopLeftX = MAX2(0.0f, viewports[i].TopLeftX);
   viewports[i].TopLeftY = MAX2(0.0f, viewports[i].TopLeftY);
   viewports[i].Width = MIN2(ctx->fb.width, viewports[i].Width);
         }
               if (ctx->cmdlist_dirty & D3D12_DIRTY_SCISSOR) {
      if (ctx->gfx_pipeline_state.rast->base.scissor && ctx->num_viewports > 0)
         else
               if (ctx->cmdlist_dirty & D3D12_DIRTY_BLEND_COLOR) {
      unsigned blend_factor_flags = ctx->gfx_pipeline_state.blend->blend_factor_flags;
   if (blend_factor_flags & (D3D12_BLEND_FACTOR_COLOR | D3D12_BLEND_FACTOR_ANY)) {
         } else if (blend_factor_flags & D3D12_BLEND_FACTOR_ALPHA) {
      float alpha_const[4] = { ctx->blend_factor[3], ctx->blend_factor[3],
                        if (ctx->cmdlist_dirty & D3D12_DIRTY_STENCIL_REF) {
      if (ctx->gfx_pipeline_state.zsa->backface_enabled &&
      screen->opts14.IndependentFrontAndBackStencilRefMaskSupported &&
   ctx->cmdlist8 != nullptr)
      else
               if (ctx->cmdlist_dirty & D3D12_DIRTY_PRIM_MODE)
            for (unsigned i = 0; i < ctx->num_vbs; ++i) {
      if (ctx->vbs[i].buffer.resource) {
      struct d3d12_resource *res = d3d12_resource(ctx->vbs[i].buffer.resource);
   d3d12_transition_resource_state(ctx, res, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_TRANSITION_FLAG_ACCUMULATE_STATE);
   if (ctx->cmdlist_dirty & D3D12_DIRTY_VERTEX_BUFFERS)
         }
   if (ctx->cmdlist_dirty & (D3D12_DIRTY_VERTEX_BUFFERS | D3D12_DIRTY_VERTEX_ELEMENTS)) {
      uint16_t *strides = ctx->gfx_pipeline_state.ves ? ctx->gfx_pipeline_state.ves->strides : NULL;
   if (strides) {
      for (unsigned i = 0; i < ctx->num_vbs; i++)
      } else {
      for (unsigned i = 0; i < ctx->num_vbs; i++)
      }
               if (index_buffer) {
      D3D12_INDEX_BUFFER_VIEW ibv;
   struct d3d12_resource *res = d3d12_resource(index_buffer);
   ibv.BufferLocation = d3d12_resource_gpu_virtual_address(res) + index_offset;
   ibv.SizeInBytes = res->base.b.width0 - index_offset;
   ibv.Format = ib_format(dinfo->index_size);
   d3d12_transition_resource_state(ctx, res, D3D12_RESOURCE_STATE_INDEX_BUFFER, D3D12_TRANSITION_FLAG_ACCUMULATE_STATE);
   if (ctx->cmdlist_dirty & D3D12_DIRTY_INDEX_BUFFER ||
      memcmp(&ctx->ibv, &ibv, sizeof(D3D12_INDEX_BUFFER_VIEW)) != 0) {
   ctx->ibv = ibv;
   d3d12_batch_reference_resource(batch, res, false);
               if (dinfo->has_user_indices)
               if (ctx->cmdlist_dirty & D3D12_DIRTY_FRAMEBUFFER) {
      D3D12_CPU_DESCRIPTOR_HANDLE render_targets[PIPE_MAX_COLOR_BUFS] = {};
   D3D12_CPU_DESCRIPTOR_HANDLE *depth_desc = NULL, tmp_desc;
   for (int i = 0; i < ctx->fb.nr_cbufs; ++i) {
      if (ctx->fb.cbufs[i]) {
      struct d3d12_surface *surface = d3d12_surface(ctx->fb.cbufs[i]);
   render_targets[i] = d3d12_surface_get_handle(surface, conversion_modes[i]);
      } else
      }
   if (ctx->fb.zsbuf) {
      struct d3d12_surface *surface = d3d12_surface(ctx->fb.zsbuf);
   tmp_desc = surface->desc_handle.cpu_handle;
   d3d12_batch_reference_surface_texture(batch, surface);
      }
               struct pipe_stream_output_target **so_targets = ctx->fake_so_buffer_factor ? ctx->fake_so_targets
         D3D12_STREAM_OUTPUT_BUFFER_VIEW *so_buffer_views = ctx->fake_so_buffer_factor ? ctx->fake_so_buffer_views
         for (unsigned i = 0; i < ctx->gfx_pipeline_state.num_so_targets; ++i) {
               if (!target)
            struct d3d12_resource *so_buffer = d3d12_resource(target->base.buffer);
            if (ctx->cmdlist_dirty & D3D12_DIRTY_STREAM_OUTPUT) {
      d3d12_batch_reference_resource(batch, so_buffer, true);
               d3d12_transition_resource_state(ctx, so_buffer, D3D12_RESOURCE_STATE_STREAM_OUT, D3D12_TRANSITION_FLAG_ACCUMULATE_STATE);
      }
   if (ctx->cmdlist_dirty & D3D12_DIRTY_STREAM_OUTPUT)
            for (int i = 0; i < ctx->fb.nr_cbufs; ++i) {
      struct pipe_surface *psurf = ctx->fb.cbufs[i];
   if (!psurf)
            struct pipe_resource *pres = conversion_modes[i] == D3D12_SURFACE_CONVERSION_BGRA_UINT ?
         transition_surface_subresources_state(ctx, psurf, pres,
      }
   if (ctx->fb.zsbuf) {
      struct pipe_surface *psurf = ctx->fb.zsbuf;
   transition_surface_subresources_state(ctx, psurf, psurf->texture,
               ID3D12Resource *indirect_arg_buf = nullptr;
   ID3D12Resource *indirect_count_buf = nullptr;
   uint64_t indirect_arg_offset = 0, indirect_count_offset = 0;
   if (indirect) {
      if (indirect->buffer) {
      struct d3d12_resource *indirect_buf = d3d12_resource(indirect->buffer);
   uint64_t buf_offset = 0;
   indirect_arg_buf = d3d12_resource_underlying(indirect_buf, &buf_offset);
   indirect_arg_offset = indirect->offset + buf_offset;
   d3d12_transition_resource_state(ctx, indirect_buf,
            }
   if (indirect->indirect_draw_count) {
      struct d3d12_resource *count_buf = d3d12_resource(indirect->indirect_draw_count);
   uint64_t count_offset = 0;
   indirect_count_buf = d3d12_resource_underlying(count_buf, &count_offset);
   indirect_count_offset = indirect->indirect_draw_count_offset + count_offset;
   d3d12_transition_resource_state(ctx, count_buf,
            }
                        for (unsigned i = 0; i < num_root_descriptors; ++i)
            if (indirect) {
      unsigned draw_count = draw_auto ? 1 : indirect->draw_count;
   ID3D12CommandSignature *cmd_sig = d3d12_get_cmd_signature(ctx, &cmd_sig_key);
   ctx->cmdlist->ExecuteIndirect(cmd_sig, draw_count, indirect_arg_buf,
      } else {
      if (dinfo->index_size > 0)
      ctx->cmdlist->DrawIndexedInstanced(draws[0].count, dinfo->instance_count,
            else
      ctx->cmdlist->DrawInstanced(draws[0].count, dinfo->instance_count,
            ctx->state_dirty &= D3D12_DIRTY_COMPUTE_MASK;
            ctx->cmdlist_dirty &= D3D12_DIRTY_COMPUTE_MASK |
            /* The next dispatch needs to reassert the compute PSO */
            for (unsigned i = 0; i < D3D12_GFX_SHADER_STAGES; ++i)
            for (int i = 0; i < ctx->fb.nr_cbufs; ++i) {
      if (ctx->fb.cbufs[i]) {
      struct d3d12_surface *surface = d3d12_surface(ctx->fb.cbufs[i]);
                     }
      static bool
   update_dispatch_indirect_with_sysvals(struct d3d12_context *ctx,
                     {
      if (*indirect_inout == nullptr ||
      ctx->compute_state == nullptr)
         if (!BITSET_TEST(ctx->compute_state->current->nir->info.system_values_read, SYSTEM_VALUE_NUM_WORKGROUPS))
            if (ctx->current_predication)
                     /* 6 uints: 2 copies of the indirect arg buffer */
   pipe_resource output_buf_templ = {};
   output_buf_templ.target = PIPE_BUFFER;
   output_buf_templ.width0 = sizeof(uint32_t) * 6;
   output_buf_templ.height0 = output_buf_templ.depth0 = output_buf_templ.array_size =
         output_buf_templ.usage = PIPE_USAGE_DEFAULT;
            struct pipe_box src_box = { (int)*indirect_offset_inout, 0, 0, sizeof(uint32_t) * 3, 1, 1 };
   ctx->base.resource_copy_region(&ctx->base, *indirect_out, 0, 0, 0, 0, indirect_in, 0, &src_box);
            if (ctx->current_predication)
            *indirect_inout = *indirect_out;
   *indirect_offset_inout = 0;
      }
      void
   d3d12_launch_grid(struct pipe_context *pctx, const struct pipe_grid_info *info)
   {
      struct d3d12_context *ctx = d3d12_context(pctx);
   struct d3d12_batch *batch;
            struct d3d12_cmd_signature_key cmd_sig_key;
   memset(&cmd_sig_key, 0, sizeof(cmd_sig_key));
   cmd_sig_key.compute = 1;
            struct pipe_resource *indirect = info->indirect;
   unsigned indirect_offset = info->indirect_offset;
   if (indirect && update_dispatch_indirect_with_sysvals(ctx, &indirect, &indirect_offset, &patched_indirect))
            d3d12_select_compute_shader_variants(ctx, info);
   d3d12_validate_queries(ctx);
   struct d3d12_shader *shader = ctx->compute_state ? ctx->compute_state->current : NULL;
   if (ctx->compute_pipeline_state.stage != shader) {
      ctx->compute_pipeline_state.stage = shader;
               if (!ctx->compute_pipeline_state.root_signature || ctx->state_dirty & D3D12_DIRTY_COMPUTE_SHADER) {
      ID3D12RootSignature *root_signature = d3d12_get_root_signature(ctx, true);
   if (ctx->compute_pipeline_state.root_signature != root_signature) {
      ctx->compute_pipeline_state.root_signature = root_signature;
   ctx->state_dirty |= D3D12_DIRTY_COMPUTE_ROOT_SIGNATURE;
                  if (!ctx->current_compute_pso || ctx->state_dirty & D3D12_DIRTY_COMPUTE_PSO) {
      ctx->current_compute_pso = d3d12_get_compute_pipeline_state(ctx);
                        if (!check_descriptors_left(ctx, true))
                  if (ctx->cmdlist_dirty & D3D12_DIRTY_COMPUTE_ROOT_SIGNATURE) {
      d3d12_batch_reference_object(batch, ctx->compute_pipeline_state.root_signature);
               if (ctx->cmdlist_dirty & D3D12_DIRTY_COMPUTE_PSO) {
      assert(ctx->current_compute_pso);
   d3d12_batch_reference_object(batch, ctx->current_compute_pso);
               D3D12_GPU_DESCRIPTOR_HANDLE root_desc_tables[MAX_DESCRIPTOR_TABLES];
   int root_desc_indices[MAX_DESCRIPTOR_TABLES];
            ID3D12Resource *indirect_arg_buf = nullptr;
   uint64_t indirect_arg_offset = 0;
   if (indirect) {
      struct d3d12_resource *indirect_buf = d3d12_resource(indirect);
   uint64_t buf_offset = 0;
   indirect_arg_buf = d3d12_resource_underlying(indirect_buf, &buf_offset);
   indirect_arg_offset = indirect_offset + buf_offset;
   d3d12_transition_resource_state(ctx, indirect_buf,
                              for (unsigned i = 0; i < num_root_descriptors; ++i)
            if (indirect) {
      ID3D12CommandSignature *cmd_sig = d3d12_get_cmd_signature(ctx, &cmd_sig_key);
      } else {
                  ctx->state_dirty &= D3D12_DIRTY_GFX_MASK;
            /* The next draw needs to reassert the graphics PSO */
   ctx->cmdlist_dirty |= D3D12_DIRTY_SHADER;
            ctx->shader_dirty[PIPE_SHADER_COMPUTE] = 0;
      }
