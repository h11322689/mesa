   /*
   * Copyright © 2019 Red Hat.
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
      /* use a gallium context to execute a command buffer */
      #include "lvp_private.h"
      #include "pipe/p_context.h"
   #include "pipe/p_state.h"
   #include "lvp_conv.h"
      #include "pipe/p_shader_tokens.h"
   #include "tgsi/tgsi_from_mesa.h"
      #include "util/format/u_format.h"
   #include "util/u_surface.h"
   #include "util/u_sampler.h"
   #include "util/u_box.h"
   #include "util/u_inlines.h"
   #include "util/u_math.h"
   #include "util/u_memory.h"
   #include "util/u_prim.h"
   #include "util/u_prim_restart.h"
   #include "util/format/u_format_zs.h"
   #include "util/ptralloc.h"
   #include "tgsi/tgsi_from_mesa.h"
   #include "vulkan/util/vk_util.h"
      #include "vk_blend.h"
   #include "vk_cmd_enqueue_entrypoints.h"
   #include "vk_util.h"
      #define VK_PROTOTYPES
   #include <vulkan/vulkan.h>
      #define DOUBLE_EQ(a, b) (fabs((a) - (b)) < DBL_EPSILON)
      enum gs_output {
   GS_OUTPUT_NONE,
   GS_OUTPUT_NOT_LINES,
   GS_OUTPUT_LINES,
   };
      struct descriptor_buffer_offset {
      struct lvp_pipeline_layout *layout;
   uint32_t buffer_index;
               };
      struct lvp_render_attachment {
      struct lvp_image_view *imgv;
   VkResolveModeFlags resolve_mode;
   struct lvp_image_view *resolve_imgv;
   VkAttachmentLoadOp load_op;
   VkAttachmentStoreOp store_op;
   VkClearValue clear_value;
      };
      struct rendering_state {
      struct pipe_context *pctx;
   struct lvp_device *device; //for uniform inlining only
   struct u_upload_mgr *uploader;
            bool blend_dirty;
   bool rs_dirty;
   bool dsa_dirty;
   bool stencil_ref_dirty;
   bool clip_state_dirty;
   bool blend_color_dirty;
   bool ve_dirty;
   bool vb_dirty;
   bool constbuf_dirty[LVP_SHADER_STAGES];
   bool pcbuf_dirty[LVP_SHADER_STAGES];
   bool has_pcbuf[LVP_SHADER_STAGES];
   bool inlines_dirty[LVP_SHADER_STAGES];
   bool vp_dirty;
   bool scissor_dirty;
   bool ib_dirty;
   bool sample_mask_dirty;
   bool min_samples_dirty;
   bool poison_mem;
   bool noop_fs_bound;
   struct pipe_draw_indirect_info indirect_info;
            struct pipe_grid_info dispatch_info;
            struct pipe_blend_state blend_state;
   struct {
      float offset_units;
   float offset_scale;
   float offset_clamp;
      } depth_bias;
   struct pipe_rasterizer_state rs_state;
            struct pipe_blend_color blend_color;
   struct pipe_stencil_ref stencil_ref;
            int num_scissors;
            int num_viewports;
   struct pipe_viewport_state viewports[16];
   struct {
                  uint8_t patch_vertices;
   uint8_t index_size;
   unsigned index_offset;
   unsigned index_buffer_size; //UINT32_MAX for unset
   struct pipe_resource *index_buffer;
   struct pipe_constant_buffer const_buffer[LVP_SHADER_STAGES][16];
   struct lvp_descriptor_set *desc_sets[LVP_PIPELINE_TYPE_COUNT][MAX_SETS];
   struct pipe_resource *desc_buffers[MAX_SETS];
   uint8_t *desc_buffer_addrs[MAX_SETS];
   struct descriptor_buffer_offset desc_buffer_offsets[LVP_PIPELINE_TYPE_COUNT][MAX_SETS];
   int num_const_bufs[LVP_SHADER_STAGES];
   int num_vb;
   unsigned start_vb;
   bool vb_strides_dirty;
   unsigned vb_strides[PIPE_MAX_ATTRIBS];
   struct pipe_vertex_buffer vb[PIPE_MAX_ATTRIBS];
   size_t vb_sizes[PIPE_MAX_ATTRIBS]; //UINT32_MAX for unset
   uint8_t vertex_buffer_index[PIPE_MAX_ATTRIBS]; /* temp storage to sort for start_vb */
            bool disable_multisample;
            uint32_t color_write_disables:8;
                     uint8_t push_constants[128 * 4];
   uint16_t push_size[LVP_PIPELINE_TYPE_COUNT];
            VkRect2D render_area;
   bool suspending;
   bool render_cond;
   uint32_t color_att_count;
   struct lvp_render_attachment color_att[PIPE_MAX_COLOR_BUFS];
   struct lvp_render_attachment depth_att;
   struct lvp_render_attachment stencil_att;
   struct lvp_image_view *ds_imgv;
   struct lvp_image_view *ds_resolve_imgv;
   uint32_t                                     forced_sample_count;
   VkResolveModeFlagBits                        forced_depth_resolve_mode;
            uint32_t sample_mask;
   unsigned min_samples;
   unsigned rast_samples;
   float min_sample_shading;
   bool force_min_sample;
   bool sample_shading;
            uint32_t num_so_targets;
   struct pipe_stream_output_target *so_targets[PIPE_MAX_SO_BUFFERS];
                     bool tess_ccw;
                        };
      static struct pipe_resource *
   get_buffer_resource(struct pipe_context *ctx, void *mem)
   {
      struct pipe_screen *pscreen = ctx->screen;
            if (!mem)
            templ.screen = pscreen;
   templ.target = PIPE_BUFFER;
   templ.format = PIPE_FORMAT_R8_UNORM;
   templ.width0 = UINT32_MAX;
   templ.height0 = 1;
   templ.depth0 = 1;
   templ.array_size = 1;
   templ.bind |= PIPE_BIND_CONSTANT_BUFFER;
            uint64_t size;
   struct pipe_resource *pres = pscreen->resource_create_unbacked(pscreen, &templ, &size);
   pscreen->resource_bind_backing(pscreen, pres, mem, 0);
      }
      ALWAYS_INLINE static void
   assert_subresource_layers(const struct pipe_resource *pres,
               {
   #ifndef NDEBUG
      if (pres->target == PIPE_TEXTURE_3D) {
      assert(layers->baseArrayLayer == 0);
   assert(layers->layerCount == 1);
   assert(offsets[0].z <= pres->depth0);
      } else {
      assert(layers->baseArrayLayer < pres->array_size);
   assert(layers->baseArrayLayer + vk_image_subresource_layer_count(&image->vk, layers) <= pres->array_size);
   assert(offsets[0].z == 0);
         #endif
   }
      static void finish_fence(struct rendering_state *state)
   {
                        state->pctx->screen->fence_finish(state->pctx->screen,
               state->pctx->screen->fence_reference(state->pctx->screen,
      }
      static unsigned
   get_pcbuf_size(struct rendering_state *state, enum pipe_shader_type pstage)
   {
      bool is_compute = pstage == MESA_SHADER_COMPUTE;
      }
      static void
   fill_ubo0(struct rendering_state *state, uint8_t *mem, enum pipe_shader_type pstage)
   {
      unsigned push_size = get_pcbuf_size(state, pstage);
   if (push_size)
      }
      static void
   update_pcbuf(struct rendering_state *state, enum pipe_shader_type pstage)
   {
      unsigned size = get_pcbuf_size(state, pstage);
   if (size) {
      uint8_t *mem;
   struct pipe_constant_buffer cbuf;
   cbuf.buffer_size = size;
   cbuf.buffer = NULL;
   cbuf.user_buffer = NULL;
   u_upload_alloc(state->uploader, 0, size, 64, &cbuf.buffer_offset, &cbuf.buffer, (void**)&mem);
   fill_ubo0(state, mem, pstage);
      }
      }
      static void
   update_inline_shader_state(struct rendering_state *state, enum pipe_shader_type sh, bool pcbuf_dirty)
   {
      unsigned stage = tgsi_processor_to_shader_stage(sh);
   state->inlines_dirty[sh] = false;
   struct lvp_shader *shader = state->shaders[stage];
   if (!shader || !shader->inlines.can_inline)
         struct lvp_inline_variant v;
   v.mask = shader->inlines.can_inline;
   /* these buffers have already been flushed in llvmpipe, so they're safe to read */
   nir_shader *base_nir = shader->pipeline_nir->nir;
   if (stage == MESA_SHADER_TESS_EVAL && state->tess_ccw)
         nir_function_impl *impl = nir_shader_get_entrypoint(base_nir);
   unsigned ssa_alloc = impl->ssa_alloc;
   unsigned count = shader->inlines.count[0];
   if (count && pcbuf_dirty) {
      unsigned push_size = get_pcbuf_size(state, sh);
   for (unsigned i = 0; i < count; i++) {
      unsigned offset = shader->inlines.uniform_offsets[0][i];
   if (offset < push_size) {
            }
   for (unsigned i = count; i < MAX_INLINABLE_UNIFORMS; i++)
      }
   bool found = false;
   struct set_entry *entry = _mesa_set_search_or_add_pre_hashed(&shader->inlines.variants, v.mask, &v, &found);
   void *shader_state;
   if (found) {
      const struct lvp_inline_variant *variant = entry->key;
      } else {
      nir_shader *nir = nir_shader_clone(NULL, base_nir);
   NIR_PASS_V(nir, lvp_inline_uniforms, shader, v.vals[0], 0);
   lvp_shader_optimize(nir);
   impl = nir_shader_get_entrypoint(nir);
   if (ssa_alloc - impl->ssa_alloc < ssa_alloc / 2 &&
      !shader->inlines.must_inline) {
   /* not enough change; don't inline further */
   shader->inlines.can_inline = 0;
   ralloc_free(nir);
   shader->shader_cso = lvp_shader_compile(state->device, shader, nir_shader_clone(NULL, shader->pipeline_nir->nir), true);
   _mesa_set_remove(&shader->inlines.variants, entry);
      } else {
      shader_state = lvp_shader_compile(state->device, shader, nir, true);
   struct lvp_inline_variant *variant = mem_dup(&v, sizeof(v));
   variant->cso = shader_state;
         }
   switch (sh) {
   case MESA_SHADER_VERTEX:
      state->pctx->bind_vs_state(state->pctx, shader_state);
      case MESA_SHADER_TESS_CTRL:
      state->pctx->bind_tcs_state(state->pctx, shader_state);
      case MESA_SHADER_TESS_EVAL:
      state->pctx->bind_tes_state(state->pctx, shader_state);
      case MESA_SHADER_GEOMETRY:
      state->pctx->bind_gs_state(state->pctx, shader_state);
      case MESA_SHADER_TASK:
      state->pctx->bind_ts_state(state->pctx, shader_state);
      case MESA_SHADER_MESH:
      state->pctx->bind_ms_state(state->pctx, shader_state);
      case MESA_SHADER_FRAGMENT:
      state->pctx->bind_fs_state(state->pctx, shader_state);
   state->noop_fs_bound = false;
      case MESA_SHADER_COMPUTE:
      state->pctx->bind_compute_state(state->pctx, shader_state);
      default: break;
      }
      static void emit_compute_state(struct rendering_state *state)
   {
      bool pcbuf_dirty = state->pcbuf_dirty[MESA_SHADER_COMPUTE];
   if (state->pcbuf_dirty[MESA_SHADER_COMPUTE])
            if (state->constbuf_dirty[MESA_SHADER_COMPUTE]) {
      for (unsigned i = 0; i < state->num_const_bufs[MESA_SHADER_COMPUTE]; i++)
      state->pctx->set_constant_buffer(state->pctx, MESA_SHADER_COMPUTE,
                  if (state->inlines_dirty[MESA_SHADER_COMPUTE])
      }
      static void
   update_min_samples(struct rendering_state *state)
   {
      state->min_samples = 1;
   if (state->sample_shading) {
      state->min_samples = ceil(state->rast_samples * state->min_sample_shading);
   if (state->min_samples > 1)
         if (state->min_samples < 1)
      }
   if (state->force_min_sample)
         if (state->rast_samples != state->framebuffer.samples) {
      state->framebuffer.samples = state->rast_samples;
         }
      static void update_vertex_elements_buffer_index(struct rendering_state *state)
   {
      for (int i = 0; i < state->velem.count; i++)
      }
      static void emit_state(struct rendering_state *state)
   {
      if (!state->shaders[MESA_SHADER_FRAGMENT] && !state->noop_fs_bound) {
      state->pctx->bind_fs_state(state->pctx, state->device->noop_fs);
      }
   if (state->blend_dirty) {
      uint32_t mask = 0;
   /* zero out the colormask values for disabled attachments */
   if (state->color_write_disables) {
      u_foreach_bit(att, state->color_write_disables) {
      mask |= state->blend_state.rt[att].colormask << (att * 4);
         }
   cso_set_blend(state->cso, &state->blend_state);
   /* reset colormasks using saved bitmask */
   if (state->color_write_disables) {
      const uint32_t att_mask = BITFIELD_MASK(4);
   u_foreach_bit(att, state->color_write_disables) {
            }
               if (state->rs_dirty) {
      bool ms = state->rs_state.multisample;
   if (state->disable_multisample &&
      (state->gs_output_lines == GS_OUTPUT_LINES ||
   (!state->shaders[MESA_SHADER_GEOMETRY] && u_reduced_prim(state->info.mode) == MESA_PRIM_LINES)))
      assert(offsetof(struct pipe_rasterizer_state, offset_clamp) - offsetof(struct pipe_rasterizer_state, offset_units) == sizeof(float) * 2);
   if (state->depth_bias.enabled) {
      state->rs_state.offset_units = state->depth_bias.offset_units;
   state->rs_state.offset_scale = state->depth_bias.offset_scale;
   state->rs_state.offset_clamp = state->depth_bias.offset_clamp;
   state->rs_state.offset_tri = true;
   state->rs_state.offset_line = true;
      } else {
      state->rs_state.offset_units = 0.0f;
   state->rs_state.offset_scale = 0.0f;
   state->rs_state.offset_clamp = 0.0f;
   state->rs_state.offset_tri = false;
   state->rs_state.offset_line = false;
      }
   cso_set_rasterizer(state->cso, &state->rs_state);
   state->rs_dirty = false;
               if (state->dsa_dirty) {
      cso_set_depth_stencil_alpha(state->cso, &state->dsa_state);
               if (state->sample_mask_dirty) {
      cso_set_sample_mask(state->cso, state->sample_mask);
               if (state->min_samples_dirty) {
      update_min_samples(state);
   cso_set_min_samples(state->cso, state->min_samples);
               if (state->blend_color_dirty) {
      state->pctx->set_blend_color(state->pctx, &state->blend_color);
               if (state->stencil_ref_dirty) {
      cso_set_stencil_ref(state->cso, state->stencil_ref);
               if (state->ve_dirty)
            if (state->vb_strides_dirty) {
      for (unsigned i = 0; i < state->velem.count; i++)
         state->ve_dirty = true;
               if (state->vb_dirty) {
      cso_set_vertex_buffers(state->cso, state->num_vb, 0, false, state->vb);
               if (state->ve_dirty) {
      cso_set_vertex_elements(state->cso, &state->velem);
                        lvp_forall_gfx_stage(sh) {
      if (state->constbuf_dirty[sh]) {
      for (unsigned idx = 0; idx < state->num_const_bufs[sh]; idx++)
      state->pctx->set_constant_buffer(state->pctx, sh,
   }
               lvp_forall_gfx_stage(sh) {
      pcbuf_dirty[sh] = state->pcbuf_dirty[sh];
   if (state->pcbuf_dirty[sh])
               lvp_forall_gfx_stage(sh) {
      if (state->inlines_dirty[sh])
               if (state->vp_dirty) {
      state->pctx->set_viewport_states(state->pctx, 0, state->num_viewports, state->viewports);
               if (state->scissor_dirty) {
      state->pctx->set_scissor_states(state->pctx, 0, state->num_scissors, state->scissors);
         }
      static void
   handle_compute_shader(struct rendering_state *state, struct lvp_shader *shader, struct lvp_pipeline_layout *layout)
   {
               if ((layout->push_constant_stages & VK_SHADER_STAGE_COMPUTE_BIT) > 0)
            if (!state->has_pcbuf[MESA_SHADER_COMPUTE])
            state->dispatch_info.block[0] = shader->pipeline_nir->nir->info.workgroup_size[0];
   state->dispatch_info.block[1] = shader->pipeline_nir->nir->info.workgroup_size[1];
   state->dispatch_info.block[2] = shader->pipeline_nir->nir->info.workgroup_size[2];
   state->inlines_dirty[MESA_SHADER_COMPUTE] = shader->inlines.can_inline;
   if (!shader->inlines.can_inline)
      }
      static void handle_compute_pipeline(struct vk_cmd_queue_entry *cmd,
         {
                  }
      static void
   set_viewport_depth_xform(struct rendering_state *state, unsigned idx)
   {
      double n = state->depth[idx].min;
            if (!state->rs_state.clip_halfz) {
      state->viewports[idx].scale[2] = 0.5 * (f - n);
      } else {
      state->viewports[idx].scale[2] = (f - n);
         }
      static void
   get_viewport_xform(struct rendering_state *state,
               {
      float x = viewport->x;
   float y = viewport->y;
   float half_width = 0.5f * viewport->width;
            state->viewports[idx].scale[0] = half_width;
   state->viewports[idx].translate[0] = half_width + x;
   state->viewports[idx].scale[1] = half_height;
               }
      static void
   update_samples(struct rendering_state *state, VkSampleCountFlags samples)
   {
      state->rast_samples = samples;
   state->rs_dirty |= state->rs_state.multisample != (samples > 1);
   state->rs_state.multisample = samples > 1;
      }
      static void
   handle_graphics_stages(struct rendering_state *state, VkShaderStageFlagBits shader_stages, bool dynamic_tess_origin)
   {
      u_foreach_bit(b, shader_stages) {
      VkShaderStageFlagBits vk_stage = (1 << b);
                     switch (vk_stage) {
   case VK_SHADER_STAGE_FRAGMENT_BIT:
      state->inlines_dirty[MESA_SHADER_FRAGMENT] = state->shaders[MESA_SHADER_FRAGMENT]->inlines.can_inline;
   if (!state->shaders[MESA_SHADER_FRAGMENT]->inlines.can_inline) {
      state->pctx->bind_fs_state(state->pctx, state->shaders[MESA_SHADER_FRAGMENT]->shader_cso);
      }
      case VK_SHADER_STAGE_VERTEX_BIT:
      state->inlines_dirty[MESA_SHADER_VERTEX] = state->shaders[MESA_SHADER_VERTEX]->inlines.can_inline;
   if (!state->shaders[MESA_SHADER_VERTEX]->inlines.can_inline)
            case VK_SHADER_STAGE_GEOMETRY_BIT:
      state->inlines_dirty[MESA_SHADER_GEOMETRY] = state->shaders[MESA_SHADER_GEOMETRY]->inlines.can_inline;
   if (!state->shaders[MESA_SHADER_GEOMETRY]->inlines.can_inline)
         state->gs_output_lines = state->shaders[MESA_SHADER_GEOMETRY]->pipeline_nir->nir->info.gs.output_primitive == MESA_PRIM_LINES ? GS_OUTPUT_LINES : GS_OUTPUT_NOT_LINES;
      case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT:
      state->inlines_dirty[MESA_SHADER_TESS_CTRL] = state->shaders[MESA_SHADER_TESS_CTRL]->inlines.can_inline;
   if (!state->shaders[MESA_SHADER_TESS_CTRL]->inlines.can_inline)
            case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT:
      state->inlines_dirty[MESA_SHADER_TESS_EVAL] = state->shaders[MESA_SHADER_TESS_EVAL]->inlines.can_inline;
   state->tess_states[0] = NULL;
   state->tess_states[1] = NULL;
   if (!state->shaders[MESA_SHADER_TESS_EVAL]->inlines.can_inline) {
      if (dynamic_tess_origin) {
      state->tess_states[0] = state->shaders[MESA_SHADER_TESS_EVAL]->shader_cso;
   state->tess_states[1] = state->shaders[MESA_SHADER_TESS_EVAL]->tess_ccw_cso;
      } else {
            }
   if (!dynamic_tess_origin)
            case VK_SHADER_STAGE_TASK_BIT_EXT:
      state->inlines_dirty[MESA_SHADER_TASK] = state->shaders[MESA_SHADER_TASK]->inlines.can_inline;
   state->dispatch_info.block[0] = state->shaders[MESA_SHADER_TASK]->pipeline_nir->nir->info.workgroup_size[0];
   state->dispatch_info.block[1] = state->shaders[MESA_SHADER_TASK]->pipeline_nir->nir->info.workgroup_size[1];
   state->dispatch_info.block[2] = state->shaders[MESA_SHADER_TASK]->pipeline_nir->nir->info.workgroup_size[2];
   if (!state->shaders[MESA_SHADER_TASK]->inlines.can_inline)
            case VK_SHADER_STAGE_MESH_BIT_EXT:
      state->inlines_dirty[MESA_SHADER_MESH] = state->shaders[MESA_SHADER_MESH]->inlines.can_inline;
   if (!(shader_stages & VK_SHADER_STAGE_TASK_BIT_EXT)) {
      state->dispatch_info.block[0] = state->shaders[MESA_SHADER_MESH]->pipeline_nir->nir->info.workgroup_size[0];
   state->dispatch_info.block[1] = state->shaders[MESA_SHADER_MESH]->pipeline_nir->nir->info.workgroup_size[1];
      }
   if (!state->shaders[MESA_SHADER_MESH]->inlines.can_inline)
            default:
      assert(0);
            }
      static void
   unbind_graphics_stages(struct rendering_state *state, VkShaderStageFlagBits shader_stages)
   {
      u_foreach_bit(vkstage, shader_stages) {
      gl_shader_stage stage = vk_to_mesa_shader_stage(1<<vkstage);
   state->has_pcbuf[stage] = false;
   switch (stage) {
   case MESA_SHADER_FRAGMENT:
      if (state->shaders[MESA_SHADER_FRAGMENT])
         state->noop_fs_bound = false;
      case MESA_SHADER_GEOMETRY:
      if (state->shaders[MESA_SHADER_GEOMETRY])
            case MESA_SHADER_TESS_CTRL:
      if (state->shaders[MESA_SHADER_TESS_CTRL])
            case MESA_SHADER_TESS_EVAL:
      if (state->shaders[MESA_SHADER_TESS_EVAL])
            case MESA_SHADER_VERTEX:
      if (state->shaders[MESA_SHADER_VERTEX])
            case MESA_SHADER_TASK:
      if (state->shaders[MESA_SHADER_TASK])
            case MESA_SHADER_MESH:
      if (state->shaders[MESA_SHADER_MESH])
            default:
         }
         }
      static void
   handle_graphics_layout(struct rendering_state *state, gl_shader_stage stage, struct lvp_pipeline_layout *layout)
   {
      if (layout->push_constant_stages & BITFIELD_BIT(stage)) {
      state->has_pcbuf[stage] = layout->push_constant_size > 0;
   if (!state->has_pcbuf[stage])
         }
      static void handle_graphics_pipeline(struct lvp_pipeline *pipeline,
         {
      const struct vk_graphics_pipeline_state *ps = &pipeline->graphics_state;
   lvp_pipeline_shaders_compile(pipeline, true);
   bool dynamic_tess_origin = BITSET_TEST(ps->dynamic, MESA_VK_DYNAMIC_TS_DOMAIN_ORIGIN);
   unbind_graphics_stages(state,
                           lvp_forall_gfx_stage(sh) {
      if (pipeline->graphics_state.shader_stages & mesa_to_vk_shader_stage(sh))
               handle_graphics_stages(state, pipeline->graphics_state.shader_stages, dynamic_tess_origin);
   lvp_forall_gfx_stage(sh) {
                  /* rasterization state */
   if (ps->rs) {
      if (!BITSET_TEST(ps->dynamic, MESA_VK_DYNAMIC_RS_DEPTH_CLAMP_ENABLE))
         if (BITSET_TEST(ps->dynamic, MESA_VK_DYNAMIC_RS_DEPTH_CLIP_ENABLE)) {
         } else {
      state->depth_clamp_sets_clip =
         if (state->depth_clamp_sets_clip)
         else
      state->rs_state.depth_clip_near = state->rs_state.depth_clip_far =
            if (!BITSET_TEST(ps->dynamic, MESA_VK_DYNAMIC_RS_RASTERIZER_DISCARD_ENABLE))
            if (!BITSET_TEST(ps->dynamic, MESA_VK_DYNAMIC_RS_LINE_MODE)) {
      state->rs_state.line_smooth = pipeline->line_smooth;
      }
   if (!BITSET_TEST(ps->dynamic, MESA_VK_DYNAMIC_RS_LINE_STIPPLE_ENABLE))
         if (!BITSET_TEST(ps->dynamic, MESA_VK_DYNAMIC_RS_POLYGON_MODE)) {
      state->rs_state.fill_front = vk_polygon_mode_to_pipe(ps->rs->polygon_mode);
      }
   if (!BITSET_TEST(ps->dynamic, MESA_VK_DYNAMIC_RS_PROVOKING_VERTEX)) {
      state->rs_state.flatshade_first =
               if (!BITSET_TEST(ps->dynamic, MESA_VK_DYNAMIC_RS_LINE_WIDTH))
         if (!BITSET_TEST(ps->dynamic, MESA_VK_DYNAMIC_RS_LINE_STIPPLE)) {
      state->rs_state.line_stipple_factor = ps->rs->line.stipple.factor - 1;
               if (!BITSET_TEST(ps->dynamic, MESA_VK_DYNAMIC_RS_DEPTH_BIAS_ENABLE))
         if (!BITSET_TEST(ps->dynamic, MESA_VK_DYNAMIC_RS_DEPTH_BIAS_FACTORS)) {
      state->depth_bias.offset_units = ps->rs->depth_bias.constant;
   state->depth_bias.offset_scale = ps->rs->depth_bias.slope;
               if (!BITSET_TEST(ps->dynamic, MESA_VK_DYNAMIC_RS_CULL_MODE))
            if (!BITSET_TEST(ps->dynamic, MESA_VK_DYNAMIC_RS_FRONT_FACE))
                     if (ps->ds) {
      if (!BITSET_TEST(ps->dynamic, MESA_VK_DYNAMIC_DS_DEPTH_TEST_ENABLE))
         if (!BITSET_TEST(ps->dynamic, MESA_VK_DYNAMIC_DS_DEPTH_WRITE_ENABLE))
         if (!BITSET_TEST(ps->dynamic, MESA_VK_DYNAMIC_DS_DEPTH_COMPARE_OP))
         if (!BITSET_TEST(ps->dynamic, MESA_VK_DYNAMIC_DS_DEPTH_BOUNDS_TEST_ENABLE))
            if (!BITSET_TEST(ps->dynamic, MESA_VK_DYNAMIC_DS_DEPTH_BOUNDS_TEST_BOUNDS)) {
      state->dsa_state.depth_bounds_min = ps->ds->depth.bounds_test.min;
               if (!BITSET_TEST(ps->dynamic, MESA_VK_DYNAMIC_DS_STENCIL_TEST_ENABLE)) {
      state->dsa_state.stencil[0].enabled = ps->ds->stencil.test_enable;
               const struct vk_stencil_test_face_state *front = &ps->ds->stencil.front;
            if (!BITSET_TEST(ps->dynamic, MESA_VK_DYNAMIC_DS_STENCIL_OP)) {
      state->dsa_state.stencil[0].func = front->op.compare;
   state->dsa_state.stencil[0].fail_op = vk_conv_stencil_op(front->op.fail);
                  state->dsa_state.stencil[1].func = back->op.compare;
   state->dsa_state.stencil[1].fail_op = vk_conv_stencil_op(back->op.fail);
   state->dsa_state.stencil[1].zpass_op = vk_conv_stencil_op(back->op.pass);
               if (!BITSET_TEST(ps->dynamic, MESA_VK_DYNAMIC_DS_STENCIL_COMPARE_MASK)) {
      state->dsa_state.stencil[0].valuemask = front->compare_mask;
               if (!BITSET_TEST(ps->dynamic, MESA_VK_DYNAMIC_DS_STENCIL_WRITE_MASK)) {
      state->dsa_state.stencil[0].writemask = front->write_mask;
               if (!BITSET_TEST(ps->dynamic, MESA_VK_DYNAMIC_DS_STENCIL_REFERENCE)) {
      state->stencil_ref.ref_value[0] = front->reference;
   state->stencil_ref.ref_value[1] = back->reference;
      }
               if (ps->cb) {
      if (!BITSET_TEST(ps->dynamic, MESA_VK_DYNAMIC_CB_LOGIC_OP_ENABLE))
         if (!BITSET_TEST(ps->dynamic, MESA_VK_DYNAMIC_CB_LOGIC_OP))
            if (!BITSET_TEST(ps->dynamic, MESA_VK_DYNAMIC_CB_COLOR_WRITE_ENABLES))
            for (unsigned i = 0; i < ps->cb->attachment_count; i++) {
      const struct vk_color_blend_attachment_state *att = &ps->cb->attachments[i];
   if (!BITSET_TEST(ps->dynamic, MESA_VK_DYNAMIC_CB_WRITE_MASKS))
                        if (!att->blend_enable) {
      state->blend_state.rt[i].rgb_func = 0;
   state->blend_state.rt[i].rgb_src_factor = 0;
   state->blend_state.rt[i].rgb_dst_factor = 0;
   state->blend_state.rt[i].alpha_func = 0;
   state->blend_state.rt[i].alpha_src_factor = 0;
      } else if (!BITSET_TEST(ps->dynamic, MESA_VK_DYNAMIC_CB_BLEND_EQUATIONS)) {
      state->blend_state.rt[i].rgb_func = vk_blend_op_to_pipe(att->color_blend_op);
   state->blend_state.rt[i].rgb_src_factor = vk_blend_factor_to_pipe(att->src_color_blend_factor);
   state->blend_state.rt[i].rgb_dst_factor = vk_blend_factor_to_pipe(att->dst_color_blend_factor);
   state->blend_state.rt[i].alpha_func = vk_blend_op_to_pipe(att->alpha_blend_op);
   state->blend_state.rt[i].alpha_src_factor = vk_blend_factor_to_pipe(att->src_alpha_blend_factor);
               /* At least llvmpipe applies the blend factor prior to the blend function,
   * regardless of what function is used. (like i965 hardware).
   * It means for MIN/MAX the blend factor has to be stomped to ONE.
   */
   if (att->color_blend_op == VK_BLEND_OP_MIN ||
      att->color_blend_op == VK_BLEND_OP_MAX) {
   state->blend_state.rt[i].rgb_src_factor = PIPE_BLENDFACTOR_ONE;
               if (att->alpha_blend_op == VK_BLEND_OP_MIN ||
      att->alpha_blend_op == VK_BLEND_OP_MAX) {
   state->blend_state.rt[i].alpha_src_factor = PIPE_BLENDFACTOR_ONE;
         }
   state->blend_dirty = true;
   if (!BITSET_TEST(ps->dynamic, MESA_VK_DYNAMIC_CB_BLEND_CONSTANTS)) {
      memcpy(state->blend_color.color, ps->cb->blend_constants, 4 * sizeof(float));
         } else if (ps->rp->color_attachment_count == 0) {
      memset(&state->blend_state, 0, sizeof(state->blend_state));
   state->blend_state.rt[0].colormask = 0xf;
               if (!BITSET_TEST(ps->dynamic, MESA_VK_DYNAMIC_RS_LINE_MODE))
         if (ps->ms) {
      if (!BITSET_TEST(ps->dynamic, MESA_VK_DYNAMIC_MS_SAMPLE_MASK)) {
      state->sample_mask = ps->ms->sample_mask;
      }
   if (!BITSET_TEST(ps->dynamic, MESA_VK_DYNAMIC_MS_ALPHA_TO_COVERAGE_ENABLE))
         if (!BITSET_TEST(ps->dynamic, MESA_VK_DYNAMIC_MS_ALPHA_TO_ONE_ENABLE))
         state->force_min_sample = pipeline->force_min_sample;
   state->sample_shading = ps->ms->sample_shading_enable;
   state->min_sample_shading = ps->ms->min_sample_shading;
   state->min_samples_dirty = true;
   state->blend_dirty = true;
   if (!BITSET_TEST(ps->dynamic, MESA_VK_DYNAMIC_MS_RASTERIZATION_SAMPLES))
      } else {
      if (!BITSET_TEST(ps->dynamic, MESA_VK_DYNAMIC_MS_SAMPLE_MASK) &&
      !BITSET_TEST(ps->dynamic, MESA_VK_DYNAMIC_MS_ALPHA_TO_ONE_ENABLE))
      state->sample_shading = false;
   state->force_min_sample = false;
   if (!BITSET_TEST(ps->dynamic, MESA_VK_DYNAMIC_MS_SAMPLE_MASK)) {
      state->sample_mask_dirty = state->sample_mask != 0xffffffff;
   state->sample_mask = 0xffffffff;
   state->min_samples_dirty = !!state->min_samples;
      }
   state->blend_dirty |= state->blend_state.alpha_to_coverage || state->blend_state.alpha_to_one;
   if (!BITSET_TEST(ps->dynamic, MESA_VK_DYNAMIC_MS_ALPHA_TO_COVERAGE_ENABLE))
         if (!BITSET_TEST(ps->dynamic, MESA_VK_DYNAMIC_MS_ALPHA_TO_ONE_ENABLE))
                     if (!BITSET_TEST(ps->dynamic, MESA_VK_DYNAMIC_VI) && ps->vi) {
      u_foreach_bit(a, ps->vi->attributes_valid) {
      uint32_t b = ps->vi->attributes[a].binding;
   state->velem.velems[a].src_offset = ps->vi->attributes[a].offset;
   state->vertex_buffer_index[a] = b;
   state->velem.velems[a].src_format =
                  uint32_t d = ps->vi->bindings[b].divisor;
   switch (ps->vi->bindings[b].input_rate) {
   case VK_VERTEX_INPUT_RATE_VERTEX:
      state->velem.velems[a].instance_divisor = 0;
      case VK_VERTEX_INPUT_RATE_INSTANCE:
      state->velem.velems[a].instance_divisor = d ? d : UINT32_MAX;
      default:
                  if (!BITSET_TEST(ps->dynamic, MESA_VK_DYNAMIC_VI_BINDING_STRIDES)) {
      state->vb_strides[b] = ps->vi->bindings[b].stride;
   state->vb_strides_dirty = true;
                  state->velem.count = util_last_bit(ps->vi->attributes_valid);
   state->vb_dirty = true;
               if (!BITSET_TEST(ps->dynamic, MESA_VK_DYNAMIC_IA_PRIMITIVE_TOPOLOGY) && ps->ia) {
      state->info.mode = vk_conv_topology(ps->ia->primitive_topology);
      }
   if (!BITSET_TEST(ps->dynamic, MESA_VK_DYNAMIC_IA_PRIMITIVE_RESTART_ENABLE) && ps->ia)
            if (ps->ts && !BITSET_TEST(ps->dynamic, MESA_VK_DYNAMIC_TS_PATCH_CONTROL_POINTS)) {
      if (state->patch_vertices != ps->ts->patch_control_points)
                     if (ps->vp) {
      if (!BITSET_TEST(ps->dynamic, MESA_VK_DYNAMIC_VP_VIEWPORT_COUNT)) {
      state->num_viewports = ps->vp->viewport_count;
      }
   if (!BITSET_TEST(ps->dynamic, MESA_VK_DYNAMIC_VP_SCISSOR_COUNT)) {
      state->num_scissors = ps->vp->scissor_count;
               if (!BITSET_TEST(ps->dynamic, MESA_VK_DYNAMIC_VP_VIEWPORTS)) {
      for (uint32_t i = 0; i < ps->vp->viewport_count; i++) {
      get_viewport_xform(state, &ps->vp->viewports[i], i);
      }
      }
   if (!BITSET_TEST(ps->dynamic, MESA_VK_DYNAMIC_VP_SCISSORS)) {
      for (uint32_t i = 0; i < ps->vp->scissor_count; i++) {
      const VkRect2D *ss = &ps->vp->scissors[i];
   state->scissors[i].minx = ss->offset.x;
   state->scissors[i].miny = ss->offset.y;
   state->scissors[i].maxx = ss->offset.x + ss->extent.width;
      }
               if (!BITSET_TEST(ps->dynamic, MESA_VK_DYNAMIC_VP_DEPTH_CLIP_NEGATIVE_ONE_TO_ONE) &&
      state->rs_state.clip_halfz != !ps->vp->depth_clip_negative_one_to_one) {
   state->rs_state.clip_halfz = !ps->vp->depth_clip_negative_one_to_one;
   state->rs_dirty = true;
   for (uint32_t i = 0; i < state->num_viewports; i++)
                  }
      static void handle_pipeline(struct vk_cmd_queue_entry *cmd,
         {
      LVP_FROM_HANDLE(lvp_pipeline, pipeline, cmd->u.bind_pipeline.pipeline);
   pipeline->used = true;
   if (pipeline->type == LVP_PIPELINE_COMPUTE) {
         } else if (pipeline->type == LVP_PIPELINE_GRAPHICS) {
         } else if (pipeline->type == LVP_PIPELINE_EXEC_GRAPH) {
         }
      }
      static void
   handle_graphics_pipeline_group(struct vk_cmd_queue_entry *cmd, struct rendering_state *state)
   {
      assert(cmd->u.bind_pipeline_shader_group_nv.pipeline_bind_point == VK_PIPELINE_BIND_POINT_GRAPHICS);
   LVP_FROM_HANDLE(lvp_pipeline, pipeline, cmd->u.bind_pipeline_shader_group_nv.pipeline);
   if (cmd->u.bind_pipeline_shader_group_nv.group_index)
         handle_graphics_pipeline(pipeline, state);
      }
      static void handle_vertex_buffers2(struct vk_cmd_queue_entry *cmd,
         {
               int i;
   for (i = 0; i < vcb->binding_count; i++) {
               state->vb[idx].buffer_offset = vcb->offsets[i];
   if (state->vb_sizes[idx] != UINT32_MAX)
         state->vb[idx].buffer.resource = vcb->buffers[i] && (!vcb->sizes || vcb->sizes[i]) ? lvp_buffer_from_handle(vcb->buffers[i])->bo : NULL;
   if (state->vb[idx].buffer.resource && vcb->sizes) {
      if (vcb->sizes[i] == VK_WHOLE_SIZE || vcb->offsets[i] + vcb->sizes[i] >= state->vb[idx].buffer.resource->width0) {
         } else {
      struct pipe_transfer *xfer;
   uint8_t *mem = pipe_buffer_map(state->pctx, state->vb[idx].buffer.resource, 0, &xfer);
   state->pctx->buffer_unmap(state->pctx, xfer);
   state->vb[idx].buffer.resource = get_buffer_resource(state->pctx, mem);
   state->vb[idx].buffer.resource->width0 = MIN2(vcb->offsets[i] + vcb->sizes[i], state->vb[idx].buffer.resource->width0);
         } else {
                  if (vcb->strides) {
      state->vb_strides[idx] = vcb->strides[i];
         }
   if (vcb->first_binding < state->start_vb)
         if (vcb->first_binding + vcb->binding_count >= state->num_vb)
            }
      static void
   handle_set_stage_buffer(struct rendering_state *state,
                           {
      state->const_buffer[stage][index].buffer = bo;
   state->const_buffer[stage][index].buffer_offset = offset;
   state->const_buffer[stage][index].buffer_size = bo->width0;
                     if (state->num_const_bufs[stage] <= index)
      }
      static void handle_set_stage(struct rendering_state *state,
                           {
      state->desc_sets[pipeline_type][index] = set;
      }
      static void
   apply_dynamic_offsets(struct lvp_descriptor_set **out_set, uint32_t *offsets, uint32_t offset_count,
         {
      if (!offset_count)
                     struct lvp_descriptor_set *set;
                                       for (uint32_t i = 0; i < set->layout->binding_count; i++) {
      const struct lvp_descriptor_set_binding_layout *binding = &set->layout->binding[i];
   if (binding->type != VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC &&
                  struct lp_descriptor *desc = set->map;
            for (uint32_t j = 0; j < binding->array_size; j++) {
      uint32_t offset_index = binding->dynamic_index + j;
                           }
      static void
   handle_descriptor_sets(struct vk_cmd_queue_entry *cmd, struct rendering_state *state)
   {
      struct vk_cmd_bind_descriptor_sets *bds = &cmd->u.bind_descriptor_sets;
                              for (uint32_t i = 0; i < bds->descriptor_set_count; i++) {
      if (state->desc_buffers[bds->first_set + i]) {
      /* always unset descriptor buffers when binding sets */
   if (pipeline_type == LVP_PIPELINE_COMPUTE) {
         bool changed = state->const_buffer[MESA_SHADER_COMPUTE][bds->first_set + i].buffer == state->desc_buffers[bds->first_set + i];
   } else {
      lvp_forall_gfx_stage(j) {
      bool changed = state->const_buffer[j][bds->first_set + i].buffer == state->desc_buffers[bds->first_set + i];
            }
   if (!layout->vk.set_layouts[bds->first_set + i])
            struct lvp_descriptor_set *set = lvp_descriptor_set_from_handle(bds->descriptor_sets[i]);
   if (!set)
            apply_dynamic_offsets(&set, bds->dynamic_offsets + dynamic_offset_index,
                     if (pipeline_type == LVP_PIPELINE_COMPUTE || pipeline_type == LVP_PIPELINE_EXEC_GRAPH) {
      if (set->layout->shader_stages & VK_SHADER_STAGE_COMPUTE_BIT)
                     if (set->layout->shader_stages & VK_SHADER_STAGE_VERTEX_BIT)
            if (set->layout->shader_stages & VK_SHADER_STAGE_GEOMETRY_BIT)
            if (set->layout->shader_stages & VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT)
            if (set->layout->shader_stages & VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT)
            if (set->layout->shader_stages & VK_SHADER_STAGE_FRAGMENT_BIT)
            if (set->layout->shader_stages & VK_SHADER_STAGE_TASK_BIT_EXT)
            if (set->layout->shader_stages & VK_SHADER_STAGE_MESH_BIT_EXT)
         }
      static struct pipe_surface *create_img_surface_bo(struct rendering_state *state,
                                             {
      if (pformat == PIPE_FORMAT_NONE)
            const struct pipe_surface template = {
      .format = pformat,
   .width = width,
   .height = height,
   .u.tex.first_layer = range->baseArrayLayer + base_layer,
   .u.tex.last_layer = range->baseArrayLayer + base_layer + layer_count - 1,
               return state->pctx->create_surface(state->pctx,
         }
   static struct pipe_surface *create_img_surface(struct rendering_state *state,
                           {
      VkImageSubresourceRange imgv_subres =
            return create_img_surface_bo(state, &imgv_subres, imgv->image->planes[0].bo,
            }
      static void add_img_view_surface(struct rendering_state *state,
               {
      if (imgv->surface) {
      if ((imgv->surface->u.tex.last_layer - imgv->surface->u.tex.first_layer) != (layer_count - 1))
               if (!imgv->surface) {
      imgv->surface = create_img_surface(state, imgv, imgv->vk.format,
               }
      static bool
   render_needs_clear(struct rendering_state *state)
   {
      for (uint32_t i = 0; i < state->color_att_count; i++) {
      if (state->color_att[i].load_op == VK_ATTACHMENT_LOAD_OP_CLEAR)
      }
   if (state->depth_att.load_op == VK_ATTACHMENT_LOAD_OP_CLEAR)
         if (state->stencil_att.load_op == VK_ATTACHMENT_LOAD_OP_CLEAR)
            }
      static void clear_attachment_layers(struct rendering_state *state,
                                       {
      struct pipe_surface *clear_surf = create_img_surface(state,
                                          if (ds_clear_flags) {
      state->pctx->clear_depth_stencil(state->pctx,
                                    } else {
      state->pctx->clear_render_target(state->pctx, clear_surf,
                        }
      }
      static void render_clear(struct rendering_state *state)
   {
      for (uint32_t i = 0; i < state->color_att_count; i++) {
      if (state->color_att[i].load_op != VK_ATTACHMENT_LOAD_OP_CLEAR)
            union pipe_color_union color_clear_val = { 0 };
   const VkClearValue value = state->color_att[i].clear_value;
   color_clear_val.ui[0] = value.color.uint32[0];
   color_clear_val.ui[1] = value.color.uint32[1];
   color_clear_val.ui[2] = value.color.uint32[2];
            struct lvp_image_view *imgv = state->color_att[i].imgv;
            if (state->info.view_mask) {
      u_foreach_bit(i, state->info.view_mask)
      clear_attachment_layers(state, imgv, &state->render_area,
   } else {
      state->pctx->clear_render_target(state->pctx,
                                    imgv->surface,
               uint32_t ds_clear_flags = 0;
   double dclear_val = 0;
   if (state->depth_att.load_op == VK_ATTACHMENT_LOAD_OP_CLEAR) {
      ds_clear_flags |= PIPE_CLEAR_DEPTH;
               uint32_t sclear_val = 0;
   if (state->stencil_att.load_op == VK_ATTACHMENT_LOAD_OP_CLEAR) {
      ds_clear_flags |= PIPE_CLEAR_STENCIL;
               if (ds_clear_flags) {
      if (state->info.view_mask) {
      u_foreach_bit(i, state->info.view_mask)
      clear_attachment_layers(state, state->ds_imgv, &state->render_area,
   } else {
      state->pctx->clear_depth_stencil(state->pctx,
                                    state->ds_imgv->surface,
   ds_clear_flags,
         }
      static void render_clear_fast(struct rendering_state *state)
   {
      /*
   * the state tracker clear interface only works if all the attachments have the same
   * clear color.
   */
   /* llvmpipe doesn't support scissored clears yet */
   if (state->render_area.offset.x || state->render_area.offset.y)
            if (state->render_area.extent.width != state->framebuffer.width ||
      state->render_area.extent.height != state->framebuffer.height)
         if (state->info.view_mask)
            if (state->render_cond)
            uint32_t buffers = 0;
   bool has_color_value = false;
   VkClearValue color_value = {0};
   for (uint32_t i = 0; i < state->color_att_count; i++) {
      if (state->color_att[i].load_op != VK_ATTACHMENT_LOAD_OP_CLEAR)
                     if (has_color_value) {
      if (memcmp(&color_value, &state->color_att[i].clear_value, sizeof(VkClearValue)))
      } else {
      memcpy(&color_value, &state->color_att[i].clear_value, sizeof(VkClearValue));
                  double dclear_val = 0;
   if (state->depth_att.load_op == VK_ATTACHMENT_LOAD_OP_CLEAR) {
      buffers |= PIPE_CLEAR_DEPTH;
               uint32_t sclear_val = 0;
   if (state->stencil_att.load_op == VK_ATTACHMENT_LOAD_OP_CLEAR) {
      buffers |= PIPE_CLEAR_STENCIL;
               union pipe_color_union col_val;
   for (unsigned i = 0; i < 4; i++)
            state->pctx->clear(state->pctx, buffers,
                     slow_clear:
         }
      static struct lvp_image_view *
   destroy_multisample_surface(struct rendering_state *state, struct lvp_image_view *imgv)
   {
      assert(imgv->image->vk.samples > 1);
   struct lvp_image_view *base = imgv->multisample;
   base->multisample = NULL;
   free((void*)imgv->image);
   pipe_surface_reference(&imgv->surface, NULL);
   free(imgv);
      }
      static void
   resolve_ds(struct rendering_state *state, bool multi)
   {
      VkResolveModeFlagBits depth_resolve_mode = multi ? state->forced_depth_resolve_mode : state->depth_att.resolve_mode;
   VkResolveModeFlagBits stencil_resolve_mode = multi ? state->forced_stencil_resolve_mode : state->stencil_att.resolve_mode;
   if (!depth_resolve_mode && !stencil_resolve_mode)
            struct lvp_image_view *src_imgv = state->ds_imgv;
   if (multi && !src_imgv->multisample)
         if (!multi && src_imgv->image->vk.samples == 1)
            assert(state->depth_att.resolve_imgv == NULL ||
         state->stencil_att.resolve_imgv == NULL ||
   state->depth_att.resolve_imgv == state->stencil_att.resolve_imgv ||
   struct lvp_image_view *dst_imgv =
      multi ? src_imgv->multisample :
   state->depth_att.resolve_imgv ? state->depth_att.resolve_imgv :
         unsigned num_blits = 1;
   if (depth_resolve_mode != stencil_resolve_mode)
            for (unsigned i = 0; i < num_blits; i++) {
      if (i == 0 && depth_resolve_mode == VK_RESOLVE_MODE_NONE)
            if (i == 1 && stencil_resolve_mode == VK_RESOLVE_MODE_NONE)
                     info.src.resource = src_imgv->image->planes[0].bo;
   info.dst.resource = dst_imgv->image->planes[0].bo;
   info.src.format = src_imgv->pformat;
   info.dst.format = dst_imgv->pformat;
            if (num_blits == 1)
         else if (i == 0)
         else
            if (i == 0 && depth_resolve_mode == VK_RESOLVE_MODE_SAMPLE_ZERO_BIT)
         if (i == 1 && stencil_resolve_mode == VK_RESOLVE_MODE_SAMPLE_ZERO_BIT)
            info.src.box.x = state->render_area.offset.x;
   info.src.box.y = state->render_area.offset.y;
   info.src.box.width = state->render_area.extent.width;
   info.src.box.height = state->render_area.extent.height;
                        }
   if (multi)
      }
      static void
   resolve_color(struct rendering_state *state, bool multi)
   {
      for (uint32_t i = 0; i < state->color_att_count; i++) {
      if (!state->color_att[i].resolve_mode &&
                  struct lvp_image_view *src_imgv = state->color_att[i].imgv;
   /* skip non-msrtss resolves during msrtss resolve */
   if (multi && !src_imgv->multisample)
                           info.src.resource = src_imgv->image->planes[0].bo;
   info.dst.resource = dst_imgv->image->planes[0].bo;
   info.src.format = src_imgv->pformat;
   info.dst.format = dst_imgv->pformat;
   info.filter = PIPE_TEX_FILTER_NEAREST;
   info.mask = PIPE_MASK_RGBA;
   info.src.box.x = state->render_area.offset.x;
   info.src.box.y = state->render_area.offset.y;
   info.src.box.width = state->render_area.extent.width;
   info.src.box.height = state->render_area.extent.height;
            info.dst.box = info.src.box;
   info.src.box.z = src_imgv->vk.base_array_layer;
            info.src.level = src_imgv->vk.base_mip_level;
                        if (!multi)
         for (uint32_t i = 0; i < state->color_att_count; i++) {
      struct lvp_image_view *src_imgv = state->color_att[i].imgv;
   if (src_imgv && src_imgv->multisample) //check if it has a msrtss view
         }
      static void render_resolve(struct rendering_state *state)
   {
      if (state->forced_sample_count) {
      resolve_ds(state, true);
      }
   resolve_ds(state, false);
      }
      static void
   replicate_attachment(struct rendering_state *state,
               {
      unsigned level = dst->surface->u.tex.level;
   const struct pipe_box box = {
      .x = 0,
   .y = 0,
   .z = 0,
   .width = u_minify(dst->image->planes[0].bo->width0, level),
   .height = u_minify(dst->image->planes[0].bo->height0, level),
      };
   state->pctx->resource_copy_region(state->pctx, dst->image->planes[0].bo, level,
      }
      static struct lvp_image_view *
   create_multisample_surface(struct rendering_state *state, struct lvp_image_view *imgv, uint32_t samples, bool replicate)
   {
               struct pipe_resource templ = *imgv->surface->texture;
   templ.nr_samples = samples;
   struct lvp_image *image = mem_dup(imgv->image, sizeof(struct lvp_image));
   image->vk.samples = samples;
   image->planes[0].pmem = NULL;
            struct lvp_image_view *multi = mem_dup(imgv, sizeof(struct lvp_image_view));
   multi->image = image;
   multi->surface = state->pctx->create_surface(state->pctx, image->planes[0].bo, imgv->surface);
   struct pipe_resource *ref = image->planes[0].bo;
   pipe_resource_reference(&ref, NULL);
   imgv->multisample = multi;
   multi->multisample = imgv;
   if (replicate)
            }
      static bool
   att_needs_replicate(const struct rendering_state *state,
               {
      if (load_op == VK_ATTACHMENT_LOAD_OP_LOAD ||
      load_op == VK_ATTACHMENT_LOAD_OP_CLEAR)
      if (state->render_area.offset.x || state->render_area.offset.y)
         if (state->render_area.extent.width < imgv->image->vk.extent.width ||
      state->render_area.extent.height < imgv->image->vk.extent.height)
         }
         static void
   render_att_init(struct lvp_render_attachment* att,
               {
      if (vk_att == NULL || vk_att->imageView == VK_NULL_HANDLE) {
      *att = (struct lvp_render_attachment) {
         };
               *att = (struct lvp_render_attachment) {
      .imgv = lvp_image_view_from_handle(vk_att->imageView),
   .load_op = vk_att->loadOp,
   .store_op = vk_att->storeOp,
      };
   if (util_format_is_depth_or_stencil(att->imgv->pformat)) {
      if (stencil) {
      att->read_only =
      (vk_att->imageLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL ||
   } else {
      att->read_only =
      (vk_att->imageLayout == VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL ||
      }
   if (poison_mem && !att->read_only && att->load_op == VK_ATTACHMENT_LOAD_OP_DONT_CARE) {
      att->load_op = VK_ATTACHMENT_LOAD_OP_CLEAR;
   if (util_format_is_depth_or_stencil(att->imgv->pformat)) {
      att->clear_value.depthStencil.depth = 0.12351251;
      } else {
      memset(att->clear_value.color.uint32, rand() % UINT8_MAX,
                  if (vk_att->resolveImageView && vk_att->resolveMode) {
      att->resolve_imgv = lvp_image_view_from_handle(vk_att->resolveImageView);
         }
         static void
   handle_begin_rendering(struct vk_cmd_queue_entry *cmd,
         {
      const VkRenderingInfo *info = cmd->u.begin_rendering.rendering_info;
   bool resuming = (info->flags & VK_RENDERING_RESUMING_BIT) == VK_RENDERING_RESUMING_BIT;
            const VkMultisampledRenderToSingleSampledInfoEXT *ssi =
         if (ssi && ssi->multisampledRenderToSingleSampledEnable) {
      state->forced_sample_count = ssi->rasterizationSamples;
   state->forced_depth_resolve_mode = info->pDepthAttachment ? info->pDepthAttachment->resolveMode : 0;
      } else {
      state->forced_sample_count = 0;
   state->forced_depth_resolve_mode = 0;
               state->info.view_mask = info->viewMask;
   state->render_area = info->renderArea;
   state->suspending = suspending;
   state->framebuffer.width = info->renderArea.offset.x +
         state->framebuffer.height = info->renderArea.offset.y +
         state->framebuffer.layers = info->viewMask ? util_last_bit(info->viewMask) : info->layerCount;
            state->color_att_count = info->colorAttachmentCount;
   memset(state->framebuffer.cbufs, 0, sizeof(state->framebuffer.cbufs));
   for (unsigned i = 0; i < info->colorAttachmentCount; i++) {
      render_att_init(&state->color_att[i], &info->pColorAttachments[i], state->poison_mem, false);
   if (state->color_att[i].imgv) {
      struct lvp_image_view *imgv = state->color_att[i].imgv;
   add_img_view_surface(state, imgv,
               if (state->forced_sample_count && imgv->image->vk.samples == 1)
      state->color_att[i].imgv = create_multisample_surface(state, imgv, state->forced_sample_count,
      state->framebuffer.cbufs[i] = state->color_att[i].imgv->surface;
   assert(state->render_area.offset.x + state->render_area.extent.width <= state->framebuffer.cbufs[i]->texture->width0);
      } else {
                     render_att_init(&state->depth_att, info->pDepthAttachment, state->poison_mem, false);
   render_att_init(&state->stencil_att, info->pStencilAttachment, state->poison_mem, true);
   if (state->depth_att.imgv || state->stencil_att.imgv) {
      assert(state->depth_att.imgv == NULL ||
         state->stencil_att.imgv == NULL ||
   state->ds_imgv = state->depth_att.imgv ? state->depth_att.imgv :
         struct lvp_image_view *imgv = state->ds_imgv;
   add_img_view_surface(state, imgv,
               if (state->forced_sample_count && imgv->image->vk.samples == 1) {
      VkAttachmentLoadOp load_op;
   if (state->depth_att.load_op == VK_ATTACHMENT_LOAD_OP_CLEAR ||
      state->stencil_att.load_op == VK_ATTACHMENT_LOAD_OP_CLEAR)
      else if (state->depth_att.load_op == VK_ATTACHMENT_LOAD_OP_LOAD ||
               else
         state->ds_imgv = create_multisample_surface(state, imgv, state->forced_sample_count,
      }
   state->framebuffer.zsbuf = state->ds_imgv->surface;
   assert(state->render_area.offset.x + state->render_area.extent.width <= state->framebuffer.zsbuf->texture->width0);
      } else {
      state->ds_imgv = NULL;
               state->pctx->set_framebuffer_state(state->pctx,
            if (!resuming && render_needs_clear(state))
      }
      static void handle_end_rendering(struct vk_cmd_queue_entry *cmd,
         {
      if (state->suspending)
         render_resolve(state);
   if (!state->poison_mem)
            union pipe_color_union color_clear_val;
            for (unsigned i = 0; i < state->framebuffer.nr_cbufs; i++) {
      if (state->color_att[i].imgv && state->color_att[i].store_op == VK_ATTACHMENT_STORE_OP_DONT_CARE) {
      if (state->info.view_mask) {
      u_foreach_bit(i, state->info.view_mask)
      clear_attachment_layers(state, state->color_att[i].imgv, &state->render_area,
   } else {
      state->pctx->clear_render_target(state->pctx,
                                    state->color_att[i].imgv->surface,
         }
   uint32_t ds_clear_flags = 0;
   if (state->depth_att.imgv && !state->depth_att.read_only && state->depth_att.store_op == VK_ATTACHMENT_STORE_OP_DONT_CARE)
         if (state->stencil_att.imgv && !state->stencil_att.read_only && state->stencil_att.store_op == VK_ATTACHMENT_STORE_OP_DONT_CARE)
         double dclear_val = 0.2389234;
   uint32_t sclear_val = rand() % UINT8_MAX;
   if (ds_clear_flags) {
      if (state->info.view_mask) {
      u_foreach_bit(i, state->info.view_mask)
      clear_attachment_layers(state, state->ds_imgv, &state->render_area,
   } else {
      state->pctx->clear_depth_stencil(state->pctx,
                                    state->ds_imgv->surface,
   ds_clear_flags,
         }
      static void handle_draw(struct vk_cmd_queue_entry *cmd,
         {
               state->info.index_size = 0;
   state->info.index.resource = NULL;
   state->info.start_instance = cmd->u.draw.first_instance;
            draw.start = cmd->u.draw.first_vertex;
   draw.count = cmd->u.draw.vertex_count;
               }
      static void handle_draw_multi(struct vk_cmd_queue_entry *cmd,
         {
      struct pipe_draw_start_count_bias *draws = calloc(cmd->u.draw_multi_ext.draw_count,
            state->info.index_size = 0;
   state->info.index.resource = NULL;
   state->info.start_instance = cmd->u.draw_multi_ext.first_instance;
   state->info.instance_count = cmd->u.draw_multi_ext.instance_count;
   if (cmd->u.draw_multi_ext.draw_count > 1)
            for (unsigned i = 0; i < cmd->u.draw_multi_ext.draw_count; i++) {
      draws[i].start = cmd->u.draw_multi_ext.vertex_info[i].firstVertex;
   draws[i].count = cmd->u.draw_multi_ext.vertex_info[i].vertexCount;
               if (cmd->u.draw_multi_indexed_ext.draw_count)
               }
      static void set_viewport(unsigned first_viewport, unsigned viewport_count,
               {
      unsigned base = 0;
   if (first_viewport == UINT32_MAX)
         else
            for (unsigned i = 0; i < viewport_count; i++) {
      int idx = i + base;
   const VkViewport *vp = &viewports[i];
   get_viewport_xform(state, vp, idx);
      }
      }
      static void handle_set_viewport(struct vk_cmd_queue_entry *cmd,
         {
      set_viewport(cmd->u.set_viewport.first_viewport,
                  }
      static void handle_set_viewport_with_count(struct vk_cmd_queue_entry *cmd,
         {
      set_viewport(UINT32_MAX,
                  }
      static void set_scissor(unsigned first_scissor,
                     {
      unsigned base = 0;
   if (first_scissor == UINT32_MAX)
         else
            for (unsigned i = 0; i < scissor_count; i++) {
      unsigned idx = i + base;
   const VkRect2D *ss = &scissors[i];
   state->scissors[idx].minx = ss->offset.x;
   state->scissors[idx].miny = ss->offset.y;
   state->scissors[idx].maxx = ss->offset.x + ss->extent.width;
      }
      }
      static void handle_set_scissor(struct vk_cmd_queue_entry *cmd,
         {
      set_scissor(cmd->u.set_scissor.first_scissor,
                  }
      static void handle_set_scissor_with_count(struct vk_cmd_queue_entry *cmd,
         {
      set_scissor(UINT32_MAX,
                  }
      static void handle_set_line_width(struct vk_cmd_queue_entry *cmd,
         {
      state->rs_state.line_width = cmd->u.set_line_width.line_width;
      }
      static void handle_set_depth_bias(struct vk_cmd_queue_entry *cmd,
         {
      state->depth_bias.offset_units = cmd->u.set_depth_bias.depth_bias_constant_factor;
   state->depth_bias.offset_scale = cmd->u.set_depth_bias.depth_bias_slope_factor;
   state->depth_bias.offset_clamp = cmd->u.set_depth_bias.depth_bias_clamp;
      }
      static void handle_set_blend_constants(struct vk_cmd_queue_entry *cmd,
         {
      memcpy(state->blend_color.color, cmd->u.set_blend_constants.blend_constants, 4 * sizeof(float));
      }
      static void handle_set_depth_bounds(struct vk_cmd_queue_entry *cmd,
         {
      state->dsa_dirty |= !DOUBLE_EQ(state->dsa_state.depth_bounds_min, cmd->u.set_depth_bounds.min_depth_bounds);
   state->dsa_dirty |= !DOUBLE_EQ(state->dsa_state.depth_bounds_max, cmd->u.set_depth_bounds.max_depth_bounds);
   state->dsa_state.depth_bounds_min = cmd->u.set_depth_bounds.min_depth_bounds;
      }
      static void handle_set_stencil_compare_mask(struct vk_cmd_queue_entry *cmd,
         {
      if (cmd->u.set_stencil_compare_mask.face_mask & VK_STENCIL_FACE_FRONT_BIT)
         if (cmd->u.set_stencil_compare_mask.face_mask & VK_STENCIL_FACE_BACK_BIT)
            }
      static void handle_set_stencil_write_mask(struct vk_cmd_queue_entry *cmd,
         {
      if (cmd->u.set_stencil_write_mask.face_mask & VK_STENCIL_FACE_FRONT_BIT)
         if (cmd->u.set_stencil_write_mask.face_mask & VK_STENCIL_FACE_BACK_BIT)
            }
      static void handle_set_stencil_reference(struct vk_cmd_queue_entry *cmd,
         {
      if (cmd->u.set_stencil_reference.face_mask & VK_STENCIL_FACE_FRONT_BIT)
         if (cmd->u.set_stencil_reference.face_mask & VK_STENCIL_FACE_BACK_BIT)
            }
      static void
   copy_depth_rect(uint8_t * dst,
                  enum pipe_format dst_format,
   unsigned dst_stride,
   unsigned dst_x,
   unsigned dst_y,
   unsigned width,
   unsigned height,
   const uint8_t * src,
   enum pipe_format src_format,
      {
      int src_stride_pos = src_stride < 0 ? -src_stride : src_stride;
   int src_blocksize = util_format_get_blocksize(src_format);
   int src_blockwidth = util_format_get_blockwidth(src_format);
   int src_blockheight = util_format_get_blockheight(src_format);
   int dst_blocksize = util_format_get_blocksize(dst_format);
   int dst_blockwidth = util_format_get_blockwidth(dst_format);
            assert(src_blocksize > 0);
   assert(src_blockwidth > 0);
            dst_x /= dst_blockwidth;
   dst_y /= dst_blockheight;
   width = (width + src_blockwidth - 1)/src_blockwidth;
   height = (height + src_blockheight - 1)/src_blockheight;
   src_x /= src_blockwidth;
            dst += dst_x * dst_blocksize;
   src += src_x * src_blocksize;
   dst += dst_y * dst_stride;
            if (dst_format == PIPE_FORMAT_S8_UINT) {
      if (src_format == PIPE_FORMAT_Z32_FLOAT_S8X24_UINT) {
      util_format_z32_float_s8x24_uint_unpack_s_8uint(dst, dst_stride,
            } else if (src_format == PIPE_FORMAT_Z24_UNORM_S8_UINT) {
      util_format_z24_unorm_s8_uint_unpack_s_8uint(dst, dst_stride,
            } else {
      } else if (dst_format == PIPE_FORMAT_Z24X8_UNORM) {
      util_format_z24_unorm_s8_uint_unpack_z24(dst, dst_stride,
            } else if (dst_format == PIPE_FORMAT_Z32_FLOAT) {
      if (src_format == PIPE_FORMAT_Z32_FLOAT_S8X24_UINT) {
      util_format_z32_float_s8x24_uint_unpack_z_float((float *)dst, dst_stride,
               } else if (dst_format == PIPE_FORMAT_Z32_FLOAT_S8X24_UINT) {
      if (src_format == PIPE_FORMAT_Z32_FLOAT)
      util_format_z32_float_s8x24_uint_pack_z_float(dst, dst_stride,
            else if (src_format == PIPE_FORMAT_S8_UINT)
      util_format_z32_float_s8x24_uint_pack_s_8uint(dst, dst_stride,
         } else if (dst_format == PIPE_FORMAT_Z24_UNORM_S8_UINT) {
      if (src_format == PIPE_FORMAT_S8_UINT)
      util_format_z24_unorm_s8_uint_pack_s_8uint(dst, dst_stride,
            if (src_format == PIPE_FORMAT_Z24X8_UNORM)
      util_format_z24_unorm_s8_uint_pack_z24(dst, dst_stride,
            }
      static void
   copy_depth_box(uint8_t *dst,
                  enum pipe_format dst_format,
   unsigned dst_stride, uint64_t dst_slice_stride,
   unsigned dst_x, unsigned dst_y, unsigned dst_z,
   unsigned width, unsigned height, unsigned depth,
   const uint8_t * src,
      {
      dst += dst_z * dst_slice_stride;
   src += src_z * src_slice_stride;
   for (unsigned z = 0; z < depth; ++z) {
      copy_depth_rect(dst,
                  dst_format,
   dst_stride,
   dst_x, dst_y,
   width, height,
               dst += dst_slice_stride;
         }
      static unsigned
   subresource_layercount(const struct lvp_image *image, const VkImageSubresourceLayers *sub)
   {
      if (sub->layerCount != VK_REMAINING_ARRAY_LAYERS)
            }
      static void handle_copy_image_to_buffer2(struct vk_cmd_queue_entry *cmd,
         {
      const struct VkCopyImageToBufferInfo2 *copycmd = cmd->u.copy_image_to_buffer2.copy_image_to_buffer_info;
   LVP_FROM_HANDLE(lvp_image, src_image, copycmd->srcImage);
   struct pipe_box box, dbox;
   struct pipe_transfer *src_t, *dst_t;
            for (uint32_t i = 0; i < copycmd->regionCount; i++) {
      const VkBufferImageCopy2 *region = &copycmd->pRegions[i];
   const VkImageAspectFlagBits aspects = copycmd->pRegions[i].imageSubresource.aspectMask;
            box.x = region->imageOffset.x;
   box.y = region->imageOffset.y;
   box.z = src_image->vk.image_type == VK_IMAGE_TYPE_3D ? region->imageOffset.z : region->imageSubresource.baseArrayLayer;
   box.width = region->imageExtent.width;
   box.height = region->imageExtent.height;
            src_data = state->pctx->texture_map(state->pctx,
                                    dbox.x = region->bufferOffset;
   dbox.y = 0;
   dbox.z = 0;
   dbox.width = lvp_buffer_from_handle(copycmd->dstBuffer)->bo->width0 - region->bufferOffset;
   dbox.height = 1;
   dbox.depth = 1;
   dst_data = state->pctx->buffer_map(state->pctx,
                                    enum pipe_format src_format = src_image->planes[plane].bo->format;
   enum pipe_format dst_format = src_format;
   if (util_format_is_depth_or_stencil(src_format)) {
      if (region->imageSubresource.aspectMask == VK_IMAGE_ASPECT_DEPTH_BIT) {
         } else if (region->imageSubresource.aspectMask == VK_IMAGE_ASPECT_STENCIL_BIT) {
                     const struct vk_image_buffer_layout buffer_layout =
         if (src_format != dst_format) {
      copy_depth_box(dst_data, dst_format,
                  buffer_layout.row_stride_B,
   buffer_layout.image_stride_B,
   0, 0, 0,
   region->imageExtent.width,
   } else {
      util_copy_box((uint8_t *)dst_data, src_format,
               buffer_layout.row_stride_B,
   buffer_layout.image_stride_B,
   0, 0, 0,
   region->imageExtent.width,
      }
   state->pctx->texture_unmap(state->pctx, src_t);
         }
      static void handle_copy_buffer_to_image(struct vk_cmd_queue_entry *cmd,
         {
      const struct VkCopyBufferToImageInfo2 *copycmd = cmd->u.copy_buffer_to_image2.copy_buffer_to_image_info;
            for (uint32_t i = 0; i < copycmd->regionCount; i++) {
      const VkBufferImageCopy2 *region = &copycmd->pRegions[i];
   struct pipe_box box, sbox;
   struct pipe_transfer *src_t, *dst_t;
   void *src_data, *dst_data;
   const VkImageAspectFlagBits aspects = copycmd->pRegions[i].imageSubresource.aspectMask;
            sbox.x = region->bufferOffset;
   sbox.y = 0;
   sbox.z = 0;
   sbox.width = lvp_buffer_from_handle(copycmd->srcBuffer)->bo->width0;
   sbox.height = 1;
   sbox.depth = 1;
   src_data = state->pctx->buffer_map(state->pctx,
                                       box.x = region->imageOffset.x;
   box.y = region->imageOffset.y;
   box.z = dst_image->vk.image_type == VK_IMAGE_TYPE_3D ? region->imageOffset.z : region->imageSubresource.baseArrayLayer;
   box.width = region->imageExtent.width;
   box.height = region->imageExtent.height;
            dst_data = state->pctx->texture_map(state->pctx,
                                    enum pipe_format dst_format = dst_image->planes[plane].bo->format;
   enum pipe_format src_format = dst_format;
   if (util_format_is_depth_or_stencil(dst_format)) {
      if (region->imageSubresource.aspectMask == VK_IMAGE_ASPECT_DEPTH_BIT) {
         } else if (region->imageSubresource.aspectMask == VK_IMAGE_ASPECT_STENCIL_BIT) {
                     const struct vk_image_buffer_layout buffer_layout =
         if (src_format != dst_format) {
      copy_depth_box(dst_data, dst_format,
                  dst_t->stride, dst_t->layer_stride,
   0, 0, 0,
   region->imageExtent.width,
   region->imageExtent.height,
   box.depth,
   src_data, src_format,
   } else {
      util_copy_box(dst_data, dst_format,
               dst_t->stride, dst_t->layer_stride,
   0, 0, 0,
   region->imageExtent.width,
   region->imageExtent.height,
   box.depth,
   src_data,
      }
   state->pctx->buffer_unmap(state->pctx, src_t);
         }
      static void handle_copy_image(struct vk_cmd_queue_entry *cmd,
         {
      const struct VkCopyImageInfo2 *copycmd = cmd->u.copy_image2.copy_image_info;
   LVP_FROM_HANDLE(lvp_image, src_image, copycmd->srcImage);
            for (uint32_t i = 0; i < copycmd->regionCount; i++) {
      const VkImageCopy2 *region = &copycmd->pRegions[i];
   const VkImageAspectFlagBits src_aspects =
         uint8_t src_plane = lvp_image_aspects_to_plane(src_image, src_aspects);
   const VkImageAspectFlagBits dst_aspects =
         uint8_t dst_plane = lvp_image_aspects_to_plane(dst_image, dst_aspects);
   struct pipe_box src_box;
   src_box.x = region->srcOffset.x;
   src_box.y = region->srcOffset.y;
   src_box.width = region->extent.width;
   src_box.height = region->extent.height;
   if (src_image->planes[src_plane].bo->target == PIPE_TEXTURE_3D) {
      src_box.depth = region->extent.depth;
      } else {
      src_box.depth = subresource_layercount(src_image, &region->srcSubresource);
               unsigned dstz = dst_image->planes[dst_plane].bo->target == PIPE_TEXTURE_3D ?
               state->pctx->resource_copy_region(state->pctx, dst_image->planes[dst_plane].bo,
                                    region->dstSubresource.mipLevel,
      }
      static void handle_copy_buffer(struct vk_cmd_queue_entry *cmd,
         {
               for (uint32_t i = 0; i < copycmd->regionCount; i++) {
      const VkBufferCopy2 *region = &copycmd->pRegions[i];
   struct pipe_box box = { 0 };
   u_box_1d(region->srcOffset, region->size, &box);
   state->pctx->resource_copy_region(state->pctx, lvp_buffer_from_handle(copycmd->dstBuffer)->bo, 0,
               }
      static void handle_blit_image(struct vk_cmd_queue_entry *cmd,
         {
      VkBlitImageInfo2 *blitcmd = cmd->u.blit_image2.blit_image_info;
   LVP_FROM_HANDLE(lvp_image, src_image, blitcmd->srcImage);
            struct pipe_blit_info info = {
      .src.resource = src_image->planes[0].bo,
   .dst.resource = dst_image->planes[0].bo,
   .src.format = src_image->planes[0].bo->format,
   .dst.format = dst_image->planes[0].bo->format,
   .mask = util_format_is_depth_or_stencil(info.src.format) ? PIPE_MASK_ZS : PIPE_MASK_RGBA,
               for (uint32_t i = 0; i < blitcmd->regionCount; i++) {
      int srcX0, srcX1, srcY0, srcY1, srcZ0, srcZ1;
            srcX0 = blitcmd->pRegions[i].srcOffsets[0].x;
   srcX1 = blitcmd->pRegions[i].srcOffsets[1].x;
   srcY0 = blitcmd->pRegions[i].srcOffsets[0].y;
   srcY1 = blitcmd->pRegions[i].srcOffsets[1].y;
   srcZ0 = blitcmd->pRegions[i].srcOffsets[0].z;
            dstX0 = blitcmd->pRegions[i].dstOffsets[0].x;
   dstX1 = blitcmd->pRegions[i].dstOffsets[1].x;
   dstY0 = blitcmd->pRegions[i].dstOffsets[0].y;
   dstY1 = blitcmd->pRegions[i].dstOffsets[1].y;
   dstZ0 = blitcmd->pRegions[i].dstOffsets[0].z;
            if (dstX0 < dstX1) {
      info.dst.box.x = dstX0;
   info.src.box.x = srcX0;
   info.dst.box.width = dstX1 - dstX0;
      } else {
      info.dst.box.x = dstX1;
   info.src.box.x = srcX1;
   info.dst.box.width = dstX0 - dstX1;
               if (dstY0 < dstY1) {
      info.dst.box.y = dstY0;
   info.src.box.y = srcY0;
   info.dst.box.height = dstY1 - dstY0;
      } else {
      info.dst.box.y = dstY1;
   info.src.box.y = srcY1;
   info.dst.box.height = dstY0 - dstY1;
               assert_subresource_layers(info.src.resource, src_image, &blitcmd->pRegions[i].srcSubresource, blitcmd->pRegions[i].srcOffsets);
   assert_subresource_layers(info.dst.resource, dst_image, &blitcmd->pRegions[i].dstSubresource, blitcmd->pRegions[i].dstOffsets);
   if (src_image->planes[0].bo->target == PIPE_TEXTURE_3D) {
      if (dstZ0 < dstZ1) {
      info.dst.box.z = dstZ0;
   info.src.box.z = srcZ0;
   info.dst.box.depth = dstZ1 - dstZ0;
      } else {
      info.dst.box.z = dstZ1;
   info.src.box.z = srcZ1;
   info.dst.box.depth = dstZ0 - dstZ1;
         } else {
      info.src.box.z = blitcmd->pRegions[i].srcSubresource.baseArrayLayer;
   info.dst.box.z = blitcmd->pRegions[i].dstSubresource.baseArrayLayer;
   info.src.box.depth = subresource_layercount(src_image, &blitcmd->pRegions[i].srcSubresource);
               info.src.level = blitcmd->pRegions[i].srcSubresource.mipLevel;
   info.dst.level = blitcmd->pRegions[i].dstSubresource.mipLevel;
         }
      static void handle_fill_buffer(struct vk_cmd_queue_entry *cmd,
         {
      struct vk_cmd_fill_buffer *fillcmd = &cmd->u.fill_buffer;
   uint32_t size = fillcmd->size;
            size = vk_buffer_range(&dst->vk, fillcmd->dst_offset, fillcmd->size);
   if (fillcmd->size == VK_WHOLE_SIZE)
            state->pctx->clear_buffer(state->pctx,
                              }
      static void handle_update_buffer(struct vk_cmd_queue_entry *cmd,
         {
      struct vk_cmd_update_buffer *updcmd = &cmd->u.update_buffer;
   uint32_t *dst;
   struct pipe_transfer *dst_t;
            u_box_1d(updcmd->dst_offset, updcmd->data_size, &box);
   dst = state->pctx->buffer_map(state->pctx,
                                    memcpy(dst, updcmd->data, updcmd->data_size);
      }
      static void handle_draw_indexed(struct vk_cmd_queue_entry *cmd,
         {
               state->info.index_bounds_valid = false;
   state->info.min_index = 0;
   state->info.max_index = ~0U;
   state->info.index_size = state->index_size;
   state->info.index.resource = state->index_buffer;
   state->info.start_instance = cmd->u.draw_indexed.first_instance;
            if (state->info.primitive_restart)
            draw.count = MIN2(cmd->u.draw_indexed.index_count, state->index_buffer_size / state->index_size);
   draw.index_bias = cmd->u.draw_indexed.vertex_offset;
   /* TODO: avoid calculating multiple times if cmdbuf is submitted again */
   draw.start = util_clamped_uadd(state->index_offset / state->index_size,
            state->info.index_bias_varies = !cmd->u.draw_indexed.vertex_offset;
      }
      static void handle_draw_multi_indexed(struct vk_cmd_queue_entry *cmd,
         {
      struct pipe_draw_start_count_bias *draws = calloc(cmd->u.draw_multi_indexed_ext.draw_count,
            state->info.index_bounds_valid = false;
   state->info.min_index = 0;
   state->info.max_index = ~0U;
   state->info.index_size = state->index_size;
   state->info.index.resource = state->index_buffer;
   state->info.start_instance = cmd->u.draw_multi_indexed_ext.first_instance;
   state->info.instance_count = cmd->u.draw_multi_indexed_ext.instance_count;
   if (cmd->u.draw_multi_indexed_ext.draw_count > 1)
            if (state->info.primitive_restart)
            unsigned size = cmd->u.draw_multi_indexed_ext.draw_count * sizeof(struct pipe_draw_start_count_bias);
   memcpy(draws, cmd->u.draw_multi_indexed_ext.index_info, size);
   if (state->index_buffer_size != UINT32_MAX) {
      for (unsigned i = 0; i < cmd->u.draw_multi_indexed_ext.draw_count; i++)
               /* only the first member is read if index_bias_varies is true */
   if (cmd->u.draw_multi_indexed_ext.draw_count &&
      cmd->u.draw_multi_indexed_ext.vertex_offset)
         /* TODO: avoid calculating multiple times if cmdbuf is submitted again */
   for (unsigned i = 0; i < cmd->u.draw_multi_indexed_ext.draw_count; i++)
      draws[i].start = util_clamped_uadd(state->index_offset / state->index_size,
                  if (cmd->u.draw_multi_indexed_ext.draw_count)
               }
      static void handle_draw_indirect(struct vk_cmd_queue_entry *cmd,
         {
      struct pipe_draw_start_count_bias draw = {0};
   struct pipe_resource *index = NULL;
   if (indexed) {
      state->info.index_bounds_valid = false;
   state->info.index_size = state->index_size;
   state->info.index.resource = state->index_buffer;
   state->info.max_index = ~0U;
   if (state->info.primitive_restart)
         if (state->index_offset || state->index_buffer_size != UINT32_MAX) {
      struct pipe_transfer *xfer;
   uint8_t *mem = pipe_buffer_map(state->pctx, state->index_buffer, 0, &xfer);
   state->pctx->buffer_unmap(state->pctx, xfer);
   index = get_buffer_resource(state->pctx, mem + state->index_offset);
   index->width0 = MIN2(state->index_buffer->width0 - state->index_offset, state->index_buffer_size);
         } else
         state->indirect_info.offset = cmd->u.draw_indirect.offset;
   state->indirect_info.stride = cmd->u.draw_indirect.stride;
   state->indirect_info.draw_count = cmd->u.draw_indirect.draw_count;
            state->pctx->draw_vbo(state->pctx, &state->info, 0, &state->indirect_info, &draw, 1);
      }
      static void handle_index_buffer(struct vk_cmd_queue_entry *cmd,
         {
      struct vk_cmd_bind_index_buffer *ib = &cmd->u.bind_index_buffer;
   state->index_size = vk_index_type_to_bytes(ib->index_type);
            if (ib->buffer) {
      state->index_offset = ib->offset;
      } else {
      state->index_offset = 0;
                  }
      static void handle_index_buffer2(struct vk_cmd_queue_entry *cmd,
         {
      struct vk_cmd_bind_index_buffer2_khr *ib = &cmd->u.bind_index_buffer2_khr;
   state->index_size = vk_index_type_to_bytes(ib->index_type);
            if (ib->buffer) {
      state->index_offset = ib->offset;
      } else {
      state->index_offset = 0;
                  }
      static void handle_dispatch(struct vk_cmd_queue_entry *cmd,
         {
      state->dispatch_info.grid[0] = cmd->u.dispatch.group_count_x;
   state->dispatch_info.grid[1] = cmd->u.dispatch.group_count_y;
   state->dispatch_info.grid[2] = cmd->u.dispatch.group_count_z;
   state->dispatch_info.grid_base[0] = 0;
   state->dispatch_info.grid_base[1] = 0;
   state->dispatch_info.grid_base[2] = 0;
   state->dispatch_info.indirect = NULL;
      }
      static void handle_dispatch_base(struct vk_cmd_queue_entry *cmd,
         {
      state->dispatch_info.grid[0] = cmd->u.dispatch_base.group_count_x;
   state->dispatch_info.grid[1] = cmd->u.dispatch_base.group_count_y;
   state->dispatch_info.grid[2] = cmd->u.dispatch_base.group_count_z;
   state->dispatch_info.grid_base[0] = cmd->u.dispatch_base.base_group_x;
   state->dispatch_info.grid_base[1] = cmd->u.dispatch_base.base_group_y;
   state->dispatch_info.grid_base[2] = cmd->u.dispatch_base.base_group_z;
   state->dispatch_info.indirect = NULL;
      }
      static void handle_dispatch_indirect(struct vk_cmd_queue_entry *cmd,
         {
      state->dispatch_info.indirect = lvp_buffer_from_handle(cmd->u.dispatch_indirect.buffer)->bo;
   state->dispatch_info.indirect_offset = cmd->u.dispatch_indirect.offset;
      }
      static void handle_push_constants(struct vk_cmd_queue_entry *cmd,
         {
               VkShaderStageFlags stage_flags = cmd->u.push_constants.stage_flags;
   state->pcbuf_dirty[MESA_SHADER_VERTEX] |= (stage_flags & VK_SHADER_STAGE_VERTEX_BIT) > 0;
   state->pcbuf_dirty[MESA_SHADER_FRAGMENT] |= (stage_flags & VK_SHADER_STAGE_FRAGMENT_BIT) > 0;
   state->pcbuf_dirty[MESA_SHADER_GEOMETRY] |= (stage_flags & VK_SHADER_STAGE_GEOMETRY_BIT) > 0;
   state->pcbuf_dirty[MESA_SHADER_TESS_CTRL] |= (stage_flags & VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT) > 0;
   state->pcbuf_dirty[MESA_SHADER_TESS_EVAL] |= (stage_flags & VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT) > 0;
   state->pcbuf_dirty[MESA_SHADER_COMPUTE] |= (stage_flags & VK_SHADER_STAGE_COMPUTE_BIT) > 0;
   state->pcbuf_dirty[MESA_SHADER_TASK] |= (stage_flags & VK_SHADER_STAGE_TASK_BIT_EXT) > 0;
   state->pcbuf_dirty[MESA_SHADER_MESH] |= (stage_flags & VK_SHADER_STAGE_MESH_BIT_EXT) > 0;
   state->inlines_dirty[MESA_SHADER_VERTEX] |= (stage_flags & VK_SHADER_STAGE_VERTEX_BIT) > 0;
   state->inlines_dirty[MESA_SHADER_FRAGMENT] |= (stage_flags & VK_SHADER_STAGE_FRAGMENT_BIT) > 0;
   state->inlines_dirty[MESA_SHADER_GEOMETRY] |= (stage_flags & VK_SHADER_STAGE_GEOMETRY_BIT) > 0;
   state->inlines_dirty[MESA_SHADER_TESS_CTRL] |= (stage_flags & VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT) > 0;
   state->inlines_dirty[MESA_SHADER_TESS_EVAL] |= (stage_flags & VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT) > 0;
   state->inlines_dirty[MESA_SHADER_COMPUTE] |= (stage_flags & VK_SHADER_STAGE_COMPUTE_BIT) > 0;
   state->inlines_dirty[MESA_SHADER_TASK] |= (stage_flags & VK_SHADER_STAGE_TASK_BIT_EXT) > 0;
      }
      static void lvp_execute_cmd_buffer(struct list_head *cmds,
            static void handle_execute_commands(struct vk_cmd_queue_entry *cmd,
         {
      for (unsigned i = 0; i < cmd->u.execute_commands.command_buffer_count; i++) {
      LVP_FROM_HANDLE(lvp_cmd_buffer, secondary_buf, cmd->u.execute_commands.command_buffers[i]);
         }
      static void handle_event_set2(struct vk_cmd_queue_entry *cmd,
         {
                        for (uint32_t i = 0; i < cmd->u.set_event2.dependency_info->memoryBarrierCount; i++)
         for (uint32_t i = 0; i < cmd->u.set_event2.dependency_info->bufferMemoryBarrierCount; i++)
         for (uint32_t i = 0; i < cmd->u.set_event2.dependency_info->imageMemoryBarrierCount; i++)
            if (src_stage_mask & VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT)
            }
      static void handle_event_reset2(struct vk_cmd_queue_entry *cmd,
         {
               if (cmd->u.reset_event2.stage_mask == VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT)
            }
      static void handle_wait_events2(struct vk_cmd_queue_entry *cmd,
         {
      finish_fence(state);
   for (unsigned i = 0; i < cmd->u.wait_events2.event_count; i++) {
                     }
      static void handle_pipeline_barrier(struct vk_cmd_queue_entry *cmd,
         {
         }
      static void handle_begin_query(struct vk_cmd_queue_entry *cmd,
         {
      struct vk_cmd_begin_query *qcmd = &cmd->u.begin_query;
            if (pool->type == VK_QUERY_TYPE_PIPELINE_STATISTICS &&
      pool->pipeline_stats & VK_QUERY_PIPELINE_STATISTIC_COMPUTE_SHADER_INVOCATIONS_BIT)
                  uint32_t count = util_bitcount(state->info.view_mask ? state->info.view_mask : BITFIELD_BIT(0));
   for (unsigned idx = 0; idx < count; idx++) {
      if (!pool->queries[qcmd->query + idx]) {
      enum pipe_query_type qtype = pool->base_type;
   pool->queries[qcmd->query + idx] = state->pctx->create_query(state->pctx,
               state->pctx->begin_query(state->pctx, pool->queries[qcmd->query + idx]);
   if (idx)
         }
      static void handle_end_query(struct vk_cmd_queue_entry *cmd,
         {
      struct vk_cmd_end_query *qcmd = &cmd->u.end_query;
   LVP_FROM_HANDLE(lvp_query_pool, pool, qcmd->query_pool);
               }
         static void handle_begin_query_indexed_ext(struct vk_cmd_queue_entry *cmd,
         {
      struct vk_cmd_begin_query_indexed_ext *qcmd = &cmd->u.begin_query_indexed_ext;
            if (pool->type == VK_QUERY_TYPE_PIPELINE_STATISTICS &&
      pool->pipeline_stats & VK_QUERY_PIPELINE_STATISTIC_COMPUTE_SHADER_INVOCATIONS_BIT)
                  uint32_t count = util_bitcount(state->info.view_mask ? state->info.view_mask : BITFIELD_BIT(0));
   for (unsigned idx = 0; idx < count; idx++) {
      if (!pool->queries[qcmd->query + idx]) {
      enum pipe_query_type qtype = pool->base_type;
   pool->queries[qcmd->query + idx] = state->pctx->create_query(state->pctx,
               state->pctx->begin_query(state->pctx, pool->queries[qcmd->query + idx]);
   if (idx)
         }
      static void handle_end_query_indexed_ext(struct vk_cmd_queue_entry *cmd,
         {
      struct vk_cmd_end_query_indexed_ext *qcmd = &cmd->u.end_query_indexed_ext;
   LVP_FROM_HANDLE(lvp_query_pool, pool, qcmd->query_pool);
               }
      static void handle_reset_query_pool(struct vk_cmd_queue_entry *cmd,
         {
      struct vk_cmd_reset_query_pool *qcmd = &cmd->u.reset_query_pool;
   LVP_FROM_HANDLE(lvp_query_pool, pool, qcmd->query_pool);
   for (unsigned i = qcmd->first_query; i < qcmd->first_query + qcmd->query_count; i++) {
      if (pool->queries[i]) {
      state->pctx->destroy_query(state->pctx, pool->queries[i]);
            }
      static void handle_write_timestamp2(struct vk_cmd_queue_entry *cmd,
         {
      struct vk_cmd_write_timestamp2 *qcmd = &cmd->u.write_timestamp2;
            if (!(qcmd->stage == VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT))
            uint32_t count = util_bitcount(state->info.view_mask ? state->info.view_mask : BITFIELD_BIT(0));
   for (unsigned idx = 0; idx < count; idx++) {
      if (!pool->queries[qcmd->query + idx]) {
                        }
      static void handle_copy_query_pool_results(struct vk_cmd_queue_entry *cmd,
         {
      struct vk_cmd_copy_query_pool_results *copycmd = &cmd->u.copy_query_pool_results;
   LVP_FROM_HANDLE(lvp_query_pool, pool, copycmd->query_pool);
            if (copycmd->flags & VK_QUERY_RESULT_PARTIAL_BIT)
         unsigned result_size = copycmd->flags & VK_QUERY_RESULT_64_BIT ? 8 : 4;
   for (unsigned i = copycmd->first_query; i < copycmd->first_query + copycmd->query_count; i++) {
      unsigned offset = copycmd->dst_offset + (copycmd->stride * (i - copycmd->first_query));
   if (pool->queries[i]) {
      unsigned num_results = 0;
   if (copycmd->flags & VK_QUERY_RESULT_WITH_AVAILABILITY_BIT) {
      if (pool->type == VK_QUERY_TYPE_PIPELINE_STATISTICS) {
         } else
         state->pctx->get_query_result_resource(state->pctx,
                                    }
   if (pool->type == VK_QUERY_TYPE_PIPELINE_STATISTICS) {
      num_results = 0;
   u_foreach_bit(bit, pool->pipeline_stats)
      state->pctx->get_query_result_resource(state->pctx,
                                 } else {
      state->pctx->get_query_result_resource(state->pctx,
                                       } else {
      /* if no queries emitted yet, just reset the buffer to 0 so avail is reported correctly */
   if (copycmd->flags & VK_QUERY_RESULT_WITH_AVAILABILITY_BIT) {
                     struct pipe_box box = {0};
   box.x = offset;
   box.width = copycmd->stride;
   box.height = 1;
   box.depth = 1;
   map = state->pctx->buffer_map(state->pctx,
                  memset(map, 0, box.width);
               }
      static void handle_clear_color_image(struct vk_cmd_queue_entry *cmd,
         {
      LVP_FROM_HANDLE(lvp_image, image, cmd->u.clear_color_image.image);
   union util_color uc;
   uint32_t *col_val = uc.ui;
   util_pack_color_union(image->planes[0].bo->format, &uc, (void*)cmd->u.clear_color_image.color);
   for (unsigned i = 0; i < cmd->u.clear_color_image.range_count; i++) {
      VkImageSubresourceRange *range = &cmd->u.clear_color_image.ranges[i];
   struct pipe_box box;
   box.x = 0;
   box.y = 0;
            uint32_t level_count = vk_image_subresource_level_count(&image->vk, range);
   for (unsigned j = range->baseMipLevel; j < range->baseMipLevel + level_count; j++) {
      box.width = u_minify(image->planes[0].bo->width0, j);
   box.height = u_minify(image->planes[0].bo->height0, j);
   box.depth = 1;
   if (image->planes[0].bo->target == PIPE_TEXTURE_3D) {
         } else if (image->planes[0].bo->target == PIPE_TEXTURE_1D_ARRAY) {
      box.y = range->baseArrayLayer;
   box.height = vk_image_subresource_layer_count(&image->vk, range);
      } else {
      box.z = range->baseArrayLayer;
               state->pctx->clear_texture(state->pctx, image->planes[0].bo,
            }
      static void handle_clear_ds_image(struct vk_cmd_queue_entry *cmd,
         {
      LVP_FROM_HANDLE(lvp_image, image, cmd->u.clear_depth_stencil_image.image);
   for (unsigned i = 0; i < cmd->u.clear_depth_stencil_image.range_count; i++) {
      VkImageSubresourceRange *range = &cmd->u.clear_depth_stencil_image.ranges[i];
   uint32_t ds_clear_flags = 0;
   if (range->aspectMask & VK_IMAGE_ASPECT_DEPTH_BIT)
         if (range->aspectMask & VK_IMAGE_ASPECT_STENCIL_BIT)
            uint32_t level_count = vk_image_subresource_level_count(&image->vk, range);
   for (unsigned j = 0; j < level_count; j++) {
      struct pipe_surface *surf;
   unsigned width, height, depth;
                  if (image->planes[0].bo->target == PIPE_TEXTURE_3D) {
         } else {
                  surf = create_img_surface_bo(state, range,
                        state->pctx->clear_depth_stencil(state->pctx,
                                                }
      static void handle_clear_attachments(struct vk_cmd_queue_entry *cmd,
         {
      for (uint32_t a = 0; a < cmd->u.clear_attachments.attachment_count; a++) {
      VkClearAttachment *att = &cmd->u.clear_attachments.attachments[a];
            if (att->aspectMask == VK_IMAGE_ASPECT_COLOR_BIT) {
         } else {
         }
   if (!imgv)
            union pipe_color_union col_val;
   double dclear_val = 0;
   uint32_t sclear_val = 0;
   uint32_t ds_clear_flags = 0;
   if (att->aspectMask & VK_IMAGE_ASPECT_DEPTH_BIT) {
      ds_clear_flags |= PIPE_CLEAR_DEPTH;
      }
   if (att->aspectMask & VK_IMAGE_ASPECT_STENCIL_BIT) {
      ds_clear_flags |= PIPE_CLEAR_STENCIL;
      }
   if (att->aspectMask & VK_IMAGE_ASPECT_COLOR_BIT) {
      for (unsigned i = 0; i < 4; i++)
                           VkClearRect *rect = &cmd->u.clear_attachments.rects[r];
   /* avoid crashing on spec violations */
   rect->rect.offset.x = MAX2(rect->rect.offset.x, 0);
   rect->rect.offset.y = MAX2(rect->rect.offset.y, 0);
   rect->rect.extent.width = MIN2(rect->rect.extent.width, state->framebuffer.width - rect->rect.offset.x);
   rect->rect.extent.height = MIN2(rect->rect.extent.height, state->framebuffer.height - rect->rect.offset.y);
   if (state->info.view_mask) {
      u_foreach_bit(i, state->info.view_mask)
      clear_attachment_layers(state, imgv, &rect->rect,
               } else
      clear_attachment_layers(state, imgv, &rect->rect,
                     }
      static void handle_resolve_image(struct vk_cmd_queue_entry *cmd,
         {
      VkResolveImageInfo2 *resolvecmd = cmd->u.resolve_image2.resolve_image_info;
   LVP_FROM_HANDLE(lvp_image, src_image, resolvecmd->srcImage);
            struct pipe_blit_info info = {0};
   info.src.resource = src_image->planes[0].bo;
   info.dst.resource = dst_image->planes[0].bo;
   info.src.format = src_image->planes[0].bo->format;
   info.dst.format = dst_image->planes[0].bo->format;
   info.mask = util_format_is_depth_or_stencil(info.src.format) ? PIPE_MASK_ZS : PIPE_MASK_RGBA;
            for (uint32_t i = 0; i < resolvecmd->regionCount; i++) {
      int srcX0, srcY0;
            srcX0 = resolvecmd->pRegions[i].srcOffset.x;
            dstX0 = resolvecmd->pRegions[i].dstOffset.x;
            info.dst.box.x = dstX0;
   info.dst.box.y = dstY0;
   info.src.box.x = srcX0;
            info.dst.box.width = resolvecmd->pRegions[i].extent.width;
   info.src.box.width = resolvecmd->pRegions[i].extent.width;
   info.dst.box.height = resolvecmd->pRegions[i].extent.height;
            info.dst.box.depth = subresource_layercount(dst_image, &resolvecmd->pRegions[i].dstSubresource);
            info.src.level = resolvecmd->pRegions[i].srcSubresource.mipLevel;
            info.dst.level = resolvecmd->pRegions[i].dstSubresource.mipLevel;
                  }
      static void handle_draw_indirect_count(struct vk_cmd_queue_entry *cmd,
         {
      struct pipe_draw_start_count_bias draw = {0};
   struct pipe_resource *index = NULL;
   if (indexed) {
      state->info.index_bounds_valid = false;
   state->info.index_size = state->index_size;
   state->info.index.resource = state->index_buffer;
   state->info.max_index = ~0U;
   if (state->index_offset || state->index_buffer_size != UINT32_MAX) {
      struct pipe_transfer *xfer;
   uint8_t *mem = pipe_buffer_map(state->pctx, state->index_buffer, 0, &xfer);
   state->pctx->buffer_unmap(state->pctx, xfer);
   index = get_buffer_resource(state->pctx, mem + state->index_offset);
   index->width0 = MIN2(state->index_buffer->width0 - state->index_offset, state->index_buffer_size);
         } else
         state->indirect_info.offset = cmd->u.draw_indirect_count.offset;
   state->indirect_info.stride = cmd->u.draw_indirect_count.stride;
   state->indirect_info.draw_count = cmd->u.draw_indirect_count.max_draw_count;
   state->indirect_info.buffer = lvp_buffer_from_handle(cmd->u.draw_indirect_count.buffer)->bo;
   state->indirect_info.indirect_draw_count_offset = cmd->u.draw_indirect_count.count_buffer_offset;
            state->pctx->draw_vbo(state->pctx, &state->info, 0, &state->indirect_info, &draw, 1);
      }
      static void handle_push_descriptor_set(struct vk_cmd_queue_entry *cmd,
         {
      struct vk_cmd_push_descriptor_set_khr *pds = &cmd->u.push_descriptor_set_khr;
   LVP_FROM_HANDLE(lvp_pipeline_layout, layout, pds->layout);
            struct lvp_descriptor_set *set;
                     struct lvp_descriptor_set *base = state->desc_sets[lvp_pipeline_type_from_bind_point(pds->pipeline_bind_point)][pds->set];
   if (base)
            VkDescriptorSet set_handle = lvp_descriptor_set_to_handle(set);
   for (uint32_t i = 0; i < pds->descriptor_write_count; i++)
                     struct vk_cmd_queue_entry bind_cmd;
   bind_cmd.u.bind_descriptor_sets = (struct vk_cmd_bind_descriptor_sets){
      .pipeline_bind_point = pds->pipeline_bind_point,
   .layout = pds->layout,
   .first_set = pds->set,
   .descriptor_set_count = 1,
      };
      }
      static void handle_push_descriptor_set_with_template(struct vk_cmd_queue_entry *cmd,
         {
      struct vk_cmd_push_descriptor_set_with_template_khr *pds = &cmd->u.push_descriptor_set_with_template_khr;
   LVP_FROM_HANDLE(lvp_descriptor_update_template, templ, pds->descriptor_update_template);
   LVP_FROM_HANDLE(lvp_pipeline_layout, layout, pds->layout);
            struct lvp_descriptor_set *set;
                     struct lvp_descriptor_set *base = state->desc_sets[lvp_pipeline_type_from_bind_point(templ->bind_point)][pds->set];
   if (base)
            VkDescriptorSet set_handle = lvp_descriptor_set_to_handle(set);
   lvp_descriptor_set_update_with_template(lvp_device_to_handle(state->device), set_handle,
            struct vk_cmd_queue_entry bind_cmd;
   bind_cmd.u.bind_descriptor_sets = (struct vk_cmd_bind_descriptor_sets){
      .pipeline_bind_point = templ->bind_point,
   .layout = pds->layout,
   .first_set = pds->set,
   .descriptor_set_count = 1,
      };
      }
      static void handle_bind_transform_feedback_buffers(struct vk_cmd_queue_entry *cmd,
         {
               for (unsigned i = 0; i < btfb->binding_count; i++) {
      int idx = i + btfb->first_binding;
   uint32_t size;
                     if (state->so_targets[idx])
            state->so_targets[idx] = state->pctx->create_stream_output_target(state->pctx,
                  }
      }
      static void handle_begin_transform_feedback(struct vk_cmd_queue_entry *cmd,
         {
      struct vk_cmd_begin_transform_feedback_ext *btf = &cmd->u.begin_transform_feedback_ext;
            for (unsigned i = 0; btf->counter_buffers && i < btf->counter_buffer_count; i++) {
      if (!btf->counter_buffers[i])
            pipe_buffer_read(state->pctx,
                  btf->counter_buffers ? lvp_buffer_from_handle(btf->counter_buffers[i])->bo : NULL,
   }
   state->pctx->set_stream_output_targets(state->pctx, state->num_so_targets,
      }
      static void handle_end_transform_feedback(struct vk_cmd_queue_entry *cmd,
         {
               if (etf->counter_buffer_count) {
      for (unsigned i = 0; etf->counter_buffers && i < etf->counter_buffer_count; i++) {
                                    pipe_buffer_write(state->pctx,
                           }
      }
      static void handle_draw_indirect_byte_count(struct vk_cmd_queue_entry *cmd,
         {
      struct vk_cmd_draw_indirect_byte_count_ext *dibc = &cmd->u.draw_indirect_byte_count_ext;
            pipe_buffer_read(state->pctx,
                        state->info.start_instance = cmd->u.draw_indirect_byte_count_ext.first_instance;
   state->info.instance_count = cmd->u.draw_indirect_byte_count_ext.instance_count;
            draw.count /= cmd->u.draw_indirect_byte_count_ext.vertex_stride;
      }
      static void handle_begin_conditional_rendering(struct vk_cmd_queue_entry *cmd,
         {
      struct VkConditionalRenderingBeginInfoEXT *bcr = cmd->u.begin_conditional_rendering_ext.conditional_rendering_begin;
   state->render_cond = true;
   state->pctx->render_condition_mem(state->pctx,
                  }
      static void handle_end_conditional_rendering(struct rendering_state *state)
   {
      state->render_cond = false;
      }
      static void handle_set_vertex_input(struct vk_cmd_queue_entry *cmd,
         {
      const struct vk_cmd_set_vertex_input_ext *vertex_input = &cmd->u.set_vertex_input_ext;
   const struct VkVertexInputBindingDescription2EXT *bindings = vertex_input->vertex_binding_descriptions;
   const struct VkVertexInputAttributeDescription2EXT *attrs = vertex_input->vertex_attribute_descriptions;
   int max_location = -1;
   for (unsigned i = 0; i < vertex_input->vertex_attribute_description_count; i++) {
      const struct VkVertexInputBindingDescription2EXT *binding = NULL;
            for (unsigned j = 0; j < vertex_input->vertex_binding_description_count; j++) {
      const struct VkVertexInputBindingDescription2EXT *b = &bindings[j];
   if (b->binding == attrs[i].binding) {
      binding = b;
         }
   assert(binding);
   state->velem.velems[location].src_offset = attrs[i].offset;
   state->vertex_buffer_index[location] = attrs[i].binding;
   state->velem.velems[location].src_format = lvp_vk_format_to_pipe_format(attrs[i].format);
   state->velem.velems[location].src_stride = binding->stride;
   uint32_t d = binding->divisor;
   switch (binding->inputRate) {
   case VK_VERTEX_INPUT_RATE_VERTEX:
      state->velem.velems[location].instance_divisor = 0;
      case VK_VERTEX_INPUT_RATE_INSTANCE:
      state->velem.velems[location].instance_divisor = d ? d : UINT32_MAX;
      default:
      assert(0);
               if ((int)location > max_location)
      }
   state->velem.count = max_location + 1;
   state->vb_strides_dirty = false;
   state->vb_dirty = true;
      }
      static void handle_set_cull_mode(struct vk_cmd_queue_entry *cmd,
         {
      state->rs_state.cull_face = vk_cull_to_pipe(cmd->u.set_cull_mode.cull_mode);
      }
      static void handle_set_front_face(struct vk_cmd_queue_entry *cmd,
         {
      state->rs_state.front_ccw = (cmd->u.set_front_face.front_face == VK_FRONT_FACE_COUNTER_CLOCKWISE);
      }
      static void handle_set_primitive_topology(struct vk_cmd_queue_entry *cmd,
         {
      state->info.mode = vk_conv_topology(cmd->u.set_primitive_topology.primitive_topology);
      }
      static void handle_set_depth_test_enable(struct vk_cmd_queue_entry *cmd,
         {
      state->dsa_dirty |= state->dsa_state.depth_enabled != cmd->u.set_depth_test_enable.depth_test_enable;
      }
      static void handle_set_depth_write_enable(struct vk_cmd_queue_entry *cmd,
         {
      state->dsa_dirty |= state->dsa_state.depth_writemask != cmd->u.set_depth_write_enable.depth_write_enable;
      }
      static void handle_set_depth_compare_op(struct vk_cmd_queue_entry *cmd,
         {
      state->dsa_dirty |= state->dsa_state.depth_func != cmd->u.set_depth_compare_op.depth_compare_op;
      }
      static void handle_set_depth_bounds_test_enable(struct vk_cmd_queue_entry *cmd,
         {
      state->dsa_dirty |= state->dsa_state.depth_bounds_test != cmd->u.set_depth_bounds_test_enable.depth_bounds_test_enable;
      }
      static void handle_set_stencil_test_enable(struct vk_cmd_queue_entry *cmd,
         {
      state->dsa_dirty |= state->dsa_state.stencil[0].enabled != cmd->u.set_stencil_test_enable.stencil_test_enable ||
         state->dsa_state.stencil[0].enabled = cmd->u.set_stencil_test_enable.stencil_test_enable;
      }
      static void handle_set_stencil_op(struct vk_cmd_queue_entry *cmd,
         {
      if (cmd->u.set_stencil_op.face_mask & VK_STENCIL_FACE_FRONT_BIT) {
      state->dsa_state.stencil[0].func = cmd->u.set_stencil_op.compare_op;
   state->dsa_state.stencil[0].fail_op = vk_conv_stencil_op(cmd->u.set_stencil_op.fail_op);
   state->dsa_state.stencil[0].zpass_op = vk_conv_stencil_op(cmd->u.set_stencil_op.pass_op);
               if (cmd->u.set_stencil_op.face_mask & VK_STENCIL_FACE_BACK_BIT) {
      state->dsa_state.stencil[1].func = cmd->u.set_stencil_op.compare_op;
   state->dsa_state.stencil[1].fail_op = vk_conv_stencil_op(cmd->u.set_stencil_op.fail_op);
   state->dsa_state.stencil[1].zpass_op = vk_conv_stencil_op(cmd->u.set_stencil_op.pass_op);
      }
      }
      static void handle_set_line_stipple(struct vk_cmd_queue_entry *cmd,
         {
      state->rs_state.line_stipple_factor = cmd->u.set_line_stipple_ext.line_stipple_factor - 1;
   state->rs_state.line_stipple_pattern = cmd->u.set_line_stipple_ext.line_stipple_pattern;
      }
      static void handle_set_depth_bias_enable(struct vk_cmd_queue_entry *cmd,
         {
      state->rs_dirty |= state->depth_bias.enabled != cmd->u.set_depth_bias_enable.depth_bias_enable;
      }
      static void handle_set_logic_op(struct vk_cmd_queue_entry *cmd,
         {
      unsigned op = vk_logic_op_to_pipe(cmd->u.set_logic_op_ext.logic_op);
   state->rs_dirty |= state->blend_state.logicop_func != op;
      }
      static void handle_set_patch_control_points(struct vk_cmd_queue_entry *cmd,
         {
      if (state->patch_vertices != cmd->u.set_patch_control_points_ext.patch_control_points)
            }
      static void handle_set_primitive_restart_enable(struct vk_cmd_queue_entry *cmd,
         {
         }
      static void handle_set_rasterizer_discard_enable(struct vk_cmd_queue_entry *cmd,
         {
      state->rs_dirty |= state->rs_state.rasterizer_discard != cmd->u.set_rasterizer_discard_enable.rasterizer_discard_enable;
      }
      static void handle_set_color_write_enable(struct vk_cmd_queue_entry *cmd,
         {
               for (unsigned i = 0; i < cmd->u.set_color_write_enable_ext.attachment_count; i++) {
      /* this is inverted because cmdbufs are zero-initialized, meaning only 'true'
   * can be detected with a bool, and the default is to enable color writes
   */
   if (cmd->u.set_color_write_enable_ext.color_write_enables[i] != VK_TRUE)
               state->blend_dirty |= state->color_write_disables != disable_mask;
      }
      static void handle_set_polygon_mode(struct vk_cmd_queue_entry *cmd,
         {
      unsigned polygon_mode = vk_polygon_mode_to_pipe(cmd->u.set_polygon_mode_ext.polygon_mode);
   if (state->rs_state.fill_front != polygon_mode)
         state->rs_state.fill_front = polygon_mode;
   if (state->rs_state.fill_back != polygon_mode)
            }
      static void handle_set_tessellation_domain_origin(struct vk_cmd_queue_entry *cmd,
         {
      bool tess_ccw = cmd->u.set_tessellation_domain_origin_ext.domain_origin == VK_TESSELLATION_DOMAIN_ORIGIN_UPPER_LEFT;
   if (tess_ccw == state->tess_ccw)
         state->tess_ccw = tess_ccw;
   if (state->tess_states[state->tess_ccw])
      }
      static void handle_set_depth_clamp_enable(struct vk_cmd_queue_entry *cmd,
         {
      state->rs_dirty |= state->rs_state.depth_clamp != cmd->u.set_depth_clamp_enable_ext.depth_clamp_enable;
   state->rs_state.depth_clamp = !!cmd->u.set_depth_clamp_enable_ext.depth_clamp_enable;
   if (state->depth_clamp_sets_clip)
      }
      static void handle_set_depth_clip_enable(struct vk_cmd_queue_entry *cmd,
         {
      state->rs_dirty |= state->rs_state.depth_clip_far != !!cmd->u.set_depth_clip_enable_ext.depth_clip_enable;
      }
      static void handle_set_logic_op_enable(struct vk_cmd_queue_entry *cmd,
         {
      state->blend_dirty |= state->blend_state.logicop_enable != !!cmd->u.set_logic_op_enable_ext.logic_op_enable;
      }
      static void handle_set_sample_mask(struct vk_cmd_queue_entry *cmd,
         {
      unsigned mask = cmd->u.set_sample_mask_ext.sample_mask ? cmd->u.set_sample_mask_ext.sample_mask[0] : 0xffffffff;
   state->sample_mask_dirty |= state->sample_mask != mask;
      }
      static void handle_set_samples(struct vk_cmd_queue_entry *cmd,
         {
         }
      static void handle_set_alpha_to_coverage(struct vk_cmd_queue_entry *cmd,
         {
      state->blend_dirty |=
            }
      static void handle_set_alpha_to_one(struct vk_cmd_queue_entry *cmd,
         {
      state->blend_dirty |=
         state->blend_state.alpha_to_one = !!cmd->u.set_alpha_to_one_enable_ext.alpha_to_one_enable;
   if (state->blend_state.alpha_to_one)
      }
      static void handle_set_halfz(struct vk_cmd_queue_entry *cmd,
         {
      if (state->rs_state.clip_halfz == !cmd->u.set_depth_clip_negative_one_to_one_ext.negative_one_to_one)
         state->rs_dirty = true;
   state->rs_state.clip_halfz = !cmd->u.set_depth_clip_negative_one_to_one_ext.negative_one_to_one;
   /* handle dynamic state: convert from one transform to the other */
   for (unsigned i = 0; i < state->num_viewports; i++)
            }
      static void handle_set_line_rasterization_mode(struct vk_cmd_queue_entry *cmd,
         {
      VkLineRasterizationModeEXT lineRasterizationMode = cmd->u.set_line_rasterization_mode_ext.line_rasterization_mode;
   /* not even going to bother trying dirty tracking on this */
   state->rs_dirty = true;
   state->rs_state.line_smooth = lineRasterizationMode == VK_LINE_RASTERIZATION_MODE_RECTANGULAR_SMOOTH_EXT;
   state->rs_state.line_rectangular = lineRasterizationMode != VK_LINE_RASTERIZATION_MODE_BRESENHAM_EXT;;
   state->disable_multisample = lineRasterizationMode == VK_LINE_RASTERIZATION_MODE_BRESENHAM_EXT ||
      }
      static void handle_set_line_stipple_enable(struct vk_cmd_queue_entry *cmd,
         {
      state->rs_dirty |= state->rs_state.line_stipple_enable != !!cmd->u.set_line_stipple_enable_ext.stippled_line_enable;
      }
      static void handle_set_provoking_vertex_mode(struct vk_cmd_queue_entry *cmd,
         {
      bool flatshade_first = cmd->u.set_provoking_vertex_mode_ext.provoking_vertex_mode != VK_PROVOKING_VERTEX_MODE_LAST_VERTEX_EXT;
   state->rs_dirty |= state->rs_state.flatshade_first != flatshade_first;
      }
      static void handle_set_color_blend_enable(struct vk_cmd_queue_entry *cmd,
         {
      for (unsigned i = 0; i < cmd->u.set_color_blend_enable_ext.attachment_count; i++) {
      if (state->blend_state.rt[cmd->u.set_color_blend_enable_ext.first_attachment + i].blend_enable != !!cmd->u.set_color_blend_enable_ext.color_blend_enables[i]) {
         }
         }
      static void handle_set_color_write_mask(struct vk_cmd_queue_entry *cmd,
         {
      for (unsigned i = 0; i < cmd->u.set_color_write_mask_ext.attachment_count; i++) {
      if (state->blend_state.rt[cmd->u.set_color_write_mask_ext.first_attachment + i].colormask != cmd->u.set_color_write_mask_ext.color_write_masks[i])
               }
      static void handle_set_color_blend_equation(struct vk_cmd_queue_entry *cmd,
         {
      const VkColorBlendEquationEXT *cb = cmd->u.set_color_blend_equation_ext.color_blend_equations;
   state->blend_dirty = true;
   for (unsigned i = 0; i < cmd->u.set_color_blend_equation_ext.attachment_count; i++) {
      state->blend_state.rt[cmd->u.set_color_blend_equation_ext.first_attachment + i].rgb_func = vk_blend_op_to_pipe(cb[i].colorBlendOp);
   state->blend_state.rt[cmd->u.set_color_blend_equation_ext.first_attachment + i].rgb_src_factor = vk_blend_factor_to_pipe(cb[i].srcColorBlendFactor);
   state->blend_state.rt[cmd->u.set_color_blend_equation_ext.first_attachment + i].rgb_dst_factor = vk_blend_factor_to_pipe(cb[i].dstColorBlendFactor);
   state->blend_state.rt[cmd->u.set_color_blend_equation_ext.first_attachment + i].alpha_func = vk_blend_op_to_pipe(cb[i].alphaBlendOp);
   state->blend_state.rt[cmd->u.set_color_blend_equation_ext.first_attachment + i].alpha_src_factor = vk_blend_factor_to_pipe(cb[i].srcAlphaBlendFactor);
            /* At least llvmpipe applies the blend factor prior to the blend function,
   * regardless of what function is used. (like i965 hardware).
   * It means for MIN/MAX the blend factor has to be stomped to ONE.
   */
   if (cb[i].colorBlendOp == VK_BLEND_OP_MIN ||
      cb[i].colorBlendOp == VK_BLEND_OP_MAX) {
   state->blend_state.rt[cmd->u.set_color_blend_equation_ext.first_attachment + i].rgb_src_factor = PIPE_BLENDFACTOR_ONE;
               if (cb[i].alphaBlendOp == VK_BLEND_OP_MIN ||
      cb[i].alphaBlendOp == VK_BLEND_OP_MAX) {
   state->blend_state.rt[cmd->u.set_color_blend_equation_ext.first_attachment + i].alpha_src_factor = PIPE_BLENDFACTOR_ONE;
            }
      static void
   handle_shaders(struct vk_cmd_queue_entry *cmd, struct rendering_state *state)
   {
               bool gfx = false;
   VkShaderStageFlagBits vkstages = 0;
   unsigned new_stages = 0;
   unsigned null_stages = 0;
   for (unsigned i = 0; i < bind->stage_count; i++) {
      gl_shader_stage stage = vk_to_mesa_shader_stage(bind->stages[i]);
   assert(stage != MESA_SHADER_NONE && stage <= MESA_SHADER_MESH);
   LVP_FROM_HANDLE(lvp_shader, shader, bind->shaders ? bind->shaders[i] : VK_NULL_HANDLE);
   if (stage == MESA_SHADER_FRAGMENT) {
      if (shader) {
      state->force_min_sample = shader->pipeline_nir->nir->info.fs.uses_sample_shading;
   state->sample_shading = state->force_min_sample;
      } else {
      state->force_min_sample = false;
         }
   if (shader) {
      vkstages |= bind->stages[i];
   new_stages |= BITFIELD_BIT(stage);
      } else {
      if (state->shaders[stage])
               if (stage != MESA_SHADER_COMPUTE) {
      state->gfx_push_sizes[stage] = shader ? shader->layout->push_constant_size : 0;
      } else {
                     if ((new_stages | null_stages) & LVP_STAGE_MASK_GFX) {
      VkShaderStageFlags all_gfx = VK_SHADER_STAGE_ALL_GRAPHICS | VK_SHADER_STAGE_MESH_BIT_EXT | VK_SHADER_STAGE_TASK_BIT_EXT;
   unbind_graphics_stages(state, null_stages & all_gfx);
   handle_graphics_stages(state, vkstages & all_gfx, true);
   u_foreach_bit(i, new_stages) {
            }
   /* ignore compute unbinds */
   if (new_stages & BITFIELD_BIT(MESA_SHADER_COMPUTE)) {
                  if (gfx) {
      state->push_size[0] = 0;
   for (unsigned i = 0; i < ARRAY_SIZE(state->gfx_push_sizes); i++)
         }
      static void handle_draw_mesh_tasks(struct vk_cmd_queue_entry *cmd,
         {
      state->dispatch_info.grid[0] = cmd->u.draw_mesh_tasks_ext.group_count_x;
   state->dispatch_info.grid[1] = cmd->u.draw_mesh_tasks_ext.group_count_y;
   state->dispatch_info.grid[2] = cmd->u.draw_mesh_tasks_ext.group_count_z;
   state->dispatch_info.grid_base[0] = 0;
   state->dispatch_info.grid_base[1] = 0;
   state->dispatch_info.grid_base[2] = 0;
   state->dispatch_info.draw_count = 1;
   state->dispatch_info.indirect = NULL;
      }
      static void handle_draw_mesh_tasks_indirect(struct vk_cmd_queue_entry *cmd,
         {
      state->dispatch_info.indirect = lvp_buffer_from_handle(cmd->u.draw_mesh_tasks_indirect_ext.buffer)->bo;
   state->dispatch_info.indirect_offset = cmd->u.draw_mesh_tasks_indirect_ext.offset;
   state->dispatch_info.indirect_stride = cmd->u.draw_mesh_tasks_indirect_ext.stride;
   state->dispatch_info.draw_count = cmd->u.draw_mesh_tasks_indirect_ext.draw_count;
      }
      static void handle_draw_mesh_tasks_indirect_count(struct vk_cmd_queue_entry *cmd,
         {
      state->dispatch_info.indirect = lvp_buffer_from_handle(cmd->u.draw_mesh_tasks_indirect_count_ext.buffer)->bo;
   state->dispatch_info.indirect_offset = cmd->u.draw_mesh_tasks_indirect_count_ext.offset;
   state->dispatch_info.indirect_stride = cmd->u.draw_mesh_tasks_indirect_count_ext.stride;
   state->dispatch_info.draw_count = cmd->u.draw_mesh_tasks_indirect_count_ext.max_draw_count;
   state->dispatch_info.indirect_draw_count_offset = cmd->u.draw_mesh_tasks_indirect_count_ext.count_buffer_offset;
   state->dispatch_info.indirect_draw_count = lvp_buffer_from_handle(cmd->u.draw_mesh_tasks_indirect_count_ext.count_buffer)->bo;
      }
      static VkBuffer
   get_buffer(struct rendering_state *state, uint8_t *ptr, size_t *offset)
   {
      simple_mtx_lock(&state->device->bda_lock);
   hash_table_foreach(&state->device->bda, he) {
      const uint8_t *bda = he->key;
   if (ptr < bda)
         struct lvp_buffer *buffer = he->data;
   if (bda + buffer->vk.size > ptr) {
      *offset = ptr - bda;
   simple_mtx_unlock(&state->device->bda_lock);
         }
   fprintf(stderr, "unrecognized BDA!\n");
      }
      static size_t
   process_sequence(struct rendering_state *state,
                     {
      size_t size = 0;
   for (uint32_t t = 0; t < dlayout->token_count; t++){
      const VkIndirectCommandsLayoutTokenNV *token = &dlayout->tokens[t];
   uint32_t stride = dlayout->stream_strides[token->stream];
   uint8_t *stream = map_streams[token->stream];
   uint32_t offset = stride * seq + token->offset;
   uint32_t draw_offset = offset + pstreams[token->stream].offset;
            struct vk_cmd_queue_entry *cmd = (struct vk_cmd_queue_entry*)(pbuf + size);
   size_t cmd_size = vk_cmd_queue_type_sizes[lvp_nv_dgc_token_to_cmd_type(token)];
            if (max_size < size + cmd_size)
                  switch (token->tokenType) {
   case VK_INDIRECT_COMMANDS_TOKEN_TYPE_SHADER_GROUP_NV: {
      VkBindShaderGroupIndirectCommandNV *bind = input;
   cmd->u.bind_pipeline_shader_group_nv.pipeline_bind_point = VK_PIPELINE_BIND_POINT_GRAPHICS;
   cmd->u.bind_pipeline_shader_group_nv.pipeline = pipeline;
   cmd->u.bind_pipeline_shader_group_nv.group_index = bind->groupIndex;
      }
   case VK_INDIRECT_COMMANDS_TOKEN_TYPE_STATE_FLAGS_NV: {
      VkSetStateFlagsIndirectCommandNV *state = input;
   if (token->indirectStateFlags & VK_INDIRECT_STATE_FLAG_FRONTFACE_BIT_NV) {
      if (state->data & BITFIELD_BIT(VK_FRONT_FACE_CLOCKWISE)) {
         } else {
            } else {
      /* skip this if unrecognized state flag */
      }
      }
   case VK_INDIRECT_COMMANDS_TOKEN_TYPE_PUSH_CONSTANT_NV: {
      uint32_t *data = input;
   cmd_size += token->pushconstantSize;
   if (max_size < size + cmd_size)
         cmd->u.push_constants.layout = token->pushconstantPipelineLayout;
   cmd->u.push_constants.stage_flags = token->pushconstantShaderStageFlags;
   cmd->u.push_constants.offset = token->pushconstantOffset;
   cmd->u.push_constants.size = token->pushconstantSize;
   cmd->u.push_constants.values = (void*)cmdptr;
   memcpy(cmd->u.push_constants.values, data, token->pushconstantSize);
      }
   case VK_INDIRECT_COMMANDS_TOKEN_TYPE_INDEX_BUFFER_NV: {
      VkBindIndexBufferIndirectCommandNV *data = input;
   cmd->u.bind_index_buffer.offset = 0;
   if (data->bufferAddress)
         else
         cmd->u.bind_index_buffer.index_type = data->indexType;
   for (unsigned i = 0; i < token->indexTypeCount; i++) {
      if (data->indexType == token->pIndexTypeValues[i]) {
      cmd->u.bind_index_buffer.index_type = token->pIndexTypes[i];
         }
      }
   case VK_INDIRECT_COMMANDS_TOKEN_TYPE_VERTEX_BUFFER_NV: {
      VkBindVertexBufferIndirectCommandNV *data = input;
   cmd_size += sizeof(*cmd->u.bind_vertex_buffers2.buffers) + sizeof(*cmd->u.bind_vertex_buffers2.offsets);
   cmd_size += sizeof(*cmd->u.bind_vertex_buffers2.sizes) + sizeof(*cmd->u.bind_vertex_buffers2.strides);
                                                                              cmd->u.bind_vertex_buffers2.offsets[0] = 0;
                  if (token->vertexDynamicStride) {
      cmd->u.bind_vertex_buffers2.strides = (void*)(cmdptr + alloc_offset);
      } else {
         }
      }
   case VK_INDIRECT_COMMANDS_TOKEN_TYPE_DRAW_INDEXED_NV: {
      cmd->u.draw_indexed_indirect.buffer = pstreams[token->stream].buffer;
   cmd->u.draw_indexed_indirect.offset = draw_offset;
   cmd->u.draw_indexed_indirect.draw_count = 1;
   cmd->u.draw_indexed_indirect.stride = 0;
      }
   case VK_INDIRECT_COMMANDS_TOKEN_TYPE_DRAW_NV: {
      cmd->u.draw_indirect.buffer = pstreams[token->stream].buffer;
   cmd->u.draw_indirect.offset = draw_offset;
   cmd->u.draw_indirect.draw_count = 1;
   cmd->u.draw_indirect.stride = 0;
      }
   // only available if VK_EXT_mesh_shader is supported
   case VK_INDIRECT_COMMANDS_TOKEN_TYPE_DRAW_MESH_TASKS_NV: {
      cmd->u.draw_mesh_tasks_indirect_ext.buffer = pstreams[token->stream].buffer;
   cmd->u.draw_mesh_tasks_indirect_ext.offset = draw_offset;
   cmd->u.draw_mesh_tasks_indirect_ext.draw_count = 1;
   cmd->u.draw_mesh_tasks_indirect_ext.stride = 0;
      }
   // only available if VK_NV_mesh_shader is supported
   case VK_INDIRECT_COMMANDS_TOKEN_TYPE_DRAW_TASKS_NV:
         default:
      unreachable("unknown token type");
      }
   size += cmd_size;
      }
      }
      static void
   handle_preprocess_generated_commands(struct vk_cmd_queue_entry *cmd, struct rendering_state *state)
   {
      VkGeneratedCommandsInfoNV *pre = cmd->u.preprocess_generated_commands_nv.generated_commands_info;
   VK_FROM_HANDLE(lvp_indirect_command_layout, dlayout, pre->indirectCommandsLayout);
   struct pipe_transfer *stream_maps[16];
   uint8_t *streams[16];
   for (unsigned i = 0; i < pre->streamCount; i++) {
      struct lvp_buffer *buf = lvp_buffer_from_handle(pre->pStreams[i].buffer);
   streams[i] = pipe_buffer_map(state->pctx, buf->bo, PIPE_MAP_READ, &stream_maps[i]);
      }
   LVP_FROM_HANDLE(lvp_buffer, pbuf, pre->preprocessBuffer);
   LVP_FROM_HANDLE(lvp_buffer, seqc, pre->sequencesCountBuffer);
            unsigned seq_count = pre->sequencesCount;
   if (seqc) {
      unsigned count = 0;
   pipe_buffer_read(state->pctx, seqc->bo, pre->sequencesCountOffset, sizeof(uint32_t), &count);
      }
   uint32_t *seq = NULL;
   struct pipe_transfer *seq_map = NULL;
   if (seqi) {
      seq = pipe_buffer_map(state->pctx, seqi->bo, PIPE_MAP_READ, &seq_map);
               struct pipe_transfer *pmap;
   uint8_t *p = pipe_buffer_map(state->pctx, pbuf->bo, PIPE_MAP_WRITE, &pmap);
   p += pre->preprocessOffset;
   struct list_head *list = (void*)p;
   size_t size = sizeof(struct list_head);
   size_t max_size = pre->preprocessSize;
   if (size > max_size)
                  size_t offset = size;
   for (unsigned i = 0; i < seq_count; i++) {
      uint32_t s = seq ? seq[i] : i;
               /* vk_cmd_queue will copy the binary and break the list, so null the tail pointer */
            for (unsigned i = 0; i < pre->streamCount; i++)
         state->pctx->buffer_unmap(state->pctx, pmap);
   if (seq_map)
      }
      static void
   handle_execute_generated_commands(struct vk_cmd_queue_entry *cmd, struct rendering_state *state, bool print_cmds)
   {
      VkGeneratedCommandsInfoNV *gen = cmd->u.execute_generated_commands_nv.generated_commands_info;
   struct vk_cmd_execute_generated_commands_nv *exec = &cmd->u.execute_generated_commands_nv;
   if (!exec->is_preprocessed) {
      struct vk_cmd_queue_entry pre;
   pre.u.preprocess_generated_commands_nv.generated_commands_info = exec->generated_commands_info;
      }
   LVP_FROM_HANDLE(lvp_buffer, pbuf, gen->preprocessBuffer);
   struct pipe_transfer *pmap;
   uint8_t *p = pipe_buffer_map(state->pctx, pbuf->bo, PIPE_MAP_WRITE, &pmap);
   p += gen->preprocessOffset;
                        }
      static void
   handle_descriptor_buffers(struct vk_cmd_queue_entry *cmd, struct rendering_state *state)
   {
      const struct vk_cmd_bind_descriptor_buffers_ext *bind = &cmd->u.bind_descriptor_buffers_ext;
   for (unsigned i = 0; i < bind->buffer_count; i++) {
      struct pipe_resource *pres = get_buffer_resource(state->pctx, (void *)(uintptr_t)bind->binding_infos[i].address);
   state->desc_buffer_addrs[i] = (void *)(uintptr_t)bind->binding_infos[i].address;
   pipe_resource_reference(&state->desc_buffers[i], pres);
   /* leave only one ref on rendering_state */
         }
      static bool
   descriptor_layouts_equal(const struct lvp_descriptor_set_layout *a, const struct lvp_descriptor_set_layout *b)
   {
      const uint8_t *pa = (const uint8_t*)a, *pb = (const uint8_t*)b;
   uint32_t hash_start_offset = sizeof(struct vk_descriptor_set_layout);
   uint32_t binding_offset = offsetof(struct lvp_descriptor_set_layout, binding);
   /* base equal */
   if (memcmp(pa + hash_start_offset, pb + hash_start_offset, binding_offset - hash_start_offset))
            /* bindings equal */
   if (a->binding_count != b->binding_count)
         size_t binding_size = a->binding_count * sizeof(struct lvp_descriptor_set_binding_layout);
   const struct lvp_descriptor_set_binding_layout *la = a->binding;
   const struct lvp_descriptor_set_binding_layout *lb = b->binding;
   if (memcmp(la, lb, binding_size)) {
      for (unsigned i = 0; i < a->binding_count; i++) {
      if (memcmp(&la[i], &lb[i], offsetof(struct lvp_descriptor_set_binding_layout, immutable_samplers)))
                  /* immutable sampler equal */
   if (a->immutable_sampler_count != b->immutable_sampler_count)
         if (a->immutable_sampler_count) {
      size_t sampler_size = a->immutable_sampler_count * sizeof(struct lvp_sampler *);
   if (memcmp(pa + binding_offset + binding_size, pb + binding_offset + binding_size, sampler_size)) {
      struct lvp_sampler **sa = (struct lvp_sampler **)(pa + binding_offset);
   struct lvp_sampler **sb = (struct lvp_sampler **)(pb + binding_offset);
   for (unsigned i = 0; i < a->immutable_sampler_count; i++) {
      if (memcmp(sa[i], sb[i], sizeof(struct lvp_sampler)))
            }
      }
      static void
   check_db_compat(struct rendering_state *state, struct lvp_pipeline_layout *layout, enum lvp_pipeline_type pipeline_type, int first_set, int set_count)
   {
      bool independent = (layout->vk.create_flags & VK_PIPELINE_LAYOUT_CREATE_INDEPENDENT_SETS_BIT_EXT) > 0;
   /* handle compatibility rules for unbinding */
   for (unsigned j = 0; j < ARRAY_SIZE(state->desc_buffers); j++) {
      struct lvp_pipeline_layout *l2 = state->desc_buffer_offsets[pipeline_type][j].layout;
   if ((j >= first_set && j < first_set + set_count) || !l2 || l2 == layout)
         bool independent_l2 = (l2->vk.create_flags & VK_PIPELINE_LAYOUT_CREATE_INDEPENDENT_SETS_BIT_EXT) > 0;
   if (independent != independent_l2) {
         } else {
      if (layout->vk.set_count != l2->vk.set_count) {
         } else {
      const struct lvp_descriptor_set_layout *a = get_set_layout(layout, j);
   const struct lvp_descriptor_set_layout *b = get_set_layout(l2, j);
   if (!!a != !!b || !descriptor_layouts_equal(a, b)) {
                     }
      static void
   bind_db_samplers(struct rendering_state *state, enum lvp_pipeline_type pipeline_type, unsigned set)
   {
      const struct lvp_descriptor_set_layout *set_layout = state->desc_buffer_offsets[pipeline_type][set].sampler_layout;
   if (!set_layout)
         unsigned buffer_index = state->desc_buffer_offsets[pipeline_type][set].buffer_index;
   if (!state->desc_buffer_addrs[buffer_index]) {
      if (set_layout->immutable_set) {
      state->desc_sets[pipeline_type][set] = set_layout->immutable_set;
   u_foreach_bit(stage, set_layout->shader_stages)
      }
      }
   uint8_t *db = state->desc_buffer_addrs[buffer_index] + state->desc_buffer_offsets[pipeline_type][set].offset;
   uint8_t did_update = 0;
   for (uint32_t binding_index = 0; binding_index < set_layout->binding_count; binding_index++) {
      const struct lvp_descriptor_set_binding_layout *bind_layout = &set_layout->binding[binding_index];
   if (!bind_layout->immutable_samplers)
            struct lp_descriptor *desc = (void*)db;
            for (uint32_t sampler_index = 0; sampler_index < bind_layout->array_size; sampler_index++) {
      if (bind_layout->immutable_samplers[sampler_index]) {
      struct lp_descriptor *immutable_desc = &bind_layout->immutable_samplers[sampler_index]->desc;
   desc[sampler_index].sampler = immutable_desc->sampler;
   desc[sampler_index].sampler_index = immutable_desc->sampler_index;
   u_foreach_bit(stage, set_layout->shader_stages)
            }
   u_foreach_bit(stage, did_update)
      }
      static void
   handle_descriptor_buffer_embedded_samplers(struct vk_cmd_queue_entry *cmd, struct rendering_state *state)
   {
      const struct vk_cmd_bind_descriptor_buffer_embedded_samplers_ext *bind = &cmd->u.bind_descriptor_buffer_embedded_samplers_ext;
            if (!layout->vk.set_layouts[bind->set])
            const struct lvp_descriptor_set_layout *set_layout = get_set_layout(layout, bind->set);
   if (!set_layout->immutable_sampler_count)
         enum lvp_pipeline_type pipeline_type = lvp_pipeline_type_from_bind_point(bind->pipeline_bind_point);
            state->desc_buffer_offsets[pipeline_type][bind->set].sampler_layout = set_layout;
      }
      static void
   handle_descriptor_buffer_offsets(struct vk_cmd_queue_entry *cmd, struct rendering_state *state)
   {
      struct vk_cmd_set_descriptor_buffer_offsets_ext *dbo = &cmd->u.set_descriptor_buffer_offsets_ext;
   enum lvp_pipeline_type pipeline_type = lvp_pipeline_type_from_bind_point(dbo->pipeline_bind_point);
   for (unsigned i = 0; i < dbo->set_count; i++) {
      LVP_FROM_HANDLE(lvp_pipeline_layout, layout, dbo->layout);
   check_db_compat(state, layout, pipeline_type, dbo->first_set, dbo->set_count);
   unsigned idx = dbo->first_set + i;
   state->desc_buffer_offsets[pipeline_type][idx].layout = layout;
   state->desc_buffer_offsets[pipeline_type][idx].buffer_index = dbo->buffer_indices[i];
   state->desc_buffer_offsets[pipeline_type][idx].offset = dbo->offsets[i];
            /* set for all stages */
   u_foreach_bit(stage, set_layout->shader_stages) {
      gl_shader_stage pstage = vk_to_mesa_shader_stage(1<<stage);
      }
         }
      #ifdef VK_ENABLE_BETA_EXTENSIONS
   static void *
   lvp_push_internal_buffer(struct rendering_state *state, gl_shader_stage stage, uint32_t size)
   {
      if (!size)
            struct pipe_shader_buffer buffer = {
                  uint8_t *mem;
                        }
      static void
   dispatch_graph(struct rendering_state *state, const VkDispatchGraphInfoAMDX *info, void *scratch)
   {
      VK_FROM_HANDLE(lvp_pipeline, pipeline, state->exec_graph->groups[info->nodeIndex]);
   struct lvp_shader *shader = &pipeline->shaders[MESA_SHADER_COMPUTE];
            VkPipelineShaderStageNodeCreateInfoAMDX enqueue_node_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_NODE_CREATE_INFO_AMDX,
               for (uint32_t i = 0; i < info->payloadCount; i++) {
               /* The spec doesn't specify any useful limits for enqueued payloads.
   * Since we allocate them in scratch memory (provided to the dispatch entrypoint),
   * we need to execute recursive shaders one to keep scratch requirements finite.
   */
   VkDispatchIndirectCommand dispatch = *(const VkDispatchIndirectCommand *)payload;
   if (nir->info.cs.workgroup_count[0]) {
      dispatch.x = nir->info.cs.workgroup_count[0];
   dispatch.y = nir->info.cs.workgroup_count[1];
               state->dispatch_info.indirect = NULL;
   state->dispatch_info.grid[0] = 1;
   state->dispatch_info.grid[1] = 1;
            for (uint32_t z = 0; z < dispatch.z; z++) {
      for (uint32_t y = 0; y < dispatch.y; y++) {
      for (uint32_t x = 0; x < dispatch.x; x++) {
                                          struct lvp_exec_graph_internal_data *internal_data =
                                                for (uint32_t enqueue = 0; enqueue < ARRAY_SIZE(internal_data->outputs); enqueue++) {
                           VkDispatchGraphInfoAMDX enqueue_info = {
                                       ASSERTED VkResult result = lvp_GetExecutionGraphPipelineNodeIndexAMDX(
                                          }
      static void
   handle_dispatch_graph(struct vk_cmd_queue_entry *cmd, struct rendering_state *state)
   {
               for (uint32_t i = 0; i < dispatch->count_info->count; i++) {
      const VkDispatchGraphInfoAMDX *info = (const void *)((const uint8_t *)dispatch->count_info->infos.hostAddress +
                  }
   #endif
      void lvp_add_enqueue_cmd_entrypoints(struct vk_device_dispatch_table *disp)
   {
      struct vk_device_dispatch_table cmd_enqueue_dispatch;
   vk_device_dispatch_table_from_entrypoints(&cmd_enqueue_dispatch,
         #define ENQUEUE_CMD(CmdName) \
      assert(cmd_enqueue_dispatch.CmdName != NULL); \
            /* This list needs to match what's in lvp_execute_cmd_buffer exactly */
   ENQUEUE_CMD(CmdBindPipeline)
   ENQUEUE_CMD(CmdSetViewport)
   ENQUEUE_CMD(CmdSetViewportWithCount)
   ENQUEUE_CMD(CmdSetScissor)
   ENQUEUE_CMD(CmdSetScissorWithCount)
   ENQUEUE_CMD(CmdSetLineWidth)
   ENQUEUE_CMD(CmdSetDepthBias)
   ENQUEUE_CMD(CmdSetBlendConstants)
   ENQUEUE_CMD(CmdSetDepthBounds)
   ENQUEUE_CMD(CmdSetStencilCompareMask)
   ENQUEUE_CMD(CmdSetStencilWriteMask)
   ENQUEUE_CMD(CmdSetStencilReference)
   ENQUEUE_CMD(CmdBindDescriptorSets)
   ENQUEUE_CMD(CmdBindIndexBuffer)
   ENQUEUE_CMD(CmdBindIndexBuffer2KHR)
   ENQUEUE_CMD(CmdBindVertexBuffers2)
   ENQUEUE_CMD(CmdDraw)
   ENQUEUE_CMD(CmdDrawMultiEXT)
   ENQUEUE_CMD(CmdDrawIndexed)
   ENQUEUE_CMD(CmdDrawIndirect)
   ENQUEUE_CMD(CmdDrawIndexedIndirect)
   ENQUEUE_CMD(CmdDrawMultiIndexedEXT)
   ENQUEUE_CMD(CmdDispatch)
   ENQUEUE_CMD(CmdDispatchBase)
   ENQUEUE_CMD(CmdDispatchIndirect)
   ENQUEUE_CMD(CmdCopyBuffer2)
   ENQUEUE_CMD(CmdCopyImage2)
   ENQUEUE_CMD(CmdBlitImage2)
   ENQUEUE_CMD(CmdCopyBufferToImage2)
   ENQUEUE_CMD(CmdCopyImageToBuffer2)
   ENQUEUE_CMD(CmdUpdateBuffer)
   ENQUEUE_CMD(CmdFillBuffer)
   ENQUEUE_CMD(CmdClearColorImage)
   ENQUEUE_CMD(CmdClearDepthStencilImage)
   ENQUEUE_CMD(CmdClearAttachments)
   ENQUEUE_CMD(CmdResolveImage2)
   ENQUEUE_CMD(CmdBeginQueryIndexedEXT)
   ENQUEUE_CMD(CmdEndQueryIndexedEXT)
   ENQUEUE_CMD(CmdBeginQuery)
   ENQUEUE_CMD(CmdEndQuery)
   ENQUEUE_CMD(CmdResetQueryPool)
   ENQUEUE_CMD(CmdCopyQueryPoolResults)
   ENQUEUE_CMD(CmdPushConstants)
   ENQUEUE_CMD(CmdExecuteCommands)
   ENQUEUE_CMD(CmdDrawIndirectCount)
   ENQUEUE_CMD(CmdDrawIndexedIndirectCount)
      //   ENQUEUE_CMD(CmdPushDescriptorSetWithTemplateKHR)
      ENQUEUE_CMD(CmdBindTransformFeedbackBuffersEXT)
   ENQUEUE_CMD(CmdBeginTransformFeedbackEXT)
   ENQUEUE_CMD(CmdEndTransformFeedbackEXT)
   ENQUEUE_CMD(CmdDrawIndirectByteCountEXT)
   ENQUEUE_CMD(CmdBeginConditionalRenderingEXT)
   ENQUEUE_CMD(CmdEndConditionalRenderingEXT)
   ENQUEUE_CMD(CmdSetVertexInputEXT)
   ENQUEUE_CMD(CmdSetCullMode)
   ENQUEUE_CMD(CmdSetFrontFace)
   ENQUEUE_CMD(CmdSetPrimitiveTopology)
   ENQUEUE_CMD(CmdSetDepthTestEnable)
   ENQUEUE_CMD(CmdSetDepthWriteEnable)
   ENQUEUE_CMD(CmdSetDepthCompareOp)
   ENQUEUE_CMD(CmdSetDepthBoundsTestEnable)
   ENQUEUE_CMD(CmdSetStencilTestEnable)
   ENQUEUE_CMD(CmdSetStencilOp)
   ENQUEUE_CMD(CmdSetLineStippleEXT)
   ENQUEUE_CMD(CmdSetDepthBiasEnable)
   ENQUEUE_CMD(CmdSetLogicOpEXT)
   ENQUEUE_CMD(CmdSetPatchControlPointsEXT)
   ENQUEUE_CMD(CmdSetPrimitiveRestartEnable)
   ENQUEUE_CMD(CmdSetRasterizerDiscardEnable)
   ENQUEUE_CMD(CmdSetColorWriteEnableEXT)
   ENQUEUE_CMD(CmdBeginRendering)
   ENQUEUE_CMD(CmdEndRendering)
   ENQUEUE_CMD(CmdSetDeviceMask)
   ENQUEUE_CMD(CmdPipelineBarrier2)
   ENQUEUE_CMD(CmdResetEvent2)
   ENQUEUE_CMD(CmdSetEvent2)
   ENQUEUE_CMD(CmdWaitEvents2)
   ENQUEUE_CMD(CmdWriteTimestamp2)
   ENQUEUE_CMD(CmdBindDescriptorBuffersEXT)
   ENQUEUE_CMD(CmdSetDescriptorBufferOffsetsEXT)
            ENQUEUE_CMD(CmdSetPolygonModeEXT)
   ENQUEUE_CMD(CmdSetTessellationDomainOriginEXT)
   ENQUEUE_CMD(CmdSetDepthClampEnableEXT)
   ENQUEUE_CMD(CmdSetDepthClipEnableEXT)
   ENQUEUE_CMD(CmdSetLogicOpEnableEXT)
   ENQUEUE_CMD(CmdSetSampleMaskEXT)
   ENQUEUE_CMD(CmdSetRasterizationSamplesEXT)
   ENQUEUE_CMD(CmdSetAlphaToCoverageEnableEXT)
   ENQUEUE_CMD(CmdSetAlphaToOneEnableEXT)
   ENQUEUE_CMD(CmdSetDepthClipNegativeOneToOneEXT)
   ENQUEUE_CMD(CmdSetLineRasterizationModeEXT)
   ENQUEUE_CMD(CmdSetLineStippleEnableEXT)
   ENQUEUE_CMD(CmdSetProvokingVertexModeEXT)
   ENQUEUE_CMD(CmdSetColorBlendEnableEXT)
   ENQUEUE_CMD(CmdSetColorBlendEquationEXT)
            ENQUEUE_CMD(CmdBindShadersEXT)
   /* required for EXT_shader_object */
   ENQUEUE_CMD(CmdSetCoverageModulationModeNV)
   ENQUEUE_CMD(CmdSetCoverageModulationTableEnableNV)
   ENQUEUE_CMD(CmdSetCoverageModulationTableNV)
   ENQUEUE_CMD(CmdSetCoverageReductionModeNV)
   ENQUEUE_CMD(CmdSetCoverageToColorEnableNV)
   ENQUEUE_CMD(CmdSetCoverageToColorLocationNV)
   ENQUEUE_CMD(CmdSetRepresentativeFragmentTestEnableNV)
   ENQUEUE_CMD(CmdSetShadingRateImageEnableNV)
   ENQUEUE_CMD(CmdSetViewportSwizzleNV)
   ENQUEUE_CMD(CmdSetViewportWScalingEnableNV)
   ENQUEUE_CMD(CmdSetAttachmentFeedbackLoopEnableEXT)
   ENQUEUE_CMD(CmdDrawMeshTasksEXT)
   ENQUEUE_CMD(CmdDrawMeshTasksIndirectEXT)
            ENQUEUE_CMD(CmdBindPipelineShaderGroupNV)
   ENQUEUE_CMD(CmdPreprocessGeneratedCommandsNV)
         #ifdef VK_ENABLE_BETA_EXTENSIONS
      ENQUEUE_CMD(CmdInitializeGraphScratchMemoryAMDX)
   ENQUEUE_CMD(CmdDispatchGraphIndirectCountAMDX)
   ENQUEUE_CMD(CmdDispatchGraphIndirectAMDX)
      #endif
      #undef ENQUEUE_CMD
   }
      static void lvp_execute_cmd_buffer(struct list_head *cmds,
         {
      struct vk_cmd_queue_entry *cmd;
            LIST_FOR_EACH_ENTRY(cmd, cmds, cmd_link) {
      if (print_cmds)
         switch (cmd->type) {
   case VK_CMD_BIND_PIPELINE:
      handle_pipeline(cmd, state);
      case VK_CMD_SET_VIEWPORT:
      handle_set_viewport(cmd, state);
      case VK_CMD_SET_VIEWPORT_WITH_COUNT:
      handle_set_viewport_with_count(cmd, state);
      case VK_CMD_SET_SCISSOR:
      handle_set_scissor(cmd, state);
      case VK_CMD_SET_SCISSOR_WITH_COUNT:
      handle_set_scissor_with_count(cmd, state);
      case VK_CMD_SET_LINE_WIDTH:
      handle_set_line_width(cmd, state);
      case VK_CMD_SET_DEPTH_BIAS:
      handle_set_depth_bias(cmd, state);
      case VK_CMD_SET_BLEND_CONSTANTS:
      handle_set_blend_constants(cmd, state);
      case VK_CMD_SET_DEPTH_BOUNDS:
      handle_set_depth_bounds(cmd, state);
      case VK_CMD_SET_STENCIL_COMPARE_MASK:
      handle_set_stencil_compare_mask(cmd, state);
      case VK_CMD_SET_STENCIL_WRITE_MASK:
      handle_set_stencil_write_mask(cmd, state);
      case VK_CMD_SET_STENCIL_REFERENCE:
      handle_set_stencil_reference(cmd, state);
      case VK_CMD_BIND_DESCRIPTOR_SETS:
      handle_descriptor_sets(cmd, state);
      case VK_CMD_BIND_INDEX_BUFFER:
      handle_index_buffer(cmd, state);
      case VK_CMD_BIND_INDEX_BUFFER2_KHR:
      handle_index_buffer2(cmd, state);
      case VK_CMD_BIND_VERTEX_BUFFERS2:
      handle_vertex_buffers2(cmd, state);
      case VK_CMD_DRAW:
      emit_state(state);
   handle_draw(cmd, state);
      case VK_CMD_DRAW_MULTI_EXT:
      emit_state(state);
   handle_draw_multi(cmd, state);
      case VK_CMD_DRAW_INDEXED:
      emit_state(state);
   handle_draw_indexed(cmd, state);
      case VK_CMD_DRAW_INDIRECT:
      emit_state(state);
   handle_draw_indirect(cmd, state, false);
      case VK_CMD_DRAW_INDEXED_INDIRECT:
      emit_state(state);
   handle_draw_indirect(cmd, state, true);
      case VK_CMD_DRAW_MULTI_INDEXED_EXT:
      emit_state(state);
   handle_draw_multi_indexed(cmd, state);
      case VK_CMD_DISPATCH:
      emit_compute_state(state);
   handle_dispatch(cmd, state);
      case VK_CMD_DISPATCH_BASE:
      emit_compute_state(state);
   handle_dispatch_base(cmd, state);
      case VK_CMD_DISPATCH_INDIRECT:
      emit_compute_state(state);
   handle_dispatch_indirect(cmd, state);
      case VK_CMD_COPY_BUFFER2:
      handle_copy_buffer(cmd, state);
      case VK_CMD_COPY_IMAGE2:
      handle_copy_image(cmd, state);
      case VK_CMD_BLIT_IMAGE2:
      handle_blit_image(cmd, state);
      case VK_CMD_COPY_BUFFER_TO_IMAGE2:
      handle_copy_buffer_to_image(cmd, state);
      case VK_CMD_COPY_IMAGE_TO_BUFFER2:
      handle_copy_image_to_buffer2(cmd, state);
      case VK_CMD_UPDATE_BUFFER:
      handle_update_buffer(cmd, state);
      case VK_CMD_FILL_BUFFER:
      handle_fill_buffer(cmd, state);
      case VK_CMD_CLEAR_COLOR_IMAGE:
      handle_clear_color_image(cmd, state);
      case VK_CMD_CLEAR_DEPTH_STENCIL_IMAGE:
      handle_clear_ds_image(cmd, state);
      case VK_CMD_CLEAR_ATTACHMENTS:
      handle_clear_attachments(cmd, state);
      case VK_CMD_RESOLVE_IMAGE2:
      handle_resolve_image(cmd, state);
      case VK_CMD_PIPELINE_BARRIER2:
      /* flushes are actually stalls, so multiple flushes are redundant */
   if (did_flush)
         handle_pipeline_barrier(cmd, state);
   did_flush = true;
      case VK_CMD_BEGIN_QUERY_INDEXED_EXT:
      handle_begin_query_indexed_ext(cmd, state);
      case VK_CMD_END_QUERY_INDEXED_EXT:
      handle_end_query_indexed_ext(cmd, state);
      case VK_CMD_BEGIN_QUERY:
      handle_begin_query(cmd, state);
      case VK_CMD_END_QUERY:
      handle_end_query(cmd, state);
      case VK_CMD_RESET_QUERY_POOL:
      handle_reset_query_pool(cmd, state);
      case VK_CMD_COPY_QUERY_POOL_RESULTS:
      handle_copy_query_pool_results(cmd, state);
      case VK_CMD_PUSH_CONSTANTS:
      handle_push_constants(cmd, state);
      case VK_CMD_EXECUTE_COMMANDS:
      handle_execute_commands(cmd, state, print_cmds);
      case VK_CMD_DRAW_INDIRECT_COUNT:
      emit_state(state);
   handle_draw_indirect_count(cmd, state, false);
      case VK_CMD_DRAW_INDEXED_INDIRECT_COUNT:
      emit_state(state);
   handle_draw_indirect_count(cmd, state, true);
      case VK_CMD_PUSH_DESCRIPTOR_SET_KHR:
      handle_push_descriptor_set(cmd, state);
      case VK_CMD_PUSH_DESCRIPTOR_SET_WITH_TEMPLATE_KHR:
      handle_push_descriptor_set_with_template(cmd, state);
      case VK_CMD_BIND_TRANSFORM_FEEDBACK_BUFFERS_EXT:
      handle_bind_transform_feedback_buffers(cmd, state);
      case VK_CMD_BEGIN_TRANSFORM_FEEDBACK_EXT:
      handle_begin_transform_feedback(cmd, state);
      case VK_CMD_END_TRANSFORM_FEEDBACK_EXT:
      handle_end_transform_feedback(cmd, state);
      case VK_CMD_DRAW_INDIRECT_BYTE_COUNT_EXT:
      emit_state(state);
   handle_draw_indirect_byte_count(cmd, state);
      case VK_CMD_BEGIN_CONDITIONAL_RENDERING_EXT:
      handle_begin_conditional_rendering(cmd, state);
      case VK_CMD_END_CONDITIONAL_RENDERING_EXT:
      handle_end_conditional_rendering(state);
      case VK_CMD_SET_VERTEX_INPUT_EXT:
      handle_set_vertex_input(cmd, state);
      case VK_CMD_SET_CULL_MODE:
      handle_set_cull_mode(cmd, state);
      case VK_CMD_SET_FRONT_FACE:
      handle_set_front_face(cmd, state);
      case VK_CMD_SET_PRIMITIVE_TOPOLOGY:
      handle_set_primitive_topology(cmd, state);
      case VK_CMD_SET_DEPTH_TEST_ENABLE:
      handle_set_depth_test_enable(cmd, state);
      case VK_CMD_SET_DEPTH_WRITE_ENABLE:
      handle_set_depth_write_enable(cmd, state);
      case VK_CMD_SET_DEPTH_COMPARE_OP:
      handle_set_depth_compare_op(cmd, state);
      case VK_CMD_SET_DEPTH_BOUNDS_TEST_ENABLE:
      handle_set_depth_bounds_test_enable(cmd, state);
      case VK_CMD_SET_STENCIL_TEST_ENABLE:
      handle_set_stencil_test_enable(cmd, state);
      case VK_CMD_SET_STENCIL_OP:
      handle_set_stencil_op(cmd, state);
      case VK_CMD_SET_LINE_STIPPLE_EXT:
      handle_set_line_stipple(cmd, state);
      case VK_CMD_SET_DEPTH_BIAS_ENABLE:
      handle_set_depth_bias_enable(cmd, state);
      case VK_CMD_SET_LOGIC_OP_EXT:
      handle_set_logic_op(cmd, state);
      case VK_CMD_SET_PATCH_CONTROL_POINTS_EXT:
      handle_set_patch_control_points(cmd, state);
      case VK_CMD_SET_PRIMITIVE_RESTART_ENABLE:
      handle_set_primitive_restart_enable(cmd, state);
      case VK_CMD_SET_RASTERIZER_DISCARD_ENABLE:
      handle_set_rasterizer_discard_enable(cmd, state);
      case VK_CMD_SET_COLOR_WRITE_ENABLE_EXT:
      handle_set_color_write_enable(cmd, state);
      case VK_CMD_BEGIN_RENDERING:
      handle_begin_rendering(cmd, state);
      case VK_CMD_END_RENDERING:
      handle_end_rendering(cmd, state);
      case VK_CMD_SET_DEVICE_MASK:
      /* no-op */
      case VK_CMD_RESET_EVENT2:
      handle_event_reset2(cmd, state);
      case VK_CMD_SET_EVENT2:
      handle_event_set2(cmd, state);
      case VK_CMD_WAIT_EVENTS2:
      handle_wait_events2(cmd, state);
      case VK_CMD_WRITE_TIMESTAMP2:
      handle_write_timestamp2(cmd, state);
      case VK_CMD_SET_POLYGON_MODE_EXT:
      handle_set_polygon_mode(cmd, state);
      case VK_CMD_SET_TESSELLATION_DOMAIN_ORIGIN_EXT:
      handle_set_tessellation_domain_origin(cmd, state);
      case VK_CMD_SET_DEPTH_CLAMP_ENABLE_EXT:
      handle_set_depth_clamp_enable(cmd, state);
      case VK_CMD_SET_DEPTH_CLIP_ENABLE_EXT:
      handle_set_depth_clip_enable(cmd, state);
      case VK_CMD_SET_LOGIC_OP_ENABLE_EXT:
      handle_set_logic_op_enable(cmd, state);
      case VK_CMD_SET_SAMPLE_MASK_EXT:
      handle_set_sample_mask(cmd, state);
      case VK_CMD_SET_RASTERIZATION_SAMPLES_EXT:
      handle_set_samples(cmd, state);
      case VK_CMD_SET_ALPHA_TO_COVERAGE_ENABLE_EXT:
      handle_set_alpha_to_coverage(cmd, state);
      case VK_CMD_SET_ALPHA_TO_ONE_ENABLE_EXT:
      handle_set_alpha_to_one(cmd, state);
      case VK_CMD_SET_DEPTH_CLIP_NEGATIVE_ONE_TO_ONE_EXT:
      handle_set_halfz(cmd, state);
      case VK_CMD_SET_LINE_RASTERIZATION_MODE_EXT:
      handle_set_line_rasterization_mode(cmd, state);
      case VK_CMD_SET_LINE_STIPPLE_ENABLE_EXT:
      handle_set_line_stipple_enable(cmd, state);
      case VK_CMD_SET_PROVOKING_VERTEX_MODE_EXT:
      handle_set_provoking_vertex_mode(cmd, state);
      case VK_CMD_SET_COLOR_BLEND_ENABLE_EXT:
      handle_set_color_blend_enable(cmd, state);
      case VK_CMD_SET_COLOR_WRITE_MASK_EXT:
      handle_set_color_write_mask(cmd, state);
      case VK_CMD_SET_COLOR_BLEND_EQUATION_EXT:
      handle_set_color_blend_equation(cmd, state);
      case VK_CMD_BIND_SHADERS_EXT:
      handle_shaders(cmd, state);
      case VK_CMD_SET_ATTACHMENT_FEEDBACK_LOOP_ENABLE_EXT:
         case VK_CMD_DRAW_MESH_TASKS_EXT:
      emit_state(state);
   handle_draw_mesh_tasks(cmd, state);
      case VK_CMD_DRAW_MESH_TASKS_INDIRECT_EXT:
      emit_state(state);
   handle_draw_mesh_tasks_indirect(cmd, state);
      case VK_CMD_DRAW_MESH_TASKS_INDIRECT_COUNT_EXT:
      emit_state(state);
   handle_draw_mesh_tasks_indirect_count(cmd, state);
      case VK_CMD_BIND_PIPELINE_SHADER_GROUP_NV:
      handle_graphics_pipeline_group(cmd, state);
      case VK_CMD_PREPROCESS_GENERATED_COMMANDS_NV:
      handle_preprocess_generated_commands(cmd, state);
      case VK_CMD_EXECUTE_GENERATED_COMMANDS_NV:
      handle_execute_generated_commands(cmd, state, print_cmds);
      case VK_CMD_BIND_DESCRIPTOR_BUFFERS_EXT:
      handle_descriptor_buffers(cmd, state);
      case VK_CMD_SET_DESCRIPTOR_BUFFER_OFFSETS_EXT:
      handle_descriptor_buffer_offsets(cmd, state);
      case VK_CMD_BIND_DESCRIPTOR_BUFFER_EMBEDDED_SAMPLERS_EXT:
         #ifdef VK_ENABLE_BETA_EXTENSIONS
         case VK_CMD_INITIALIZE_GRAPH_SCRATCH_MEMORY_AMDX:
         case VK_CMD_DISPATCH_GRAPH_INDIRECT_COUNT_AMDX:
         case VK_CMD_DISPATCH_GRAPH_INDIRECT_AMDX:
         case VK_CMD_DISPATCH_GRAPH_AMDX:
         #endif
         default:
      fprintf(stderr, "Unsupported command %s\n", vk_cmd_queue_type_names[cmd->type]);
   unreachable("Unsupported command");
      }
   did_flush = false;
   if (!cmd->cmd_link.next)
         }
      VkResult lvp_execute_cmds(struct lvp_device *device,
               {
      struct rendering_state *state = queue->state;
   memset(state, 0, sizeof(*state));
   state->pctx = queue->ctx;
   state->device = device;
   state->uploader = queue->uploader;
   state->cso = queue->cso;
   state->blend_dirty = true;
   state->dsa_dirty = true;
   state->rs_dirty = true;
   state->vp_dirty = true;
   state->rs_state.point_line_tri_clip = true;
   state->rs_state.unclamped_fragment_depth_values = device->vk.enabled_extensions.EXT_depth_range_unrestricted;
   state->sample_mask_dirty = true;
   state->min_samples_dirty = true;
   state->sample_mask = UINT32_MAX;
   state->poison_mem = device->poison_mem;
            /* default values */
   state->min_sample_shading = 1;
   state->num_viewports = 1;
   state->num_scissors = 1;
   state->rs_state.line_width = 1.0;
   state->rs_state.flatshade_first = true;
   state->rs_state.clip_halfz = true;
   state->rs_state.front_ccw = true;
   state->rs_state.point_size_per_vertex = true;
   state->rs_state.point_quad_rasterization = true;
   state->rs_state.half_pixel_center = true;
   state->rs_state.scissor = true;
   state->rs_state.no_ms_sample_mask_out = true;
            /* create a gallium context */
            state->start_vb = -1;
   state->num_vb = 0;
   cso_unbind_context(queue->cso);
   for (unsigned i = 0; i < ARRAY_SIZE(state->so_targets); i++) {
      if (state->so_targets[i]) {
                     if (util_dynarray_num_elements(&state->push_desc_sets, struct lvp_descriptor_set *))
            util_dynarray_foreach (&state->push_desc_sets, struct lvp_descriptor_set *, set)
                     for (unsigned i = 0; i < ARRAY_SIZE(state->desc_buffers); i++)
               }
      size_t
   lvp_get_rendering_state_size(void)
   {
         }
