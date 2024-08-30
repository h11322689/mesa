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
      #include "lvp_private.h"
   #include "vk_nir_convert_ycbcr.h"
   #include "vk_pipeline.h"
   #include "vk_render_pass.h"
   #include "vk_util.h"
   #include "glsl_types.h"
   #include "util/os_time.h"
   #include "spirv/nir_spirv.h"
   #include "nir/nir_builder.h"
   #include "nir/nir_serialize.h"
   #include "lvp_lower_vulkan_resource.h"
   #include "pipe/p_state.h"
   #include "pipe/p_context.h"
   #include "nir/nir_xfb_info.h"
      #define SPIR_V_MAGIC_NUMBER 0x07230203
      #define MAX_DYNAMIC_STATES 72
      typedef void (*cso_destroy_func)(struct pipe_context*, void*);
      static void
   shader_destroy(struct lvp_device *device, struct lvp_shader *shader, bool locked)
   {
      if (!shader->pipeline_nir)
         gl_shader_stage stage = shader->pipeline_nir->nir->info.stage;
   cso_destroy_func destroy[] = {
      device->queue.ctx->delete_vs_state,
   device->queue.ctx->delete_tcs_state,
   device->queue.ctx->delete_tes_state,
   device->queue.ctx->delete_gs_state,
   device->queue.ctx->delete_fs_state,
   device->queue.ctx->delete_compute_state,
   device->queue.ctx->delete_ts_state,
               if (!locked)
            set_foreach(&shader->inlines.variants, entry) {
      struct lvp_inline_variant *variant = (void*)entry->key;
   destroy[stage](device->queue.ctx, variant->cso);
      }
            if (shader->shader_cso)
         if (shader->tess_ccw_cso)
            if (!locked)
            lvp_pipeline_nir_ref(&shader->pipeline_nir, NULL);
      }
      void
   lvp_pipeline_destroy(struct lvp_device *device, struct lvp_pipeline *pipeline, bool locked)
   {
      lvp_forall_stage(i)
            if (pipeline->layout)
            for (unsigned i = 0; i < pipeline->num_groups; i++) {
      LVP_FROM_HANDLE(lvp_pipeline, p, pipeline->groups[i]);
               vk_free(&device->vk.alloc, pipeline->state_data);
   vk_object_base_finish(&pipeline->base);
      }
      VKAPI_ATTR void VKAPI_CALL lvp_DestroyPipeline(
      VkDevice                                    _device,
   VkPipeline                                  _pipeline,
      {
      LVP_FROM_HANDLE(lvp_device, device, _device);
            if (!_pipeline)
            if (pipeline->used) {
      simple_mtx_lock(&device->queue.lock);
   util_dynarray_append(&device->queue.pipeline_destroys, struct lvp_pipeline*, pipeline);
      } else {
            }
      static void
   shared_var_info(const struct glsl_type *type, unsigned *size, unsigned *align)
   {
               uint32_t comp_size = glsl_type_is_boolean(type)
         unsigned length = glsl_get_vector_elements(type);
   *size = comp_size * length,
      }
      static bool
   remove_barriers_impl(nir_builder *b, nir_intrinsic_instr *intr, void *data)
   {
      if (intr->intrinsic != nir_intrinsic_barrier)
         if (data) {
      if (nir_intrinsic_execution_scope(intr) != SCOPE_NONE)
            if (nir_intrinsic_memory_scope(intr) == SCOPE_WORKGROUP ||
      nir_intrinsic_memory_scope(intr) == SCOPE_DEVICE ||
   nir_intrinsic_memory_scope(intr) == SCOPE_QUEUE_FAMILY)
   }
   nir_instr_remove(&intr->instr);
      }
      static bool
   remove_barriers(nir_shader *nir, bool is_compute)
   {
      return nir_shader_intrinsics_pass(nir, remove_barriers_impl,
            }
      static bool
   lower_demote_impl(nir_builder *b, nir_intrinsic_instr *intr, void *data)
   {
      if (intr->intrinsic == nir_intrinsic_demote || intr->intrinsic == nir_intrinsic_terminate) {
      intr->intrinsic = nir_intrinsic_discard;
      }
   if (intr->intrinsic == nir_intrinsic_demote_if || intr->intrinsic == nir_intrinsic_terminate_if) {
      intr->intrinsic = nir_intrinsic_discard_if;
      }
      }
      static bool
   lower_demote(nir_shader *nir)
   {
      return nir_shader_intrinsics_pass(nir, lower_demote_impl,
      }
      static bool
   find_tex(const nir_instr *instr, const void *data_cb)
   {
      if (instr->type == nir_instr_type_tex)
            }
      static nir_def *
   fixup_tex_instr(struct nir_builder *b, nir_instr *instr, void *data_cb)
   {
      nir_tex_instr *tex_instr = nir_instr_as_tex(instr);
            int idx = nir_tex_instr_src_index(tex_instr, nir_tex_src_texture_offset);
   if (idx == -1)
            if (!nir_src_is_const(tex_instr->src[idx].src))
                  nir_tex_instr_remove_src(tex_instr, idx);
   tex_instr->texture_index += offset;
      }
      static bool
   lvp_nir_fixup_indirect_tex(nir_shader *shader)
   {
         }
      static void
   optimize(nir_shader *nir)
   {
      bool progress = false;
   do {
               NIR_PASS(progress, nir, nir_lower_flrp, 32|64, true);
   NIR_PASS(progress, nir, nir_split_array_vars, nir_var_function_temp);
   NIR_PASS(progress, nir, nir_shrink_vec_array_vars, nir_var_function_temp);
   NIR_PASS(progress, nir, nir_opt_deref);
                     NIR_PASS(progress, nir, nir_copy_prop);
   NIR_PASS(progress, nir, nir_opt_dce);
            NIR_PASS(progress, nir, nir_opt_algebraic);
            NIR_PASS(progress, nir, nir_opt_remove_phis);
   bool trivial_continues = false;
   NIR_PASS(trivial_continues, nir, nir_opt_trivial_continues);
   progress |= trivial_continues;
   if (trivial_continues) {
      /* If nir_opt_trivial_continues makes progress, then we need to clean
   * things up if we want any hope of nir_opt_if or nir_opt_loop_unroll
   * to make progress.
   */
   NIR_PASS(progress, nir, nir_copy_prop);
   NIR_PASS(progress, nir, nir_opt_dce);
      }
   NIR_PASS(progress, nir, nir_opt_if, nir_opt_if_aggressive_last_continue | nir_opt_if_optimize_phi_true_false);
   NIR_PASS(progress, nir, nir_opt_dead_cf);
   NIR_PASS(progress, nir, nir_opt_conditional_discard);
   NIR_PASS(progress, nir, nir_opt_remove_phis);
   NIR_PASS(progress, nir, nir_opt_cse);
            NIR_PASS(progress, nir, nir_opt_deref);
   NIR_PASS(progress, nir, nir_lower_alu_to_scalar, NULL, NULL);
   NIR_PASS(progress, nir, nir_opt_loop_unroll);
         }
      void
   lvp_shader_optimize(nir_shader *nir)
   {
      optimize(nir);
   NIR_PASS_V(nir, nir_lower_var_copies);
   NIR_PASS_V(nir, nir_remove_dead_variables, nir_var_function_temp, NULL);
   NIR_PASS_V(nir, nir_opt_dce);
      }
      static struct lvp_pipeline_nir *
   create_pipeline_nir(nir_shader *nir)
   {
      struct lvp_pipeline_nir *pipeline_nir = ralloc(NULL, struct lvp_pipeline_nir);
   pipeline_nir->nir = nir;
   pipeline_nir->ref_cnt = 1;
      }
      static VkResult
   compile_spirv(struct lvp_device *pdevice, const VkPipelineShaderStageCreateInfo *sinfo, nir_shader **nir)
   {
      gl_shader_stage stage = vk_to_mesa_shader_stage(sinfo->stage);
   assert(stage <= LVP_SHADER_STAGES && stage != MESA_SHADER_NONE);
         #ifdef VK_ENABLE_BETA_EXTENSIONS
      const VkPipelineShaderStageNodeCreateInfoAMDX *node_info = vk_find_struct_const(
      #endif
         const struct spirv_to_nir_options spirv_options = {
      .environment = NIR_SPIRV_VULKAN,
   .caps = {
      .float64 = (pdevice->pscreen->get_param(pdevice->pscreen, PIPE_CAP_DOUBLES) == 1),
   .int16 = true,
   .int64 = (pdevice->pscreen->get_param(pdevice->pscreen, PIPE_CAP_INT64) == 1),
   .tessellation = true,
      #if LLVM_VERSION_MAJOR >= 15
         #endif
            .image_ms_array = true,
   .image_read_without_format = true,
   .image_write_without_format = true,
   .storage_image_ms = true,
   .geometry_streams = true,
   .storage_8bit = true,
   .storage_16bit = true,
   .variable_pointers = true,
   .stencil_export = true,
   .post_depth_coverage = true,
   .transform_feedback = true,
   .device_group = true,
   .draw_parameters = true,
   .shader_viewport_index_layer = true,
   .shader_clock = true,
   .multiview = true,
   .physical_storage_buffer_address = true,
   .int64_atomics = true,
   .subgroup_arithmetic = true,
   .subgroup_basic = true,
      #if LLVM_VERSION_MAJOR >= 10
         #endif
            .subgroup_vote = true,
   .vk_memory_model = true,
   .vk_memory_model_device_scope = true,
   .int8 = true,
   .float16 = true,
   .demote_to_helper_invocation = true,
   .mesh_shading = true,
   .descriptor_array_dynamic_indexing = true,
   .descriptor_array_non_uniform_indexing = true,
   .descriptor_indexing = true,
   .runtime_descriptor_array = true,
      },
   .ubo_addr_format = nir_address_format_vec2_index_32bit_offset,
   .ssbo_addr_format = nir_address_format_vec2_index_32bit_offset,
   .phys_ssbo_addr_format = nir_address_format_64bit_global,
   .push_const_addr_format = nir_address_format_logical,
   #ifdef VK_ENABLE_BETA_EXTENSIONS
         #endif
               result = vk_pipeline_shader_stage_to_nir(&pdevice->vk, sinfo,
                  }
      static bool
   inline_variant_equals(const void *a, const void *b)
   {
      const struct lvp_inline_variant *av = a, *bv = b;
   assert(av->mask == bv->mask);
   u_foreach_bit(slot, av->mask) {
      if (memcmp(av->vals[slot], bv->vals[slot], sizeof(av->vals[slot])))
      }
      }
      static const struct vk_ycbcr_conversion_state *
   lvp_ycbcr_conversion_lookup(const void *data, uint32_t set, uint32_t binding, uint32_t array_index)
   {
               const struct lvp_descriptor_set_layout *set_layout = container_of(layout->vk.set_layouts[set], struct lvp_descriptor_set_layout, vk);
   const struct lvp_descriptor_set_binding_layout *binding_layout = &set_layout->binding[binding];
   if (!binding_layout->immutable_samplers)
            struct vk_ycbcr_conversion *ycbcr_conversion = binding_layout->immutable_samplers[array_index]->vk.ycbcr_conversion;
      }
      /* pipeline is NULL for shader objects. */
   static void
   lvp_shader_lower(struct lvp_device *pdevice, struct lvp_pipeline *pipeline, nir_shader *nir, struct lvp_pipeline_layout *layout)
   {
      if (nir->info.stage != MESA_SHADER_TESS_CTRL)
            const struct nir_lower_sysvals_to_varyings_options sysvals_to_varyings = {
      .frag_coord = true,
      };
            struct nir_lower_subgroups_options subgroup_opts = {0};
   subgroup_opts.lower_quad = true;
   subgroup_opts.ballot_components = 1;
   subgroup_opts.ballot_bit_size = 32;
   subgroup_opts.lower_inverse_ballot = true;
            if (nir->info.stage == MESA_SHADER_FRAGMENT)
         NIR_PASS_V(nir, nir_lower_system_values);
   NIR_PASS_V(nir, nir_lower_is_helper_invocation);
   NIR_PASS_V(nir, lower_demote);
            NIR_PASS_V(nir, nir_remove_dead_variables,
            optimize(nir);
            NIR_PASS_V(nir, nir_lower_io_to_temporaries, nir_shader_get_entrypoint(nir), true, true);
   NIR_PASS_V(nir, nir_split_var_copies);
            NIR_PASS_V(nir, nir_lower_explicit_io, nir_var_mem_push_const,
            NIR_PASS_V(nir, nir_lower_explicit_io,
                  NIR_PASS_V(nir, nir_lower_explicit_io,
                  if (nir->info.stage == MESA_SHADER_COMPUTE)
                     nir_lower_non_uniform_access_options options = {
         };
                     if (nir->info.stage == MESA_SHADER_COMPUTE ||
      nir->info.stage == MESA_SHADER_TASK ||
   nir->info.stage == MESA_SHADER_MESH) {
   NIR_PASS_V(nir, nir_lower_vars_to_explicit_types, nir_var_mem_shared, shared_var_info);
               if (nir->info.stage == MESA_SHADER_TASK ||
      nir->info.stage == MESA_SHADER_MESH) {
   NIR_PASS_V(nir, nir_lower_vars_to_explicit_types, nir_var_mem_task_payload, shared_var_info);
                        if (nir->info.stage == MESA_SHADER_VERTEX ||
      nir->info.stage == MESA_SHADER_GEOMETRY) {
      } else if (nir->info.stage == MESA_SHADER_FRAGMENT) {
                  // TODO: also optimize the tex srcs. see radeonSI for reference */
   /* Skip if there are potentially conflicting rounding modes */
   struct nir_fold_16bit_tex_image_options fold_16bit_options = {
      .rounding_mode = nir_rounding_mode_undef,
      };
            /* Lower texture OPs llvmpipe supports to reduce the amount of sample
   * functions that need to be pre-compiled.
   */
   const nir_lower_tex_options tex_options = {
         };
                     if (nir->info.stage != MESA_SHADER_VERTEX)
         else {
      nir->num_inputs = util_last_bit64(nir->info.inputs_read);
   nir_foreach_shader_in_variable(var, nir) {
            }
   nir_assign_io_var_locations(nir, nir_var_shader_out, &nir->num_outputs,
      }
      static void
   lvp_shader_init(struct lvp_shader *shader, nir_shader *nir)
   {
      nir_function_impl *impl = nir_shader_get_entrypoint(nir);
   if (impl->ssa_alloc > 100) //skip for small shaders
         shader->pipeline_nir = create_pipeline_nir(nir);
   if (shader->inlines.can_inline)
      }
      static VkResult
   lvp_shader_compile_to_ir(struct lvp_pipeline *pipeline,
         {
      struct lvp_device *pdevice = pipeline->device;
   gl_shader_stage stage = vk_to_mesa_shader_stage(sinfo->stage);
   assert(stage <= LVP_SHADER_STAGES && stage != MESA_SHADER_NONE);
   struct lvp_shader *shader = &pipeline->shaders[stage];
   nir_shader *nir;
   VkResult result = compile_spirv(pdevice, sinfo, &nir);
   if (result == VK_SUCCESS) {
      lvp_shader_lower(pdevice, pipeline, nir, pipeline->layout);
      }
      }
      static void
   merge_tess_info(struct shader_info *tes_info,
         {
      /* The Vulkan 1.0.38 spec, section 21.1 Tessellator says:
   *
   *    "PointMode. Controls generation of points rather than triangles
   *     or lines. This functionality defaults to disabled, and is
   *     enabled if either shader stage includes the execution mode.
   *
   * and about Triangles, Quads, IsoLines, VertexOrderCw, VertexOrderCcw,
   * PointMode, SpacingEqual, SpacingFractionalEven, SpacingFractionalOdd,
   * and OutputVertices, it says:
   *
   *    "One mode must be set in at least one of the tessellation
   *     shader stages."
   *
   * So, the fields can be set in either the TCS or TES, but they must
   * agree if set in both.  Our backend looks at TES, so bitwise-or in
   * the values from the TCS.
   */
   assert(tcs_info->tess.tcs_vertices_out == 0 ||
         tes_info->tess.tcs_vertices_out == 0 ||
            assert(tcs_info->tess.spacing == TESS_SPACING_UNSPECIFIED ||
         tes_info->tess.spacing == TESS_SPACING_UNSPECIFIED ||
            assert(tcs_info->tess._primitive_mode == 0 ||
         tes_info->tess._primitive_mode == 0 ||
   tes_info->tess._primitive_mode |= tcs_info->tess._primitive_mode;
   tes_info->tess.ccw |= tcs_info->tess.ccw;
      }
      static void
   lvp_shader_xfb_init(struct lvp_shader *shader)
   {
      nir_xfb_info *xfb_info = shader->pipeline_nir->nir->xfb_info;
   if (xfb_info) {
      uint8_t output_mapping[VARYING_SLOT_TESS_MAX];
            nir_foreach_shader_out_variable(var, shader->pipeline_nir->nir) {
      unsigned slots = nir_variable_count_slots(var, var->type);
   for (unsigned i = 0; i < slots; i++)
               shader->stream_output.num_outputs = xfb_info->output_count;
   for (unsigned i = 0; i < PIPE_MAX_SO_BUFFERS; i++) {
      if (xfb_info->buffers_written & (1 << i)) {
            }
   for (unsigned i = 0; i < xfb_info->output_count; i++) {
      shader->stream_output.output[i].output_buffer = xfb_info->outputs[i].buffer;
   shader->stream_output.output[i].dst_offset = xfb_info->outputs[i].offset / 4;
   shader->stream_output.output[i].register_index = output_mapping[xfb_info->outputs[i].location];
   shader->stream_output.output[i].num_components = util_bitcount(xfb_info->outputs[i].component_mask);
   shader->stream_output.output[i].start_component = xfb_info->outputs[i].component_offset;
               }
      static void
   lvp_pipeline_xfb_init(struct lvp_pipeline *pipeline)
   {
      gl_shader_stage stage = MESA_SHADER_VERTEX;
   if (pipeline->shaders[MESA_SHADER_GEOMETRY].pipeline_nir)
         else if (pipeline->shaders[MESA_SHADER_TESS_EVAL].pipeline_nir)
         else if (pipeline->shaders[MESA_SHADER_MESH].pipeline_nir)
         pipeline->last_vertex = stage;
      }
      static void *
   lvp_shader_compile_stage(struct lvp_device *device, struct lvp_shader *shader, nir_shader *nir)
   {
      if (nir->info.stage == MESA_SHADER_COMPUTE) {
      struct pipe_compute_state shstate = {0};
   shstate.prog = nir;
   shstate.ir_type = PIPE_SHADER_IR_NIR;
   shstate.static_shared_mem = nir->info.shared_size;
      } else {
      struct pipe_shader_state shstate = {0};
   shstate.type = PIPE_SHADER_IR_NIR;
   shstate.ir.nir = nir;
            switch (nir->info.stage) {
   case MESA_SHADER_FRAGMENT:
         case MESA_SHADER_VERTEX:
         case MESA_SHADER_GEOMETRY:
         case MESA_SHADER_TESS_CTRL:
         case MESA_SHADER_TESS_EVAL:
         case MESA_SHADER_TASK:
         case MESA_SHADER_MESH:
         default:
      unreachable("illegal shader");
         }
      }
      void *
   lvp_shader_compile(struct lvp_device *device, struct lvp_shader *shader, nir_shader *nir, bool locked)
   {
               if (!locked)
                     if (!locked)
               }
      #ifndef NDEBUG
   static bool
   layouts_equal(const struct lvp_descriptor_set_layout *a, const struct lvp_descriptor_set_layout *b)
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
   #endif
      static void
   merge_layouts(struct vk_device *device, struct lvp_pipeline *dst, struct lvp_pipeline_layout *src)
   {
      if (!src)
         if (dst->layout) {
      /* these must match */
   ASSERTED VkPipelineCreateFlags src_flag = src->vk.create_flags & VK_PIPELINE_LAYOUT_CREATE_INDEPENDENT_SETS_BIT_EXT;
   ASSERTED VkPipelineCreateFlags dst_flag = dst->layout->vk.create_flags & VK_PIPELINE_LAYOUT_CREATE_INDEPENDENT_SETS_BIT_EXT;
      }
   /* always try to reuse existing layout: independent sets bit doesn't guarantee independent sets */
   if (!dst->layout) {
      dst->layout = (struct lvp_pipeline_layout*)vk_pipeline_layout_ref(&src->vk);
      }
   /* this is a big optimization when hit */
   if (dst->layout == src)
      #ifndef NDEBUG
      /* verify that layouts match */
   const struct lvp_pipeline_layout *smaller = dst->layout->vk.set_count < src->vk.set_count ? dst->layout : src;
   const struct lvp_pipeline_layout *bigger = smaller == dst->layout ? src : dst->layout;
   for (unsigned i = 0; i < smaller->vk.set_count; i++) {
      if (!smaller->vk.set_layouts[i] || !bigger->vk.set_layouts[i] ||
                  const struct lvp_descriptor_set_layout *smaller_set_layout =
         const struct lvp_descriptor_set_layout *bigger_set_layout =
            assert(!smaller_set_layout->binding_count ||
               #endif
      /* must be independent sets with different layouts: reallocate to avoid modifying original layout */
   struct lvp_pipeline_layout *old_layout = dst->layout;
   dst->layout = vk_zalloc(&device->alloc, sizeof(struct lvp_pipeline_layout), 8, VK_SYSTEM_ALLOCATION_SCOPE_OBJECT);
   memcpy(dst->layout, old_layout, sizeof(struct lvp_pipeline_layout));
   dst->layout->vk.ref_cnt = 1;
   for (unsigned i = 0; i < dst->layout->vk.set_count; i++) {
      if (dst->layout->vk.set_layouts[i])
      }
            for (unsigned i = 0; i < src->vk.set_count; i++) {
      if (!dst->layout->vk.set_layouts[i]) {
      dst->layout->vk.set_layouts[i] = src->vk.set_layouts[i];
   if (dst->layout->vk.set_layouts[i])
         }
   dst->layout->vk.set_count = MAX2(dst->layout->vk.set_count,
         dst->layout->push_constant_size += src->push_constant_size;
      }
      static void
   copy_shader_sanitized(struct lvp_shader *dst, const struct lvp_shader *src)
   {
      *dst = *src;
   dst->pipeline_nir = NULL; //this gets handled later
   dst->tess_ccw = NULL; //this gets handled later
   assert(!dst->shader_cso);
   assert(!dst->tess_ccw_cso);
   if (src->inlines.can_inline)
      }
      static VkResult
   lvp_graphics_pipeline_init(struct lvp_pipeline *pipeline,
                           {
                        const VkGraphicsPipelineLibraryCreateInfoEXT *libinfo = vk_find_struct_const(pCreateInfo,
         const VkPipelineLibraryCreateInfoKHR *libstate = vk_find_struct_const(pCreateInfo,
         const VkGraphicsPipelineLibraryFlagsEXT layout_stages = VK_GRAPHICS_PIPELINE_LIBRARY_PRE_RASTERIZATION_SHADERS_BIT_EXT |
         if (libinfo)
         else if (!libstate)
      pipeline->stages = VK_GRAPHICS_PIPELINE_LIBRARY_VERTEX_INPUT_INTERFACE_BIT_EXT |
                     if (flags & VK_PIPELINE_CREATE_2_LIBRARY_BIT_KHR)
                     if (!layout || !(layout->vk.create_flags & VK_PIPELINE_LAYOUT_CREATE_INDEPENDENT_SETS_BIT_EXT))
      /* this is a regular pipeline with no partials: directly reuse */
      else if (pipeline->stages & layout_stages) {
      if ((pipeline->stages & layout_stages) == layout_stages)
      /* this has all the layout stages: directly reuse */
      else {
      /* this is a partial: copy for later merging to avoid modifying another layout */
                  if (libstate) {
      for (unsigned i = 0; i < libstate->libraryCount; i++) {
      LVP_FROM_HANDLE(lvp_pipeline, p, libstate->pLibraries[i]);
   vk_graphics_pipeline_state_merge(&pipeline->graphics_state,
         if (p->stages & VK_GRAPHICS_PIPELINE_LIBRARY_PRE_RASTERIZATION_SHADERS_BIT_EXT) {
      pipeline->line_smooth = p->line_smooth;
   pipeline->disable_multisample = p->disable_multisample;
   pipeline->line_rectangular = p->line_rectangular;
   memcpy(pipeline->shaders, p->shaders, sizeof(struct lvp_shader) * 4);
   memcpy(&pipeline->shaders[MESA_SHADER_TASK], &p->shaders[MESA_SHADER_TASK], sizeof(struct lvp_shader) * 2);
   lvp_forall_gfx_stage(i) {
      if (i == MESA_SHADER_FRAGMENT)
               }
   if (p->stages & VK_GRAPHICS_PIPELINE_LIBRARY_FRAGMENT_SHADER_BIT_EXT) {
      pipeline->force_min_sample = p->force_min_sample;
      }
   if (p->stages & layout_stages) {
      if (!layout || (layout->vk.create_flags & VK_PIPELINE_LAYOUT_CREATE_INDEPENDENT_SETS_BIT_EXT))
      }
                  result = vk_graphics_pipeline_state_fill(&device->vk,
                           if (result != VK_SUCCESS)
            assert(pipeline->library || pipeline->stages & (VK_GRAPHICS_PIPELINE_LIBRARY_PRE_RASTERIZATION_SHADERS_BIT_EXT |
                           for (uint32_t i = 0; i < pCreateInfo->stageCount; i++) {
      const VkPipelineShaderStageCreateInfo *sinfo = &pCreateInfo->pStages[i];
   gl_shader_stage stage = vk_to_mesa_shader_stage(sinfo->stage);
   if (stage == MESA_SHADER_FRAGMENT) {
      if (!(pipeline->stages & VK_GRAPHICS_PIPELINE_LIBRARY_FRAGMENT_SHADER_BIT_EXT))
      } else {
      if (!(pipeline->stages & VK_GRAPHICS_PIPELINE_LIBRARY_PRE_RASTERIZATION_SHADERS_BIT_EXT))
      }
   result = lvp_shader_compile_to_ir(pipeline, sinfo);
   if (result != VK_SUCCESS)
            switch (stage) {
   case MESA_SHADER_FRAGMENT:
      if (pipeline->shaders[MESA_SHADER_FRAGMENT].pipeline_nir->nir->info.fs.uses_sample_shading)
            default: break;
      }
   if (pCreateInfo->stageCount && pipeline->shaders[MESA_SHADER_TESS_EVAL].pipeline_nir) {
      nir_lower_patch_vertices(pipeline->shaders[MESA_SHADER_TESS_EVAL].pipeline_nir->nir, pipeline->shaders[MESA_SHADER_TESS_CTRL].pipeline_nir->nir->info.tess.tcs_vertices_out, NULL);
   merge_tess_info(&pipeline->shaders[MESA_SHADER_TESS_EVAL].pipeline_nir->nir->info, &pipeline->shaders[MESA_SHADER_TESS_CTRL].pipeline_nir->nir->info);
   if (BITSET_TEST(pipeline->graphics_state.dynamic,
            pipeline->shaders[MESA_SHADER_TESS_EVAL].tess_ccw = create_pipeline_nir(nir_shader_clone(NULL, pipeline->shaders[MESA_SHADER_TESS_EVAL].pipeline_nir->nir));
      } else if (pipeline->graphics_state.ts &&
                  }
   if (libstate) {
      for (unsigned i = 0; i < libstate->libraryCount; i++) {
      LVP_FROM_HANDLE(lvp_pipeline, p, libstate->pLibraries[i]);
   if (p->stages & VK_GRAPHICS_PIPELINE_LIBRARY_FRAGMENT_SHADER_BIT_EXT) {
      if (p->shaders[MESA_SHADER_FRAGMENT].pipeline_nir)
      }
   if (p->stages & VK_GRAPHICS_PIPELINE_LIBRARY_PRE_RASTERIZATION_SHADERS_BIT_EXT) {
      lvp_forall_gfx_stage(j) {
      if (j == MESA_SHADER_FRAGMENT)
         if (p->shaders[j].pipeline_nir)
      }
   if (p->shaders[MESA_SHADER_TESS_EVAL].tess_ccw)
            } else if (pipeline->stages & VK_GRAPHICS_PIPELINE_LIBRARY_PRE_RASTERIZATION_SHADERS_BIT_EXT) {
      const struct vk_rasterization_state *rs = pipeline->graphics_state.rs;
   if (rs) {
      /* always draw bresenham if !smooth */
   pipeline->line_smooth = rs->line.mode == VK_LINE_RASTERIZATION_MODE_RECTANGULAR_SMOOTH_EXT;
   pipeline->disable_multisample = rs->line.mode == VK_LINE_RASTERIZATION_MODE_BRESENHAM_EXT ||
            } else
            }
   if (!libstate && !pipeline->library)
                  fail:
      for (unsigned i = 0; i < ARRAY_SIZE(pipeline->shaders); i++) {
         }
               }
      void
   lvp_pipeline_shaders_compile(struct lvp_pipeline *pipeline, bool locked)
   {
      if (pipeline->compiled)
         for (uint32_t i = 0; i < ARRAY_SIZE(pipeline->shaders); i++) {
      if (!pipeline->shaders[i].pipeline_nir)
            gl_shader_stage stage = i;
            if (!pipeline->shaders[stage].inlines.can_inline) {
      pipeline->shaders[stage].shader_cso = lvp_shader_compile(pipeline->device, &pipeline->shaders[stage],
         if (pipeline->shaders[MESA_SHADER_TESS_EVAL].tess_ccw)
      pipeline->shaders[MESA_SHADER_TESS_EVAL].tess_ccw_cso = lvp_shader_compile(pipeline->device, &pipeline->shaders[stage],
      }
      }
      static VkResult
   lvp_graphics_pipeline_create(
      VkDevice _device,
   VkPipelineCache _cache,
   const VkGraphicsPipelineCreateInfo *pCreateInfo,
   VkPipelineCreateFlagBits2KHR flags,
   VkPipeline *pPipeline,
      {
      LVP_FROM_HANDLE(lvp_device, device, _device);
   LVP_FROM_HANDLE(lvp_pipeline_cache, cache, _cache);
   struct lvp_pipeline *pipeline;
                     size_t size = 0;
   const VkGraphicsPipelineShaderGroupsCreateInfoNV *groupinfo = vk_find_struct_const(pCreateInfo, GRAPHICS_PIPELINE_SHADER_GROUPS_CREATE_INFO_NV);
   if (!group && groupinfo)
            pipeline = vk_zalloc(&device->vk.alloc, sizeof(*pipeline) + size, 8,
         if (pipeline == NULL)
            vk_object_base_init(&device->vk, &pipeline->base,
         uint64_t t0 = os_time_get_nano();
   result = lvp_graphics_pipeline_init(pipeline, device, cache, pCreateInfo, flags);
   if (result != VK_SUCCESS) {
      vk_free(&device->vk.alloc, pipeline);
      }
   if (!group && groupinfo) {
      VkGraphicsPipelineCreateInfo pci = *pCreateInfo;
   for (unsigned i = 0; i < groupinfo->groupCount; i++) {
      const VkGraphicsShaderGroupCreateInfoNV *g = &groupinfo->pGroups[i];
   pci.pVertexInputState = g->pVertexInputState;
   pci.pTessellationState = g->pTessellationState;
   pci.pStages = g->pStages;
   pci.stageCount = g->stageCount;
   result = lvp_graphics_pipeline_create(_device, _cache, &pci, flags, &pipeline->groups[i], true);
   if (result != VK_SUCCESS) {
      lvp_pipeline_destroy(device, pipeline, false);
      }
      }
   for (unsigned i = 0; i < groupinfo->pipelineCount; i++)
                     VkPipelineCreationFeedbackCreateInfo *feedback = (void*)vk_find_struct_const(pCreateInfo->pNext, PIPELINE_CREATION_FEEDBACK_CREATE_INFO);
   if (feedback && !group) {
      feedback->pPipelineCreationFeedback->duration = os_time_get_nano() - t0;
   feedback->pPipelineCreationFeedback->flags = VK_PIPELINE_CREATION_FEEDBACK_VALID_BIT;
                           }
      static VkPipelineCreateFlagBits2KHR
   get_pipeline_create_flags(const void *pCreateInfo)
   {
      const VkBaseInStructure *base = pCreateInfo;
   const VkPipelineCreateFlags2CreateInfoKHR *flags2 =
            if (flags2)
            switch (((VkBaseInStructure *)pCreateInfo)->sType) {
   case VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO: {
      const VkGraphicsPipelineCreateInfo *create_info = (VkGraphicsPipelineCreateInfo *)pCreateInfo;
      }
   case VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO: {
      const VkComputePipelineCreateInfo *create_info = (VkComputePipelineCreateInfo *)pCreateInfo;
      }
   case VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR: {
      const VkRayTracingPipelineCreateInfoKHR *create_info = (VkRayTracingPipelineCreateInfoKHR *)pCreateInfo;
         #ifdef VK_ENABLE_BETA_EXTENSIONS
      case VK_STRUCTURE_TYPE_EXECUTION_GRAPH_PIPELINE_CREATE_INFO_AMDX: {
      const VkExecutionGraphPipelineCreateInfoAMDX *create_info = (VkExecutionGraphPipelineCreateInfoAMDX *)pCreateInfo;
         #endif
      default:
                     }
      VKAPI_ATTR VkResult VKAPI_CALL lvp_CreateGraphicsPipelines(
      VkDevice                                    _device,
   VkPipelineCache                             pipelineCache,
   uint32_t                                    count,
   const VkGraphicsPipelineCreateInfo*         pCreateInfos,
   const VkAllocationCallbacks*                pAllocator,
      {
      VkResult result = VK_SUCCESS;
            for (; i < count; i++) {
      VkResult r = VK_PIPELINE_COMPILE_REQUIRED;
            if (!(flags & VK_PIPELINE_CREATE_2_FAIL_ON_PIPELINE_COMPILE_REQUIRED_BIT_KHR))
      r = lvp_graphics_pipeline_create(_device,
                              if (r != VK_SUCCESS) {
      result = r;
   pPipelines[i] = VK_NULL_HANDLE;
   if (flags & VK_PIPELINE_CREATE_2_EARLY_RETURN_ON_FAILURE_BIT_KHR)
         }
   if (result != VK_SUCCESS) {
      for (; i < count; i++)
                  }
      static VkResult
   lvp_compute_pipeline_init(struct lvp_pipeline *pipeline,
                     {
      pipeline->device = device;
   pipeline->layout = lvp_pipeline_layout_from_handle(pCreateInfo->layout);
   vk_pipeline_layout_ref(&pipeline->layout->vk);
                     VkResult result = lvp_shader_compile_to_ir(pipeline, &pCreateInfo->stage);
   if (result != VK_SUCCESS)
            struct lvp_shader *shader = &pipeline->shaders[MESA_SHADER_COMPUTE];
   if (!shader->inlines.can_inline)
         pipeline->compiled = true;
      }
      static VkResult
   lvp_compute_pipeline_create(
      VkDevice _device,
   VkPipelineCache _cache,
   const VkComputePipelineCreateInfo *pCreateInfo,
   VkPipelineCreateFlagBits2KHR flags,
      {
      LVP_FROM_HANDLE(lvp_device, device, _device);
   LVP_FROM_HANDLE(lvp_pipeline_cache, cache, _cache);
   struct lvp_pipeline *pipeline;
                     pipeline = vk_zalloc(&device->vk.alloc, sizeof(*pipeline), 8,
         if (pipeline == NULL)
            vk_object_base_init(&device->vk, &pipeline->base,
         uint64_t t0 = os_time_get_nano();
   result = lvp_compute_pipeline_init(pipeline, device, cache, pCreateInfo);
   if (result != VK_SUCCESS) {
      vk_free(&device->vk.alloc, pipeline);
               const VkPipelineCreationFeedbackCreateInfo *feedback = (void*)vk_find_struct_const(pCreateInfo->pNext, PIPELINE_CREATION_FEEDBACK_CREATE_INFO);
   if (feedback) {
      feedback->pPipelineCreationFeedback->duration = os_time_get_nano() - t0;
   feedback->pPipelineCreationFeedback->flags = VK_PIPELINE_CREATION_FEEDBACK_VALID_BIT;
                           }
      VKAPI_ATTR VkResult VKAPI_CALL lvp_CreateComputePipelines(
      VkDevice                                    _device,
   VkPipelineCache                             pipelineCache,
   uint32_t                                    count,
   const VkComputePipelineCreateInfo*          pCreateInfos,
   const VkAllocationCallbacks*                pAllocator,
      {
      VkResult result = VK_SUCCESS;
            for (; i < count; i++) {
      VkResult r = VK_PIPELINE_COMPILE_REQUIRED;
            if (!(flags & VK_PIPELINE_CREATE_2_FAIL_ON_PIPELINE_COMPILE_REQUIRED_BIT_KHR))
      r = lvp_compute_pipeline_create(_device,
                        if (r != VK_SUCCESS) {
      result = r;
   pPipelines[i] = VK_NULL_HANDLE;
   if (flags & VK_PIPELINE_CREATE_2_EARLY_RETURN_ON_FAILURE_BIT_KHR)
         }
   if (result != VK_SUCCESS) {
      for (; i < count; i++)
                     }
      VKAPI_ATTR void VKAPI_CALL lvp_DestroyShaderEXT(
      VkDevice                                    _device,
   VkShaderEXT                                 _shader,
      {
      LVP_FROM_HANDLE(lvp_device, device, _device);
            if (!shader)
                  vk_pipeline_layout_unref(&device->vk, &shader->layout->vk);
   blob_finish(&shader->blob);
   vk_object_base_finish(&shader->base);
      }
      static VkShaderEXT
   create_shader_object(struct lvp_device *device, const VkShaderCreateInfoEXT *pCreateInfo, const VkAllocationCallbacks *pAllocator)
   {
      nir_shader *nir = NULL;
   gl_shader_stage stage = vk_to_mesa_shader_stage(pCreateInfo->stage);
   assert(stage <= LVP_SHADER_STAGES && stage != MESA_SHADER_NONE);
   if (pCreateInfo->codeType == VK_SHADER_CODE_TYPE_SPIRV_EXT) {
      VkShaderModuleCreateInfo minfo = {
      VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
   NULL,
   0,
   pCreateInfo->codeSize,
      };
   VkPipelineShaderStageCreateFlagBits flags = 0;
   if (pCreateInfo->flags & VK_SHADER_CREATE_ALLOW_VARYING_SUBGROUP_SIZE_BIT_EXT)
         if (pCreateInfo->flags & VK_SHADER_CREATE_REQUIRE_FULL_SUBGROUPS_BIT_EXT)
         VkPipelineShaderStageCreateInfo sinfo = {
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
   &minfo,
   flags,
   pCreateInfo->stage,
   VK_NULL_HANDLE,
   pCreateInfo->pName,
      };
   VkResult result = compile_spirv(device, &sinfo, &nir);
   if (result != VK_SUCCESS)
            } else {
      assert(pCreateInfo->codeType == VK_SHADER_CODE_TYPE_BINARY_EXT);
   if (pCreateInfo->codeSize < SHA1_DIGEST_LENGTH + VK_UUID_SIZE + 1)
         struct blob_reader blob;
   const uint8_t *data = pCreateInfo->pCode;
   uint8_t uuid[VK_UUID_SIZE];
   lvp_device_get_cache_uuid(uuid);
   if (memcmp(uuid, data, VK_UUID_SIZE))
         size_t size = pCreateInfo->codeSize - SHA1_DIGEST_LENGTH - VK_UUID_SIZE;
            struct mesa_sha1 sctx;
   _mesa_sha1_init(&sctx);
   _mesa_sha1_update(&sctx, data + SHA1_DIGEST_LENGTH + VK_UUID_SIZE, size);
   _mesa_sha1_final(&sctx, sha1);
   if (memcmp(sha1, data + VK_UUID_SIZE, SHA1_DIGEST_LENGTH))
            blob_reader_init(&blob, data + SHA1_DIGEST_LENGTH + VK_UUID_SIZE, size);
   nir = nir_deserialize(NULL, device->pscreen->get_compiler_options(device->pscreen, PIPE_SHADER_IR_NIR, stage), &blob);
   if (!nir)
      }
   if (!nir_shader_get_entrypoint(nir))
         struct lvp_shader *shader = vk_object_zalloc(&device->vk, pAllocator, sizeof(struct lvp_shader), VK_OBJECT_TYPE_SHADER_EXT);
   if (!shader)
         blob_init(&shader->blob);
   VkPipelineLayoutCreateInfo pci = {
      VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
   NULL,
   0,
   pCreateInfo->setLayoutCount,
   pCreateInfo->pSetLayouts,
   pCreateInfo->pushConstantRangeCount,
      };
            if (pCreateInfo->codeType == VK_SHADER_CODE_TYPE_SPIRV_EXT)
                     lvp_shader_xfb_init(shader);
   if (stage == MESA_SHADER_TESS_EVAL) {
      /* spec requires that all tess modes are set in both shaders */
   nir_lower_patch_vertices(shader->pipeline_nir->nir, shader->pipeline_nir->nir->info.tess.tcs_vertices_out, NULL);
   shader->tess_ccw = create_pipeline_nir(nir_shader_clone(NULL, shader->pipeline_nir->nir));
   shader->tess_ccw->nir->info.tess.ccw = !shader->pipeline_nir->nir->info.tess.ccw;
      } else if (stage == MESA_SHADER_FRAGMENT && nir->info.fs.uses_fbfetch_output) {
      /* this is (currently) illegal */
   assert(!nir->info.fs.uses_fbfetch_output);
            vk_object_base_finish(&shader->base);
   vk_free2(&device->vk.alloc, pAllocator, shader);
      }
   nir_serialize(&shader->blob, nir, true);
   shader->shader_cso = lvp_shader_compile(device, shader, nir_shader_clone(NULL, nir), false);
      fail:
      ralloc_free(nir);
      }
      VKAPI_ATTR VkResult VKAPI_CALL lvp_CreateShadersEXT(
      VkDevice                                    _device,
   uint32_t                                    createInfoCount,
   const VkShaderCreateInfoEXT*                pCreateInfos,
   const VkAllocationCallbacks*                pAllocator,
      {
      LVP_FROM_HANDLE(lvp_device, device, _device);
   unsigned i;
   for (i = 0; i < createInfoCount; i++) {
      pShaders[i] = create_shader_object(device, &pCreateInfos[i], pAllocator);
   if (!pShaders[i]) {
      if (pCreateInfos[i].codeType == VK_SHADER_CODE_TYPE_BINARY_EXT) {
      if (i < createInfoCount - 1)
            }
         }
      }
         VKAPI_ATTR VkResult VKAPI_CALL lvp_GetShaderBinaryDataEXT(
      VkDevice                                    device,
   VkShaderEXT                                 _shader,
   size_t*                                     pDataSize,
      {
      LVP_FROM_HANDLE(lvp_shader, shader, _shader);
   VkResult ret = VK_SUCCESS;
   if (pData) {
      if (*pDataSize < shader->blob.size + SHA1_DIGEST_LENGTH + VK_UUID_SIZE) {
      ret = VK_INCOMPLETE;
      } else {
      *pDataSize = MIN2(*pDataSize, shader->blob.size + SHA1_DIGEST_LENGTH + VK_UUID_SIZE);
   uint8_t *data = pData;
   lvp_device_get_cache_uuid(data);
   struct mesa_sha1 sctx;
   _mesa_sha1_init(&sctx);
   _mesa_sha1_update(&sctx, shader->blob.data, shader->blob.size);
   _mesa_sha1_final(&sctx, data + VK_UUID_SIZE);
         } else {
         }
      }
      #ifdef VK_ENABLE_BETA_EXTENSIONS
   static VkResult
   lvp_exec_graph_pipeline_create(VkDevice _device, VkPipelineCache _cache,
                     {
      LVP_FROM_HANDLE(lvp_device, device, _device);
   struct lvp_pipeline *pipeline;
                     uint32_t stage_count = create_info->stageCount;
   if (create_info->pLibraryInfo) {
      for (uint32_t i = 0; i < create_info->pLibraryInfo->libraryCount; i++) {
      VK_FROM_HANDLE(lvp_pipeline, library, create_info->pLibraryInfo->pLibraries[i]);
                  pipeline = vk_zalloc(&device->vk.alloc, sizeof(*pipeline) + stage_count * sizeof(VkPipeline), 8,
         if (!pipeline)
            vk_object_base_init(&device->vk, &pipeline->base,
                     pipeline->type = LVP_PIPELINE_EXEC_GRAPH;
            pipeline->exec_graph.scratch_size = 0;
            uint32_t stage_index = 0;
   for (uint32_t i = 0; i < create_info->stageCount; i++) {
      const VkPipelineShaderStageNodeCreateInfoAMDX *node_info = vk_find_struct_const(
            VkComputePipelineCreateInfo stage_create_info = {
      .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
   .flags = create_info->flags,
   .stage = create_info->pStages[i],
               result = lvp_compute_pipeline_create(_device, _cache, &stage_create_info, flags, &pipeline->groups[i]);
   if (result != VK_SUCCESS)
            VK_FROM_HANDLE(lvp_pipeline, stage, pipeline->groups[i]);
            if (node_info) {
      stage->exec_graph.name = node_info->pName;
               /* TODO: Add a shader info NIR pass to figure out how many the payloads the shader creates. */
   stage->exec_graph.scratch_size = nir->info.cs.node_payloads_size * 256;
                        if (create_info->pLibraryInfo) {
      for (uint32_t i = 0; i < create_info->pLibraryInfo->libraryCount; i++) {
      VK_FROM_HANDLE(lvp_pipeline, library, create_info->pLibraryInfo->pLibraries[i]);
   for (uint32_t j = 0; j < library->num_groups; j++) {
      /* TODO: Do we need reference counting? */
   pipeline->groups[stage_index] = library->groups[j];
      }
                  const VkPipelineCreationFeedbackCreateInfo *feedback = (void*)vk_find_struct_const(create_info->pNext, PIPELINE_CREATION_FEEDBACK_CREATE_INFO);
   if (feedback) {
      feedback->pPipelineCreationFeedback->duration = os_time_get_nano() - t0;
   feedback->pPipelineCreationFeedback->flags = VK_PIPELINE_CREATION_FEEDBACK_VALID_BIT;
                              fail:
      for (uint32_t i = 0; i < stage_count; i++)
                        }
      VKAPI_ATTR VkResult VKAPI_CALL
   lvp_CreateExecutionGraphPipelinesAMDX(VkDevice device, VkPipelineCache pipelineCache,
                           {
      VkResult result = VK_SUCCESS;
            for (; i < createInfoCount; i++) {
               VkResult r = VK_PIPELINE_COMPILE_REQUIRED;
   if (!(flags & VK_PIPELINE_CREATE_2_FAIL_ON_PIPELINE_COMPILE_REQUIRED_BIT_KHR))
         if (r != VK_SUCCESS) {
      result = r;
   pPipelines[i] = VK_NULL_HANDLE;
   if (flags & VK_PIPELINE_CREATE_2_EARLY_RETURN_ON_FAILURE_BIT_KHR)
         }
   if (result != VK_SUCCESS) {
      for (; i < createInfoCount; i++)
                  }
      VKAPI_ATTR VkResult VKAPI_CALL
   lvp_GetExecutionGraphPipelineScratchSizeAMDX(VkDevice device, VkPipeline executionGraph,
         {
      VK_FROM_HANDLE(lvp_pipeline, pipeline, executionGraph);
   pSizeInfo->size = MAX2(pipeline->exec_graph.scratch_size * 32, 16);
      }
      VKAPI_ATTR VkResult VKAPI_CALL
   lvp_GetExecutionGraphPipelineNodeIndexAMDX(VkDevice device, VkPipeline executionGraph,
               {
               for (uint32_t i = 0; i < pipeline->num_groups; i++) {
      VK_FROM_HANDLE(lvp_pipeline, stage, pipeline->groups[i]);
   if (stage->exec_graph.index == pNodeInfo->index &&
      !strcmp(stage->exec_graph.name, pNodeInfo->pName)) {
   *pNodeIndex = i;
                     }
   #endif
