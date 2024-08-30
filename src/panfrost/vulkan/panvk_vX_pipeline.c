   /*
   * Copyright © 2021 Collabora Ltd.
   *
   * Derived from tu_pipeline.c which is:
   * Copyright © 2016 Red Hat.
   * Copyright © 2016 Bas Nieuwenhuizen
   * Copyright © 2015 Intel Corporation
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
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   * DEALINGS IN THE SOFTWARE.
   */
      #include "panvk_cs.h"
   #include "panvk_private.h"
      #include "pan_bo.h"
      #include "nir/nir.h"
   #include "nir/nir_builder.h"
   #include "spirv/nir_spirv.h"
   #include "util/blend.h"
   #include "util/mesa-sha1.h"
   #include "util/u_atomic.h"
   #include "util/u_debug.h"
   #include "vk_blend.h"
   #include "vk_format.h"
   #include "vk_util.h"
      #include "panfrost/util/pan_lower_framebuffer.h"
      struct panvk_pipeline_builder {
      struct panvk_device *device;
   struct panvk_pipeline_cache *cache;
   const VkAllocationCallbacks *alloc;
   struct {
      const VkGraphicsPipelineCreateInfo *gfx;
      } create_info;
            struct panvk_shader *shaders[MESA_SHADER_STAGES];
   struct {
      uint32_t shader_offset;
      } stages[MESA_SHADER_STAGES];
   uint32_t blend_shader_offsets[MAX_RTS];
   uint32_t shader_total_size;
   uint32_t static_state_size;
            bool rasterizer_discard;
   /* these states are affectd by rasterizer_discard */
   VkSampleCountFlagBits samples;
   bool use_depth_stencil_attachment;
   uint8_t active_color_attachments;
      };
      static VkResult
   panvk_pipeline_builder_create_pipeline(struct panvk_pipeline_builder *builder,
         {
               struct panvk_pipeline *pipeline = vk_object_zalloc(
         if (!pipeline)
            pipeline->layout = builder->layout;
   *out_pipeline = pipeline;
      }
      static void
   panvk_pipeline_builder_finish(struct panvk_pipeline_builder *builder)
   {
      for (uint32_t i = 0; i < MESA_SHADER_STAGES; i++) {
      if (!builder->shaders[i])
         panvk_shader_destroy(builder->device, builder->shaders[i],
         }
      static bool
   panvk_pipeline_static_state(struct panvk_pipeline *pipeline, uint32_t id)
   {
         }
      static VkResult
   panvk_pipeline_builder_compile_shaders(struct panvk_pipeline_builder *builder,
         {
      const VkPipelineShaderStageCreateInfo *stage_infos[MESA_SHADER_STAGES] = {
         const VkPipelineShaderStageCreateInfo *stages =
      builder->create_info.gfx ? builder->create_info.gfx->pStages
      unsigned stage_count =
            for (uint32_t i = 0; i < stage_count; i++) {
      gl_shader_stage stage = vk_to_mesa_shader_stage(stages[i].stage);
               /* compile shaders in reverse order */
   for (gl_shader_stage stage = MESA_SHADER_STAGES - 1;
      stage > MESA_SHADER_NONE; stage--) {
   const VkPipelineShaderStageCreateInfo *stage_info = stage_infos[stage];
   if (!stage_info)
                     shader = panvk_per_arch(shader_create)(
      builder->device, stage, stage_info, builder->layout,
   PANVK_SYSVAL_UBO_INDEX, &pipeline->blend.state,
   panvk_pipeline_static_state(pipeline,
            if (!shader)
            builder->shaders[stage] = shader;
   builder->shader_total_size = ALIGN_POT(builder->shader_total_size, 128);
   builder->stages[stage].shader_offset = builder->shader_total_size;
   builder->shader_total_size +=
                  }
      static VkResult
   panvk_pipeline_builder_upload_shaders(struct panvk_pipeline_builder *builder,
         {
      /* In some cases, the optimized shader is empty. Don't bother allocating
   * anything in this case.
   */
   if (builder->shader_total_size == 0)
            struct panfrost_bo *bin_bo =
      panfrost_bo_create(&builder->device->physical_device->pdev,
         pipeline->binary_bo = bin_bo;
            for (uint32_t i = 0; i < MESA_SHADER_STAGES; i++) {
      const struct panvk_shader *shader = builder->shaders[i];
   if (!shader)
            memcpy(pipeline->binary_bo->ptr.cpu + builder->stages[i].shader_offset,
                        }
      static void
   panvk_pipeline_builder_alloc_static_state_bo(
         {
      struct panfrost_device *pdev = &builder->device->physical_device->pdev;
            for (uint32_t i = 0; i < MESA_SHADER_STAGES; i++) {
      const struct panvk_shader *shader = builder->shaders[i];
   if (!shader && i != MESA_SHADER_FRAGMENT)
            if (pipeline->fs.dynamic_rsd && i == MESA_SHADER_FRAGMENT)
            bo_size = ALIGN_POT(bo_size, pan_alignment(RENDERER_STATE));
   builder->stages[i].rsd_offset = bo_size;
   bo_size += pan_size(RENDERER_STATE);
   if (i == MESA_SHADER_FRAGMENT)
               if (builder->create_info.gfx &&
      panvk_pipeline_static_state(pipeline, VK_DYNAMIC_STATE_VIEWPORT) &&
   panvk_pipeline_static_state(pipeline, VK_DYNAMIC_STATE_SCISSOR)) {
   bo_size = ALIGN_POT(bo_size, pan_alignment(VIEWPORT));
   builder->vpd_offset = bo_size;
               if (bo_size) {
      pipeline->state_bo =
               }
      static void
   panvk_pipeline_builder_init_sysvals(struct panvk_pipeline_builder *builder,
               {
                  }
      static void
   panvk_pipeline_builder_init_shaders(struct panvk_pipeline_builder *builder,
         {
      for (uint32_t i = 0; i < MESA_SHADER_STAGES; i++) {
      const struct panvk_shader *shader = builder->shaders[i];
   if (!shader)
            pipeline->tls_size = MAX2(pipeline->tls_size, shader->info.tls_size);
            if (shader->has_img_access)
            if (i == MESA_SHADER_VERTEX && shader->info.vs.writes_point_size) {
      VkPrimitiveTopology topology =
                  /* Even if the vertex shader writes point size, we only consider the
   * pipeline to write point size when we're actually drawing points.
   * Otherwise the point size write would conflict with wide lines.
   */
                        /* Handle empty shaders gracefully */
   if (util_dynarray_num_elements(&builder->shaders[i]->binary, uint8_t)) {
      shader_ptr =
               if (i != MESA_SHADER_FRAGMENT) {
      void *rsd =
                        panvk_per_arch(emit_non_fs_rsd)(builder->device, &shader->info,
                              if (i == MESA_SHADER_COMPUTE)
               if (builder->create_info.gfx && !pipeline->fs.dynamic_rsd) {
      void *rsd = pipeline->state_bo->ptr.cpu +
         mali_ptr gpu_rsd = pipeline->state_bo->ptr.gpu +
                  panvk_per_arch(emit_base_fs_rsd)(builder->device, pipeline, rsd);
   for (unsigned rt = 0; rt < pipeline->blend.state.rt_count; rt++) {
      panvk_per_arch(emit_blend)(builder->device, pipeline, rt, bd);
                  } else if (builder->create_info.gfx) {
      panvk_per_arch(emit_base_fs_rsd)(builder->device, pipeline,
         for (unsigned rt = 0; rt < MAX2(pipeline->blend.state.rt_count, 1);
      rt++) {
   panvk_per_arch(emit_blend)(builder->device, pipeline, rt,
                  pipeline->num_ubos = PANVK_NUM_BUILTIN_UBOS + builder->layout->num_ubos +
      }
      static void
   panvk_pipeline_builder_parse_viewport(struct panvk_pipeline_builder *builder,
         {
      /* The spec says:
   *
   *    pViewportState is a pointer to an instance of the
   *    VkPipelineViewportStateCreateInfo structure, and is ignored if the
   *    pipeline has rasterization disabled.
   */
   if (!builder->rasterizer_discard &&
      panvk_pipeline_static_state(pipeline, VK_DYNAMIC_STATE_VIEWPORT) &&
   panvk_pipeline_static_state(pipeline, VK_DYNAMIC_STATE_SCISSOR)) {
   void *vpd = pipeline->state_bo->ptr.cpu + builder->vpd_offset;
   panvk_per_arch(emit_viewport)(
      builder->create_info.gfx->pViewportState->pViewports,
         }
   if (panvk_pipeline_static_state(pipeline, VK_DYNAMIC_STATE_VIEWPORT))
      pipeline->viewport =
         if (panvk_pipeline_static_state(pipeline, VK_DYNAMIC_STATE_SCISSOR))
      pipeline->scissor =
   }
      static void
   panvk_pipeline_builder_parse_dynamic(struct panvk_pipeline_builder *builder,
         {
      const VkPipelineDynamicStateCreateInfo *dynamic_info =
            if (!dynamic_info)
            for (uint32_t i = 0; i < dynamic_info->dynamicStateCount; i++) {
      VkDynamicState state = dynamic_info->pDynamicStates[i];
   switch (state) {
   case VK_DYNAMIC_STATE_VIEWPORT ... VK_DYNAMIC_STATE_STENCIL_REFERENCE:
      pipeline->dynamic_state_mask |= 1 << state;
      default:
               }
      static enum mali_draw_mode
   translate_prim_topology(VkPrimitiveTopology in)
   {
      switch (in) {
   case VK_PRIMITIVE_TOPOLOGY_POINT_LIST:
         case VK_PRIMITIVE_TOPOLOGY_LINE_LIST:
         case VK_PRIMITIVE_TOPOLOGY_LINE_STRIP:
         case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST:
         case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP:
         case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN:
         case VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY:
   case VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY:
   case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY:
   case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY:
   case VK_PRIMITIVE_TOPOLOGY_PATCH_LIST:
   default:
            }
      static void
   panvk_pipeline_builder_parse_input_assembly(
         {
      pipeline->ia.primitive_restart =
         pipeline->ia.topology = translate_prim_topology(
      }
      bool
   panvk_per_arch(blend_needs_lowering)(const struct panfrost_device *dev,
               {
      /* LogicOp requires a blend shader */
   if (state->logicop_enable)
            /* Not all formats can be blended by fixed-function hardware */
   if (!panfrost_blendable_formats_v7[state->rts[rt].format].internal)
                     /* v6 doesn't support blend constants in FF blend equations.
   * v7 only uses the constant from RT 0 (TODO: what if it's the same
   * constant? or a constant is shared?)
   */
   if (constant_mask && (PAN_ARCH == 6 || (PAN_ARCH == 7 && rt > 0)))
            if (!pan_blend_is_homogenous_constant(constant_mask, state->constants))
            bool supports_2src = pan_blend_supports_2src(dev->arch);
      }
      static void
   panvk_pipeline_builder_parse_color_blend(struct panvk_pipeline_builder *builder,
         {
      struct panfrost_device *pdev = &builder->device->physical_device->pdev;
   pipeline->blend.state.logicop_enable =
         pipeline->blend.state.logicop_func =
         pipeline->blend.state.rt_count =
         memcpy(pipeline->blend.state.constants,
                  for (unsigned i = 0; i < pipeline->blend.state.rt_count; i++) {
      const VkPipelineColorBlendAttachmentState *in =
                                    out->nr_samples =
         out->equation.blend_enable = in->blendEnable;
   out->equation.color_mask = in->colorWriteMask;
   out->equation.rgb_func = vk_blend_op_to_pipe(in->colorBlendOp);
   out->equation.rgb_src_factor =
         out->equation.rgb_dst_factor =
         out->equation.alpha_func = vk_blend_op_to_pipe(in->alphaBlendOp);
   out->equation.alpha_src_factor =
         out->equation.alpha_dst_factor =
            if (!dest_has_alpha) {
      out->equation.rgb_src_factor =
                        out->equation.alpha_src_factor =
         out->equation.alpha_dst_factor =
                        unsigned constant_mask =
      panvk_per_arch(blend_needs_lowering)(pdev, &pipeline->blend.state, i)
      ? 0
   pipeline->blend.constant[i].index = ffs(constant_mask) - 1;
   if (constant_mask) {
      /* On Bifrost, the blend constant is expressed with a UNORM of the
   * size of the target format. The value is then shifted such that
   * used bits are in the MSB. Here we calculate the factor at pipeline
   * creation time so we only have to do a
   *   hw_constant = float_constant * factor;
   * at descriptor emission time.
   */
   const struct util_format_description *format_desc =
         unsigned chan_size = 0;
   for (unsigned c = 0; c < format_desc->nr_channels; c++)
         pipeline->blend.constant[i].bifrost_factor = ((1 << chan_size) - 1)
            }
      static void
   panvk_pipeline_builder_parse_multisample(struct panvk_pipeline_builder *builder,
         {
      unsigned nr_samples = MAX2(
            pipeline->ms.rast_samples =
         pipeline->ms.sample_mask =
      builder->create_info.gfx->pMultisampleState->pSampleMask
      ? builder->create_info.gfx->pMultisampleState->pSampleMask[0]
   pipeline->ms.min_samples =
      MAX2(builder->create_info.gfx->pMultisampleState->minSampleShading *
         }
      static enum mali_stencil_op
   translate_stencil_op(VkStencilOp in)
   {
      switch (in) {
   case VK_STENCIL_OP_KEEP:
         case VK_STENCIL_OP_ZERO:
         case VK_STENCIL_OP_REPLACE:
         case VK_STENCIL_OP_INCREMENT_AND_CLAMP:
         case VK_STENCIL_OP_DECREMENT_AND_CLAMP:
         case VK_STENCIL_OP_INCREMENT_AND_WRAP:
         case VK_STENCIL_OP_DECREMENT_AND_WRAP:
         case VK_STENCIL_OP_INVERT:
         default:
            }
      static void
   panvk_pipeline_builder_parse_zs(struct panvk_pipeline_builder *builder,
         {
      if (!builder->use_depth_stencil_attachment)
            pipeline->zs.z_test =
            /* The Vulkan spec says:
   *
   *    depthWriteEnable controls whether depth writes are enabled when
   *    depthTestEnable is VK_TRUE. Depth writes are always disabled when
   *    depthTestEnable is VK_FALSE.
   *
   * The hardware does not make this distinction, though, so we AND in the
   * condition ourselves.
   */
   pipeline->zs.z_write =
      pipeline->zs.z_test &&
         pipeline->zs.z_compare_func = panvk_per_arch(translate_compare_func)(
         pipeline->zs.s_test =
         pipeline->zs.s_front.fail_op = translate_stencil_op(
         pipeline->zs.s_front.pass_op = translate_stencil_op(
         pipeline->zs.s_front.z_fail_op = translate_stencil_op(
         pipeline->zs.s_front.compare_func = panvk_per_arch(translate_compare_func)(
         pipeline->zs.s_front.compare_mask =
         pipeline->zs.s_front.write_mask =
         pipeline->zs.s_front.ref =
         pipeline->zs.s_back.fail_op = translate_stencil_op(
         pipeline->zs.s_back.pass_op = translate_stencil_op(
         pipeline->zs.s_back.z_fail_op = translate_stencil_op(
         pipeline->zs.s_back.compare_func = panvk_per_arch(translate_compare_func)(
         pipeline->zs.s_back.compare_mask =
         pipeline->zs.s_back.write_mask =
         pipeline->zs.s_back.ref =
      }
      static void
   panvk_pipeline_builder_parse_rast(struct panvk_pipeline_builder *builder,
         {
      pipeline->rast.clamp_depth =
         pipeline->rast.depth_bias.enable =
         pipeline->rast.depth_bias.constant_factor =
         pipeline->rast.depth_bias.clamp =
         pipeline->rast.depth_bias.slope_factor =
         pipeline->rast.front_ccw =
      builder->create_info.gfx->pRasterizationState->frontFace ==
      pipeline->rast.cull_front_face =
      builder->create_info.gfx->pRasterizationState->cullMode &
      pipeline->rast.cull_back_face =
      builder->create_info.gfx->pRasterizationState->cullMode &
      pipeline->rast.line_width =
         pipeline->rast.enable =
      }
      static bool
   panvk_fs_required(struct panvk_pipeline *pipeline)
   {
               /* If we generally have side effects */
   if (info->fs.sidefx)
            /* If colour is written we need to execute */
   const struct pan_blend_state *blend = &pipeline->blend.state;
   for (unsigned i = 0; i < blend->rt_count; ++i) {
      if (blend->rts[i].equation.color_mask)
               /* If depth is written and not implied we need to execute.
   * TODO: Predicate on Z/S writes being enabled */
      }
      #define PANVK_DYNAMIC_FS_RSD_MASK                                              \
      ((1 << VK_DYNAMIC_STATE_DEPTH_BIAS) |                                       \
   (1 << VK_DYNAMIC_STATE_BLEND_CONSTANTS) |                                  \
   (1 << VK_DYNAMIC_STATE_STENCIL_COMPARE_MASK) |                             \
   (1 << VK_DYNAMIC_STATE_STENCIL_WRITE_MASK) |                               \
         static void
   panvk_pipeline_builder_init_fs_state(struct panvk_pipeline_builder *builder,
         {
      if (!builder->shaders[MESA_SHADER_FRAGMENT])
            pipeline->fs.dynamic_rsd =
         pipeline->fs.address = pipeline->binary_bo->ptr.gpu +
         pipeline->fs.info = builder->shaders[MESA_SHADER_FRAGMENT]->info;
   pipeline->fs.rt_mask = builder->active_color_attachments;
      }
      static void
   panvk_pipeline_update_varying_slot(struct panvk_varyings_info *varyings,
                     {
      gl_varying_slot loc = varying->location;
                              enum pipe_format new_fmt = varying->format;
                     /* We expect inputs to either be set by a previous stage or be built
   * in, skip the entry if that's not the case, we'll emit a const
   * varying returning zero for those entries.
   */
   if (input && old_fmt == PIPE_FORMAT_NONE)
            unsigned new_size = util_format_get_blocksize(new_fmt);
            if (old_size < new_size)
            /* Type (float or not) information is only known in the fragment shader, so
   * override for that
   */
   if (input) {
      assert(stage == MESA_SHADER_FRAGMENT && "no geom/tess on Bifrost");
                  }
      static void
   panvk_pipeline_builder_collect_varyings(struct panvk_pipeline_builder *builder,
         {
      for (uint32_t s = 0; s < MESA_SHADER_STAGES; s++) {
      if (!builder->shaders[s])
                     for (unsigned i = 0; i < info->varyings.input_count; i++) {
      panvk_pipeline_update_varying_slot(&pipeline->varyings, s,
               for (unsigned i = 0; i < info->varyings.output_count; i++) {
      panvk_pipeline_update_varying_slot(&pipeline->varyings, s,
                  /* TODO: Xfb */
   gl_varying_slot loc;
   BITSET_FOREACH_SET(loc, pipeline->varyings.active, VARYING_SLOT_MAX) {
      if (pipeline->varyings.varying[loc].format == PIPE_FORMAT_NONE)
            enum panvk_varying_buf_id buf_id = panvk_varying_buf_id(loc);
   unsigned buf_idx = panvk_varying_buf_index(&pipeline->varyings, buf_id);
            pipeline->varyings.varying[loc].buf = buf_idx;
   pipeline->varyings.varying[loc].offset =
               }
      static void
   panvk_pipeline_builder_parse_vertex_input(
         {
      struct panvk_attribs_info *attribs = &pipeline->attribs;
   const VkPipelineVertexInputStateCreateInfo *info =
            const VkPipelineVertexInputDivisorStateCreateInfoEXT *div_info =
      vk_find_struct_const(info->pNext,
         for (unsigned i = 0; i < info->vertexBindingDescriptionCount; i++) {
      const VkVertexInputBindingDescription *desc =
         attribs->buf_count = MAX2(desc->binding + 1, attribs->buf_count);
   attribs->buf[desc->binding].stride = desc->stride;
   attribs->buf[desc->binding].per_instance =
         attribs->buf[desc->binding].instance_divisor = 1;
               if (div_info) {
      for (unsigned i = 0; i < div_info->vertexBindingDivisorCount; i++) {
      const VkVertexInputBindingDivisorDescriptionEXT *div =
                        const struct pan_shader_info *vs =
            for (unsigned i = 0; i < info->vertexAttributeDescriptionCount; i++) {
      const VkVertexInputAttributeDescription *desc =
            unsigned attrib = desc->location + VERT_ATTRIB_GENERIC0;
   unsigned slot =
            attribs->attrib[slot].buf = desc->binding;
   attribs->attrib[slot].format = vk_format_to_pipe_format(desc->format);
               if (vs->attribute_count >= PAN_VERTEX_ID) {
      attribs->buf[attribs->buf_count].special = true;
   attribs->buf[attribs->buf_count].special_id = PAN_VERTEX_ID;
   attribs->attrib[PAN_VERTEX_ID].buf = attribs->buf_count++;
               if (vs->attribute_count >= PAN_INSTANCE_ID) {
      attribs->buf[attribs->buf_count].special = true;
   attribs->buf[attribs->buf_count].special_id = PAN_INSTANCE_ID;
   attribs->attrib[PAN_INSTANCE_ID].buf = attribs->buf_count++;
                  }
      static VkResult
   panvk_pipeline_builder_build(struct panvk_pipeline_builder *builder,
         {
      VkResult result = panvk_pipeline_builder_create_pipeline(builder, pipeline);
   if (result != VK_SUCCESS)
            /* TODO: make those functions return a result and handle errors */
   if (builder->create_info.gfx) {
      panvk_pipeline_builder_parse_dynamic(builder, *pipeline);
   panvk_pipeline_builder_parse_color_blend(builder, *pipeline);
   panvk_pipeline_builder_compile_shaders(builder, *pipeline);
   panvk_pipeline_builder_collect_varyings(builder, *pipeline);
   panvk_pipeline_builder_parse_input_assembly(builder, *pipeline);
   panvk_pipeline_builder_parse_multisample(builder, *pipeline);
   panvk_pipeline_builder_parse_zs(builder, *pipeline);
   panvk_pipeline_builder_parse_rast(builder, *pipeline);
   panvk_pipeline_builder_parse_vertex_input(builder, *pipeline);
   panvk_pipeline_builder_upload_shaders(builder, *pipeline);
   panvk_pipeline_builder_init_fs_state(builder, *pipeline);
   panvk_pipeline_builder_alloc_static_state_bo(builder, *pipeline);
   panvk_pipeline_builder_init_shaders(builder, *pipeline);
      } else {
      panvk_pipeline_builder_compile_shaders(builder, *pipeline);
   panvk_pipeline_builder_upload_shaders(builder, *pipeline);
   panvk_pipeline_builder_alloc_static_state_bo(builder, *pipeline);
                  }
      static void
   panvk_pipeline_builder_init_graphics(
      struct panvk_pipeline_builder *builder, struct panvk_device *dev,
   struct panvk_pipeline_cache *cache,
   const VkGraphicsPipelineCreateInfo *create_info,
      {
      VK_FROM_HANDLE(panvk_pipeline_layout, layout, create_info->layout);
   assert(layout);
   *builder = (struct panvk_pipeline_builder){
      .device = dev,
   .cache = cache,
   .layout = layout,
   .create_info.gfx = create_info,
               builder->rasterizer_discard =
            if (builder->rasterizer_discard) {
         } else {
               const struct panvk_render_pass *pass =
         const struct panvk_subpass *subpass =
            builder->use_depth_stencil_attachment =
            assert(subpass->color_count <=
         builder->active_color_attachments = 0;
   for (uint32_t i = 0; i < subpass->color_count; i++) {
      uint32_t idx = subpass->color_attachments[i].idx;
                  builder->active_color_attachments |= 1 << i;
            }
      VkResult
   panvk_per_arch(CreateGraphicsPipelines)(
      VkDevice device, VkPipelineCache pipelineCache, uint32_t count,
   const VkGraphicsPipelineCreateInfo *pCreateInfos,
      {
      VK_FROM_HANDLE(panvk_device, dev, device);
            for (uint32_t i = 0; i < count; i++) {
      struct panvk_pipeline_builder builder;
   panvk_pipeline_builder_init_graphics(&builder, dev, cache,
            struct panvk_pipeline *pipeline;
   VkResult result = panvk_pipeline_builder_build(&builder, &pipeline);
            if (result != VK_SUCCESS) {
      for (uint32_t j = 0; j < i; j++) {
      panvk_DestroyPipeline(device, pPipelines[j], pAllocator);
                                          }
      static void
   panvk_pipeline_builder_init_compute(
      struct panvk_pipeline_builder *builder, struct panvk_device *dev,
   struct panvk_pipeline_cache *cache,
   const VkComputePipelineCreateInfo *create_info,
      {
      VK_FROM_HANDLE(panvk_pipeline_layout, layout, create_info->layout);
   assert(layout);
   *builder = (struct panvk_pipeline_builder){
      .device = dev,
   .cache = cache,
   .layout = layout,
   .create_info.compute = create_info,
         }
      VkResult
   panvk_per_arch(CreateComputePipelines)(
      VkDevice device, VkPipelineCache pipelineCache, uint32_t count,
   const VkComputePipelineCreateInfo *pCreateInfos,
      {
      VK_FROM_HANDLE(panvk_device, dev, device);
            for (uint32_t i = 0; i < count; i++) {
      struct panvk_pipeline_builder builder;
   panvk_pipeline_builder_init_compute(&builder, dev, cache,
            struct panvk_pipeline *pipeline;
   VkResult result = panvk_pipeline_builder_build(&builder, &pipeline);
            if (result != VK_SUCCESS) {
      for (uint32_t j = 0; j < i; j++) {
      panvk_DestroyPipeline(device, pPipelines[j], pAllocator);
                                          }
