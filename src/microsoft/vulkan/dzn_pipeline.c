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
      #include "dzn_private.h"
      #include "spirv/nir_spirv.h"
      #include "dxil_nir.h"
   #include "nir_to_dxil.h"
   #include "dxil_spirv_nir.h"
   #include "spirv_to_dxil.h"
      #include "dxil_validator.h"
      #include "vk_alloc.h"
   #include "vk_util.h"
   #include "vk_format.h"
   #include "vk_pipeline.h"
   #include "vk_pipeline_cache.h"
      #include "util/u_debug.h"
      #define d3d12_pipeline_state_stream_new_desc(__stream, __maxstreamsz, __id, __type, __desc) \
      __type *__desc; \
   do { \
      struct { \
      D3D12_PIPELINE_STATE_SUBOBJECT_TYPE type; \
      } *__wrapper; \
   (__stream)->SizeInBytes = ALIGN_POT((__stream)->SizeInBytes, alignof(void *)); \
   __wrapper = (void *)((uint8_t *)(__stream)->pPipelineStateSubobjectStream + (__stream)->SizeInBytes); \
   (__stream)->SizeInBytes += sizeof(*__wrapper); \
   assert((__stream)->SizeInBytes <= __maxstreamsz); \
   __wrapper->type = __id; \
   __desc = &__wrapper->desc; \
            #define d3d12_pipeline_state_stream_new_desc_abbrev(__stream, __maxstreamsz, __id, __type, __desc) \
            #define d3d12_gfx_pipeline_state_stream_new_desc(__stream, __id, __type, __desc) \
            #define d3d12_compute_pipeline_state_stream_new_desc(__stream, __id, __type, __desc) \
            static bool
   gfx_pipeline_variant_key_equal(const void *a, const void *b)
   {
         }
      static uint32_t
   gfx_pipeline_variant_key_hash(const void *key)
   {
         }
      struct dzn_cached_blob {
      struct vk_pipeline_cache_object base;
   uint8_t hash[SHA1_DIGEST_LENGTH];
   const void *data;
      };
      static bool
   dzn_cached_blob_serialize(struct vk_pipeline_cache_object *object,
         {
      struct dzn_cached_blob *cached_blob =
            blob_write_bytes(blob, cached_blob->data, cached_blob->size);
      }
      static void
   dzn_cached_blob_destroy(struct vk_device *device,
         {
      struct dzn_cached_blob *shader =
               }
      static struct vk_pipeline_cache_object *
   dzn_cached_blob_create(struct vk_device *device,
                        static struct vk_pipeline_cache_object *
   dzn_cached_blob_deserialize(struct vk_pipeline_cache *cache,
               {
      size_t data_size = blob->end - blob->current;
            return dzn_cached_blob_create(cache->base.device, key_data,
      }
      const struct vk_pipeline_cache_object_ops dzn_cached_blob_ops = {
      .serialize = dzn_cached_blob_serialize,
   .deserialize = dzn_cached_blob_deserialize,
      };
         static struct vk_pipeline_cache_object *
   dzn_cached_blob_create(struct vk_device *device,
                     {
      VK_MULTIALLOC(ma);
   VK_MULTIALLOC_DECL(&ma, struct dzn_cached_blob, blob, 1);
            if (!vk_multialloc_alloc(&ma, &device->alloc,
                           vk_pipeline_cache_object_init(device, &blob->base,
                  if (data)
         blob->data = copy;
               }
      static VkResult
   dzn_graphics_pipeline_prepare_for_variants(struct dzn_device *device,
         {
      if (pipeline->variants)
            pipeline->variants =
      _mesa_hash_table_create(NULL,
            if (!pipeline->variants)
               }
      static dxil_spirv_shader_stage
   to_dxil_shader_stage(VkShaderStageFlagBits in)
   {
      switch (in) {
   case VK_SHADER_STAGE_VERTEX_BIT: return DXIL_SPIRV_SHADER_VERTEX;
   case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT: return DXIL_SPIRV_SHADER_TESS_CTRL;
   case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT: return DXIL_SPIRV_SHADER_TESS_EVAL;
   case VK_SHADER_STAGE_GEOMETRY_BIT: return DXIL_SPIRV_SHADER_GEOMETRY;
   case VK_SHADER_STAGE_FRAGMENT_BIT: return DXIL_SPIRV_SHADER_FRAGMENT;
   case VK_SHADER_STAGE_COMPUTE_BIT: return DXIL_SPIRV_SHADER_COMPUTE;
   default: unreachable("Unsupported stage");
      }
      struct dzn_nir_options {
      enum dxil_spirv_yz_flip_mode yz_flip_mode;
   uint16_t y_flip_mask, z_flip_mask;
   bool force_sample_rate_shading;
   bool lower_view_index;
   bool lower_view_index_to_rt_layer;
   enum pipe_format *vi_conversions;
   const nir_shader_compiler_options *nir_opts;
      };
      static VkResult
   dzn_pipeline_get_nir_shader(struct dzn_device *device,
                              const struct dzn_pipeline_layout *layout,
   struct vk_pipeline_cache *cache,
      {
      if (cache) {
      *nir = vk_pipeline_cache_lookup_nir(cache, hash, SHA1_DIGEST_LENGTH,
         if (*nir)
               struct dzn_physical_device *pdev =
         VK_FROM_HANDLE(vk_shader_module, module, stage_info->module);
            VkResult result =
      vk_shader_module_to_nir(&device->vk, module, stage,
            if (result != VK_SUCCESS)
            struct dxil_spirv_runtime_conf conf = {
      .runtime_data_cbv = {
      .register_space = DZN_REGISTER_SPACE_SYSVALS,
      },
   .push_constant_cbv = {
      .register_space = DZN_REGISTER_SPACE_PUSH_CONSTANT,
      },
   .zero_based_vertex_instance_id = false,
   .zero_based_compute_workgroup_id = false,
   .yz_flip = {
      .mode = options->yz_flip_mode,
   .y_mask = options->y_flip_mask,
      },
   .declared_read_only_images_as_srvs = !device->bindless,
   .inferred_read_only_images_as_srvs = !device->bindless,
   .force_sample_rate_shading = options->force_sample_rate_shading,
   .lower_view_index = options->lower_view_index,
   .lower_view_index_to_rt_layer = options->lower_view_index_to_rt_layer,
               bool requires_runtime_data;
            if (stage == MESA_SHADER_VERTEX) {
      bool needs_conv = false;
   for (uint32_t i = 0; i < MAX_VERTEX_GENERIC_ATTRIBS; i++) {
      if (options->vi_conversions[i] != PIPE_FORMAT_NONE)
               if (needs_conv)
      }
            if (cache)
               }
      static bool
   adjust_resource_index_binding(struct nir_builder *builder,
               {
      if (intrin->intrinsic != nir_intrinsic_vulkan_resource_index)
            const struct dzn_pipeline_layout *layout = cb_data;
   unsigned set = nir_intrinsic_desc_set(intrin);
            if (set >= layout->set_count ||
      binding >= layout->binding_translation[set].binding_count)
         binding = layout->binding_translation[set].base_reg[binding];
               }
      static void
   adjust_to_bindless_cb(struct dxil_spirv_binding_remapping *inout, void *context)
   {
      const struct dzn_pipeline_layout *layout = context;
   assert(inout->descriptor_set < layout->set_count);
   uint32_t new_binding = layout->binding_translation[inout->descriptor_set].base_reg[inout->binding];
   switch (layout->binding_translation[inout->descriptor_set].binding_class[inout->binding]) {
   case DZN_PIPELINE_BINDING_DYNAMIC_BUFFER:
      inout->descriptor_set = layout->set_count;
      case DZN_PIPELINE_BINDING_STATIC_SAMPLER:
      if (inout->is_sampler) {
      inout->descriptor_set = ~0;
      }
      case DZN_PIPELINE_BINDING_NORMAL:
      inout->binding = new_binding;
      default:
            }
      static bool
   adjust_var_bindings(nir_shader *shader,
                     {
      uint32_t modes = nir_var_image | nir_var_uniform | nir_var_mem_ubo | nir_var_mem_ssbo;
            if (bindings_hash)
            nir_foreach_variable_with_modes(var, shader, modes) {
      if (var->data.mode == nir_var_uniform) {
               if (!glsl_type_is_sampler(type) && !glsl_type_is_texture(type))
                        if (s >= layout->set_count)
            assert(b < layout->binding_translation[s].binding_count);
   if (!device->bindless)
            if (bindings_hash) {
      _mesa_sha1_update(&bindings_hash_ctx, &s, sizeof(s));
   _mesa_sha1_update(&bindings_hash_ctx, &b, sizeof(b));
                  if (bindings_hash)
            if (device->bindless) {
      struct dxil_spirv_nir_lower_bindless_options options = {
      .dynamic_buffer_binding = layout->dynamic_buffer_count ? layout->set_count : ~0,
   .num_descriptor_sets = layout->set_count,
   .callback_context = (void *)layout,
      };
   bool ret = dxil_spirv_nir_lower_bindless(shader, &options);
   /* We skipped remapping variable bindings in the hashing loop, but if there's static
   * samplers still declared, we need to remap those now. */
   nir_foreach_variable_with_modes(var, shader, nir_var_uniform) {
      assert(glsl_type_is_sampler(glsl_without_array(var->type)));
      }
      } else {
      return nir_shader_intrinsics_pass(shader, adjust_resource_index_binding,
         }
      enum dxil_shader_model
         {
      static_assert(D3D_SHADER_MODEL_6_0 == 0x60 && SHADER_MODEL_6_0 == 0x60000, "Validating math below");
   static_assert(D3D_SHADER_MODEL_6_7 == 0x67 && SHADER_MODEL_6_7 == 0x60007, "Validating math below");
      }
      static VkResult
   dzn_pipeline_compile_shader(struct dzn_device *device,
                     {
      struct dzn_instance *instance =
         struct dzn_physical_device *pdev =
         struct nir_to_dxil_options opts = {
      .environment = DXIL_ENVIRONMENT_VULKAN,
   .lower_int16 = !pdev->options4.Native16BitShaderOpsSupported &&
   /* Don't lower 16-bit types if they can only come from min-precision */
      (device->vk.enabled_extensions.KHR_shader_float16_int8 ||
   device->vk.enabled_features.shaderFloat16 ||
      .shader_model_max = dzn_get_shader_model(pdev),
   #ifdef _WIN32
         #endif
      };
   struct blob dxil_blob;
            if (instance->debug_flags & DZN_DEBUG_NIR)
            if (nir_to_dxil(nir, &opts, NULL, &dxil_blob)) {
      blob_finish_get_buffer(&dxil_blob, (void **)&slot->pShaderBytecode,
      } else {
                  if (dxil_blob.allocated)
            if (result != VK_SUCCESS)
         #ifdef _WIN32
      char *err;
   bool res = dxil_validate_module(instance->dxil_validator,
                  if (instance->debug_flags & DZN_DEBUG_DXIL) {
      char *disasm = dxil_disasm_module(instance->dxil_validator,
               if (disasm) {
      fprintf(stderr,
         "== BEGIN SHADER ============================================\n"
   "%s\n"
   "== END SHADER ==============================================\n",
                  if (!res) {
      if (err) {
      mesa_loge(
         "== VALIDATION ERROR =============================================\n"
   "%s\n"
   "== END ==========================================================\n",
      }
         #endif
            }
      static D3D12_SHADER_BYTECODE *
   dzn_pipeline_get_gfx_shader_slot(D3D12_PIPELINE_STATE_STREAM_DESC *stream,
         {
      switch (in) {
   case MESA_SHADER_VERTEX: {
      d3d12_gfx_pipeline_state_stream_new_desc(stream, VS, D3D12_SHADER_BYTECODE, desc);
      }
   case MESA_SHADER_TESS_CTRL: {
      d3d12_gfx_pipeline_state_stream_new_desc(stream, DS, D3D12_SHADER_BYTECODE, desc);
      }
   case MESA_SHADER_TESS_EVAL: {
      d3d12_gfx_pipeline_state_stream_new_desc(stream, HS, D3D12_SHADER_BYTECODE, desc);
      }
   case MESA_SHADER_GEOMETRY: {
      d3d12_gfx_pipeline_state_stream_new_desc(stream, GS, D3D12_SHADER_BYTECODE, desc);
      }
   case MESA_SHADER_FRAGMENT: {
      d3d12_gfx_pipeline_state_stream_new_desc(stream, PS, D3D12_SHADER_BYTECODE, desc);
      }
   default: unreachable("Unsupported stage");
      }
      struct dzn_cached_dxil_shader_header {
      gl_shader_stage stage;
   size_t size;
      };
      static VkResult
   dzn_pipeline_cache_lookup_dxil_shader(struct vk_pipeline_cache *cache,
                     {
               if (!cache)
                     cache_obj =
      vk_pipeline_cache_lookup_object(cache, dxil_hash, SHA1_DIGEST_LENGTH,
            if (!cache_obj)
            struct dzn_cached_blob *cached_blob =
                           const struct dzn_cached_dxil_shader_header *info =
            assert(sizeof(struct dzn_cached_dxil_shader_header) + info->size <= cached_blob->size);
   assert(info->stage > MESA_SHADER_NONE && info->stage < MESA_VULKAN_SHADER_STAGES);
            void *code = malloc(info->size);
   if (!code) {
      ret = vk_error(cache->base.device, VK_ERROR_OUT_OF_HOST_MEMORY);
                        bc->pShaderBytecode = code;
   bc->BytecodeLength = info->size;
         out:
      vk_pipeline_cache_object_unref(cache->base.device, cache_obj);
      }
      static void
   dzn_pipeline_cache_add_dxil_shader(struct vk_pipeline_cache *cache,
                     {
      size_t size = sizeof(struct dzn_cached_dxil_shader_header) +
            struct vk_pipeline_cache_object *cache_obj =
         if (!cache_obj)
            struct dzn_cached_blob *cached_blob =
         struct dzn_cached_dxil_shader_header *info =
         info->stage = stage;
   info->size = bc->BytecodeLength;
            cache_obj = vk_pipeline_cache_add_object(cache, cache_obj);
      }
      struct dzn_cached_gfx_pipeline_header {
      uint32_t stages : 31;
   uint32_t rast_disabled_from_missing_position : 1;
      };
      static VkResult
   dzn_pipeline_cache_lookup_gfx_pipeline(struct dzn_graphics_pipeline *pipeline,
                     {
               if (!cache)
                     cache_obj =
      vk_pipeline_cache_lookup_object(cache, pipeline_hash, SHA1_DIGEST_LENGTH,
            if (!cache_obj)
            struct dzn_cached_blob *cached_blob =
         D3D12_PIPELINE_STATE_STREAM_DESC *stream_desc =
            const struct dzn_cached_gfx_pipeline_header *info =
                           if (info->input_count > 0) {
      const D3D12_INPUT_ELEMENT_DESC *inputs =
                     memcpy(pipeline->templates.inputs, inputs,
         d3d12_gfx_pipeline_state_stream_new_desc(stream_desc, INPUT_LAYOUT, D3D12_INPUT_LAYOUT_DESC, desc);
   desc->pInputElementDescs = pipeline->templates.inputs;
   desc->NumElements = info->input_count;
                        u_foreach_bit(s, info->stages) {
      uint8_t *dxil_hash = (uint8_t *)cached_blob->data + offset;
            D3D12_SHADER_BYTECODE *slot =
            VkResult ret =
         if (ret != VK_SUCCESS)
            assert(stage == s);
                                 vk_pipeline_cache_object_unref(cache->base.device, cache_obj);
      }
      static void
   dzn_pipeline_cache_add_gfx_pipeline(struct dzn_graphics_pipeline *pipeline,
                           {
      size_t offset =
      ALIGN_POT(sizeof(struct dzn_cached_gfx_pipeline_header), alignof(D3D12_INPUT_ELEMENT_DESC)) +
               for (uint32_t i = 0; i < MESA_VULKAN_SHADER_STAGES; i++) {
      if (pipeline->templates.shaders[i].bc) {
      stages |= BITFIELD_BIT(i);
                  struct vk_pipeline_cache_object *cache_obj =
         if (!cache_obj)
            struct dzn_cached_blob *cached_blob =
            offset = 0;
   struct dzn_cached_gfx_pipeline_header *info =
            info->input_count = vertex_input_count;
   info->stages = stages;
                     D3D12_INPUT_ELEMENT_DESC *inputs =
         memcpy(inputs, pipeline->templates.inputs,
                  u_foreach_bit(s, stages) {
               memcpy(dxil_hash, dxil_hashes[s], SHA1_DIGEST_LENGTH);
               cache_obj = vk_pipeline_cache_add_object(cache, cache_obj);
      }
      static void
   dzn_graphics_pipeline_hash_attribs(D3D12_INPUT_ELEMENT_DESC *attribs,
               {
               _mesa_sha1_init(&ctx);
   _mesa_sha1_update(&ctx, attribs, sizeof(*attribs) * MAX_VERTEX_GENERIC_ATTRIBS);
   _mesa_sha1_update(&ctx, vi_conversions, sizeof(*vi_conversions) * MAX_VERTEX_GENERIC_ATTRIBS);
      }
      static VkResult
   dzn_graphics_pipeline_compile_shaders(struct dzn_device *device,
                                       struct dzn_graphics_pipeline *pipeline,
   {
      struct dzn_physical_device *pdev =
         const VkPipelineViewportStateCreateInfo *vp_info =
      info->pRasterizationState->rasterizerDiscardEnable ?
      struct {
      const VkPipelineShaderStageCreateInfo *info;
   uint8_t spirv_hash[SHA1_DIGEST_LENGTH];
   uint8_t dxil_hash[SHA1_DIGEST_LENGTH];
   uint8_t nir_hash[SHA1_DIGEST_LENGTH];
      } stages[MESA_VULKAN_SHADER_STAGES] = { 0 };
   const uint8_t *dxil_hashes[MESA_VULKAN_SHADER_STAGES] = { 0 };
   uint8_t attribs_hash[SHA1_DIGEST_LENGTH];
   uint8_t pipeline_hash[SHA1_DIGEST_LENGTH];
   gl_shader_stage last_raster_stage = MESA_SHADER_NONE;
   uint32_t active_stage_mask = 0;
            /* First step: collect stage info in a table indexed by gl_shader_stage
   * so we can iterate over stages in pipeline order or reverse pipeline
   * order.
   */
   for (uint32_t i = 0; i < info->stageCount; i++) {
      gl_shader_stage stage =
                     if ((stage == MESA_SHADER_VERTEX ||
      stage == MESA_SHADER_TESS_EVAL ||
   stage == MESA_SHADER_GEOMETRY) &&
               if (stage == MESA_SHADER_FRAGMENT &&
      info->pRasterizationState &&
   (info->pRasterizationState->rasterizerDiscardEnable ||
   info->pRasterizationState->cullMode == VK_CULL_MODE_FRONT_AND_BACK)) {
   /* Disable rasterization (AKA leave fragment shader NULL) when
   * front+back culling or discard is set.
   */
               stages[stage].info = &info->pStages[i];
               pipeline->use_gs_for_polygon_mode_point =
      info->pRasterizationState &&
   info->pRasterizationState->polygonMode == VK_POLYGON_MODE_POINT &&
      if (pipeline->use_gs_for_polygon_mode_point)
            enum dxil_spirv_yz_flip_mode yz_flip_mode = DXIL_SPIRV_YZ_FLIP_NONE;
   uint16_t y_flip_mask = 0, z_flip_mask = 0;
   bool lower_view_index =
      !pipeline->multiview.native_view_instancing &&
         if (pipeline->vp.dynamic) {
         } else if (vp_info) {
      for (uint32_t i = 0; vp_info->pViewports && i < vp_info->viewportCount; i++) {
                     if (vp_info->pViewports[i].minDepth > vp_info->pViewports[i].maxDepth)
               if (y_flip_mask && z_flip_mask)
         else if (z_flip_mask)
         else if (y_flip_mask)
               bool force_sample_rate_shading =
      !info->pRasterizationState->rasterizerDiscardEnable &&
   info->pMultisampleState &&
         if (cache) {
                        _mesa_sha1_init(&pipeline_hash_ctx);
   _mesa_sha1_update(&pipeline_hash_ctx, &device->bindless, sizeof(device->bindless));
   _mesa_sha1_update(&pipeline_hash_ctx, attribs_hash, sizeof(attribs_hash));
   _mesa_sha1_update(&pipeline_hash_ctx, &yz_flip_mode, sizeof(yz_flip_mode));
   _mesa_sha1_update(&pipeline_hash_ctx, &y_flip_mask, sizeof(y_flip_mask));
   _mesa_sha1_update(&pipeline_hash_ctx, &z_flip_mask, sizeof(z_flip_mask));
   _mesa_sha1_update(&pipeline_hash_ctx, &force_sample_rate_shading, sizeof(force_sample_rate_shading));
   _mesa_sha1_update(&pipeline_hash_ctx, &lower_view_index, sizeof(lower_view_index));
            u_foreach_bit(stage, active_stage_mask) {
      const VkPipelineShaderStageRequiredSubgroupSizeCreateInfo *subgroup_size =
      (const VkPipelineShaderStageRequiredSubgroupSizeCreateInfo *)
                     vk_pipeline_hash_shader_stage(stages[stage].info, NULL, stages[stage].spirv_hash);
   _mesa_sha1_update(&pipeline_hash_ctx, &subgroup_enum, sizeof(subgroup_enum));
   _mesa_sha1_update(&pipeline_hash_ctx, stages[stage].spirv_hash, sizeof(stages[stage].spirv_hash));
      }
            bool cache_hit;
   ret = dzn_pipeline_cache_lookup_gfx_pipeline(pipeline, cache, pipeline_hash,
         if (ret != VK_SUCCESS)
            if (cache_hit)
               /* Second step: get NIR shaders for all stages. */
   nir_shader_compiler_options nir_opts;
   unsigned supported_bit_sizes = (pdev->options4.Native16BitShaderOpsSupported ? 16 : 0) | 32 | 64;
   dxil_get_nir_compiler_options(&nir_opts, dzn_get_shader_model(pdev), supported_bit_sizes, supported_bit_sizes);
   nir_opts.lower_base_vertex = true;
   u_foreach_bit(stage, active_stage_mask) {
               const VkPipelineShaderStageRequiredSubgroupSizeCreateInfo *subgroup_size =
      (const VkPipelineShaderStageRequiredSubgroupSizeCreateInfo *)
      enum gl_subgroup_size subgroup_enum = subgroup_size && subgroup_size->requiredSubgroupSize >= 8 ?
            if (cache) {
      _mesa_sha1_init(&nir_hash_ctx);
   _mesa_sha1_update(&nir_hash_ctx, &device->bindless, sizeof(device->bindless));
   if (stage != MESA_SHADER_FRAGMENT) {
      _mesa_sha1_update(&nir_hash_ctx, &lower_view_index, sizeof(lower_view_index));
      }
   if (stage == MESA_SHADER_VERTEX)
         if (stage == last_raster_stage) {
      _mesa_sha1_update(&nir_hash_ctx, &yz_flip_mode, sizeof(yz_flip_mode));
   _mesa_sha1_update(&nir_hash_ctx, &y_flip_mask, sizeof(y_flip_mask));
   _mesa_sha1_update(&nir_hash_ctx, &z_flip_mask, sizeof(z_flip_mask));
      }
   _mesa_sha1_update(&nir_hash_ctx, &subgroup_enum, sizeof(subgroup_enum));
   _mesa_sha1_update(&nir_hash_ctx, stages[stage].spirv_hash, sizeof(stages[stage].spirv_hash));
               struct dzn_nir_options options = {
      .yz_flip_mode = stage == last_raster_stage ? yz_flip_mode : DXIL_SPIRV_YZ_FLIP_NONE,
   .y_flip_mask = y_flip_mask,
   .z_flip_mask = z_flip_mask,
   .force_sample_rate_shading = stage == MESA_SHADER_FRAGMENT ? force_sample_rate_shading : false,
   .lower_view_index = lower_view_index,
   .lower_view_index_to_rt_layer = stage == last_raster_stage ? lower_view_index : false,
   .vi_conversions = vi_conversions,
   .nir_opts = &nir_opts,
               ret = dzn_pipeline_get_nir_shader(device, layout,
                           if (ret != VK_SUCCESS)
               if (pipeline->use_gs_for_polygon_mode_point) {
      /* TODO: Cache; handle TES */
   struct dzn_nir_point_gs_info gs_info = {
      .cull_mode = info->pRasterizationState->cullMode,
   .front_ccw = info->pRasterizationState->frontFace == VK_FRONT_FACE_COUNTER_CLOCKWISE,
   .depth_bias = info->pRasterizationState->depthBiasEnable,
   .depth_bias_dynamic = pipeline->zsa.dynamic_depth_bias,
   .ds_fmt = pipeline->zsa.ds_fmt,
   .constant_depth_bias = info->pRasterizationState->depthBiasConstantFactor,
   .slope_scaled_depth_bias = info->pRasterizationState->depthBiasSlopeFactor,
   .depth_bias_clamp = info->pRasterizationState->depthBiasClamp,
   .runtime_data_cbv = {
      .register_space = DZN_REGISTER_SPACE_SYSVALS,
         };
   pipeline->templates.shaders[MESA_SHADER_GEOMETRY].nir =
                  struct dxil_spirv_runtime_conf conf = {
      .runtime_data_cbv = {
      .register_space = DZN_REGISTER_SPACE_SYSVALS,
      },
   .yz_flip = {
      .mode = yz_flip_mode,
   .y_mask = y_flip_mask,
                  bool requires_runtime_data;
   NIR_PASS_V(pipeline->templates.shaders[MESA_SHADER_GEOMETRY].nir, dxil_spirv_nir_lower_yz_flip,
            active_stage_mask |= (1 << MESA_SHADER_GEOMETRY);
            if ((active_stage_mask & (1 << MESA_SHADER_FRAGMENT)) &&
      BITSET_TEST(pipeline->templates.shaders[MESA_SHADER_FRAGMENT].nir->info.system_values_read, SYSTEM_VALUE_FRONT_FACE))
            /* Third step: link those NIR shaders. We iterate in reverse order
   * so we can eliminate outputs that are never read by the next stage.
   */
   uint32_t link_mask = active_stage_mask;
   while (link_mask != 0) {
      gl_shader_stage stage = util_last_bit(link_mask) - 1;
   link_mask &= ~BITFIELD_BIT(stage);
            struct dxil_spirv_runtime_conf conf = {
      .runtime_data_cbv = {
      .register_space = DZN_REGISTER_SPACE_SYSVALS,
            assert(pipeline->templates.shaders[stage].nir);
   bool requires_runtime_data;
   dxil_spirv_nir_link(pipeline->templates.shaders[stage].nir,
                        if (prev_stage != MESA_SHADER_NONE) {
      memcpy(stages[stage].link_hashes[0], stages[prev_stage].spirv_hash, SHA1_DIGEST_LENGTH);
                  u_foreach_bit(stage, active_stage_mask) {
               NIR_PASS_V(pipeline->templates.shaders[stage].nir, adjust_var_bindings, device, layout,
            if (cache) {
               _mesa_sha1_init(&dxil_hash_ctx);
   _mesa_sha1_update(&dxil_hash_ctx, stages[stage].nir_hash, sizeof(stages[stage].nir_hash));
   _mesa_sha1_update(&dxil_hash_ctx, stages[stage].spirv_hash, sizeof(stages[stage].spirv_hash));
   _mesa_sha1_update(&dxil_hash_ctx, stages[stage].link_hashes[0], sizeof(stages[stage].link_hashes[0]));
   _mesa_sha1_update(&dxil_hash_ctx, stages[stage].link_hashes[1], sizeof(stages[stage].link_hashes[1]));
   _mesa_sha1_update(&dxil_hash_ctx, bindings_hash, sizeof(bindings_hash));
                  gl_shader_stage cached_stage;
   D3D12_SHADER_BYTECODE bc;
   ret = dzn_pipeline_cache_lookup_dxil_shader(cache, stages[stage].dxil_hash, &cached_stage, &bc);
                  if (cached_stage != MESA_SHADER_NONE) {
      assert(cached_stage == stage);
   D3D12_SHADER_BYTECODE *slot =
         *slot = bc;
                     uint32_t vert_input_count = 0;
   if (pipeline->templates.shaders[MESA_SHADER_VERTEX].nir) {
      /* Now, declare one D3D12_INPUT_ELEMENT_DESC per VS input variable, so
   * we can handle location overlaps properly.
   */
   nir_foreach_shader_in_variable(var, pipeline->templates.shaders[MESA_SHADER_VERTEX].nir) {
      assert(var->data.location >= VERT_ATTRIB_GENERIC0);
   unsigned loc = var->data.location - VERT_ATTRIB_GENERIC0;
                  pipeline->templates.inputs[vert_input_count] = attribs[loc];
   pipeline->templates.inputs[vert_input_count].SemanticIndex = vert_input_count;
               if (vert_input_count > 0) {
      d3d12_gfx_pipeline_state_stream_new_desc(out, INPUT_LAYOUT, D3D12_INPUT_LAYOUT_DESC, desc);
   desc->pInputElementDescs = pipeline->templates.inputs;
                  /* Last step: translate NIR shaders into DXIL modules */
   u_foreach_bit(stage, active_stage_mask) {
      gl_shader_stage prev_stage =
         uint32_t prev_stage_output_clip_size = 0;
   if (stage == MESA_SHADER_FRAGMENT) {
      /* Disable rasterization if the last geometry stage doesn't
   * write the position.
   */
   if (prev_stage == MESA_SHADER_NONE ||
      !(pipeline->templates.shaders[prev_stage].nir->info.outputs_written & VARYING_BIT_POS)) {
   pipeline->rast_disabled_from_missing_position = true;
   /* Clear a cache hit if there was one. */
   pipeline->templates.shaders[stage].bc = NULL;
         } else if (prev_stage != MESA_SHADER_NONE) {
                  /* Cache hit, we can skip the compilation. */
   if (pipeline->templates.shaders[stage].bc)
            D3D12_SHADER_BYTECODE *slot =
            ret = dzn_pipeline_compile_shader(device, pipeline->templates.shaders[stage].nir, prev_stage_output_clip_size, slot);
   if (ret != VK_SUCCESS)
                     if (cache)
               if (cache)
      dzn_pipeline_cache_add_gfx_pipeline(pipeline, cache, vert_input_count, pipeline_hash,
            }
      VkFormat
   dzn_graphics_pipeline_patch_vi_format(VkFormat format)
   {
      switch (format) {
   case VK_FORMAT_A2R10G10B10_SNORM_PACK32:
   case VK_FORMAT_A2R10G10B10_UNORM_PACK32:
   case VK_FORMAT_A2R10G10B10_SSCALED_PACK32:
   case VK_FORMAT_A2R10G10B10_USCALED_PACK32:
   case VK_FORMAT_A2B10G10R10_SNORM_PACK32:
   case VK_FORMAT_A2B10G10R10_SSCALED_PACK32:
   case VK_FORMAT_A2B10G10R10_USCALED_PACK32:
         case VK_FORMAT_R8G8B8A8_SSCALED:
         case VK_FORMAT_R8G8B8A8_USCALED:
         case VK_FORMAT_R16G16B16A16_USCALED:
         case VK_FORMAT_R16G16B16A16_SSCALED:
         default:
            }
      static VkResult
   dzn_graphics_pipeline_translate_vi(struct dzn_graphics_pipeline *pipeline,
                     {
      const VkPipelineVertexInputStateCreateInfo *in_vi =
         const VkPipelineVertexInputDivisorStateCreateInfoEXT *divisors =
      (const VkPipelineVertexInputDivisorStateCreateInfoEXT *)
         if (!in_vi->vertexAttributeDescriptionCount)
                     pipeline->vb.count = 0;
   for (uint32_t i = 0; i < in_vi->vertexBindingDescriptionCount; i++) {
      const struct VkVertexInputBindingDescription *bdesc =
            pipeline->vb.count = MAX2(pipeline->vb.count, bdesc->binding + 1);
   pipeline->vb.strides[bdesc->binding] = bdesc->stride;
   if (bdesc->inputRate == VK_VERTEX_INPUT_RATE_INSTANCE) {
         } else {
      assert(bdesc->inputRate == VK_VERTEX_INPUT_RATE_VERTEX);
                  for (uint32_t i = 0; i < in_vi->vertexAttributeDescriptionCount; i++) {
      const VkVertexInputAttributeDescription *attr =
                  if (slot_class[attr->binding] == D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA &&
      divisors) {
   for (uint32_t d = 0; d < divisors->vertexBindingDivisorCount; d++) {
      if (attr->binding == divisors->pVertexBindingDivisors[d].binding) {
      divisor = &divisors->pVertexBindingDivisors[d];
                     VkFormat patched_format = dzn_graphics_pipeline_patch_vi_format(attr->format);
   if (patched_format != attr->format)
            /* nir_to_dxil() name all vertex inputs as TEXCOORDx */
   inputs[attr->location] = (D3D12_INPUT_ELEMENT_DESC) {
      .SemanticName = "TEXCOORD",
   .Format = dzn_buffer_get_dxgi_format(patched_format),
   .InputSlot = attr->binding,
   .InputSlotClass = slot_class[attr->binding],
   .InstanceDataStepRate =
      divisor ? divisor->divisor :
                        }
      static D3D12_PRIMITIVE_TOPOLOGY_TYPE
   to_prim_topology_type(VkPrimitiveTopology in)
   {
      switch (in) {
   case VK_PRIMITIVE_TOPOLOGY_POINT_LIST:
         case VK_PRIMITIVE_TOPOLOGY_LINE_LIST:
   case VK_PRIMITIVE_TOPOLOGY_LINE_STRIP:
   case VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY:
   case VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY:
         case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST:
   case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP:
   case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN:
   case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY:
   case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY:
         case VK_PRIMITIVE_TOPOLOGY_PATCH_LIST:
         default: unreachable("Invalid primitive topology");
      }
      static D3D12_PRIMITIVE_TOPOLOGY
   to_prim_topology(VkPrimitiveTopology in, unsigned patch_control_points, bool support_triangle_fan)
   {
      switch (in) {
   case VK_PRIMITIVE_TOPOLOGY_POINT_LIST: return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
   case VK_PRIMITIVE_TOPOLOGY_LINE_LIST: return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
   case VK_PRIMITIVE_TOPOLOGY_LINE_STRIP: return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
   case VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY: return D3D_PRIMITIVE_TOPOLOGY_LINELIST_ADJ;
   case VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY: return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP_ADJ;
   case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
   case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
   /* Triangle fans are emulated using an intermediate index buffer. */
   case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN: return support_triangle_fan ?
         case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ;
   case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP_ADJ;
   case VK_PRIMITIVE_TOPOLOGY_PATCH_LIST:
      assert(patch_control_points);
      default: unreachable("Invalid primitive topology");
      }
      static VkResult
   dzn_graphics_pipeline_translate_ia(struct dzn_device *device,
                     {
      struct dzn_physical_device *pdev =
         const VkPipelineInputAssemblyStateCreateInfo *in_ia =
         bool has_tes = false;
   for (uint32_t i = 0; i < in->stageCount; i++) {
      if (in->pStages[i].stage == VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT ||
      in->pStages[i].stage == VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT) {
   has_tes = true;
         }
   const VkPipelineTessellationStateCreateInfo *in_tes =
                  d3d12_gfx_pipeline_state_stream_new_desc(out, PRIMITIVE_TOPOLOGY, D3D12_PRIMITIVE_TOPOLOGY_TYPE, prim_top_type);
   *prim_top_type = to_prim_topology_type(in_ia->topology);
   pipeline->ia.triangle_fan = in_ia->topology == VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN && !pdev->options15.TriangleFanSupported;
   pipeline->ia.topology =
      to_prim_topology(in_ia->topology, in_tes ? in_tes->patchControlPoints : 0,
         if (in_ia->primitiveRestartEnable) {
      d3d12_gfx_pipeline_state_stream_new_desc(out, IB_STRIP_CUT_VALUE, D3D12_INDEX_BUFFER_STRIP_CUT_VALUE, ib_strip_cut);
   pipeline->templates.desc_offsets.ib_strip_cut =
         *ib_strip_cut = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
                  }
      static D3D12_FILL_MODE
   translate_polygon_mode(VkPolygonMode in)
   {
      switch (in) {
   case VK_POLYGON_MODE_FILL: return D3D12_FILL_MODE_SOLID;
   case VK_POLYGON_MODE_LINE: return D3D12_FILL_MODE_WIREFRAME;
   case VK_POLYGON_MODE_POINT:
      /* This is handled elsewhere */
      default: unreachable("Unsupported polygon mode");
      }
      static D3D12_CULL_MODE
   translate_cull_mode(VkCullModeFlags in)
   {
      switch (in) {
   case VK_CULL_MODE_NONE: return D3D12_CULL_MODE_NONE;
   case VK_CULL_MODE_FRONT_BIT: return D3D12_CULL_MODE_FRONT;
   case VK_CULL_MODE_BACK_BIT: return D3D12_CULL_MODE_BACK;
   /* Front+back face culling is equivalent to 'rasterization disabled' */
   case VK_CULL_MODE_FRONT_AND_BACK: return D3D12_CULL_MODE_NONE;
   default: unreachable("Unsupported cull mode");
      }
      static int32_t
   translate_depth_bias(double depth_bias)
   {
      if (depth_bias > INT32_MAX)
         else if (depth_bias < INT32_MIN)
               }
      static void
   dzn_graphics_pipeline_translate_rast(struct dzn_device *device,
                     {
      struct dzn_physical_device *pdev = container_of(device->vk.physical, struct dzn_physical_device, vk);
   const VkPipelineRasterizationStateCreateInfo *in_rast =
         const VkPipelineViewportStateCreateInfo *in_vp =
         const VkPipelineMultisampleStateCreateInfo *in_ms =
            if (in_vp) {
      pipeline->vp.count = in_vp->viewportCount;
   if (in_vp->pViewports) {
      for (uint32_t i = 0; in_vp->pViewports && i < in_vp->viewportCount; i++)
               pipeline->scissor.count = in_vp->scissorCount;
   if (in_vp->pScissors) {
      for (uint32_t i = 0; i < in_vp->scissorCount; i++)
                  if (pdev->options19.NarrowQuadrilateralLinesSupported) {
      assert(pdev->options16.DynamicDepthBiasSupported);
   d3d12_gfx_pipeline_state_stream_new_desc(out, RASTERIZER2, D3D12_RASTERIZER_DESC2, desc);
   pipeline->templates.desc_offsets.rast =
         desc->DepthClipEnable = !in_rast->depthClampEnable;
   desc->FillMode = translate_polygon_mode(in_rast->polygonMode);
   desc->CullMode = translate_cull_mode(in_rast->cullMode);
   desc->FrontCounterClockwise =
         if (in_rast->depthBiasEnable) {
      desc->DepthBias = in_rast->depthBiasConstantFactor;
   desc->SlopeScaledDepthBias = in_rast->depthBiasSlopeFactor;
      }
      } else {
      static_assert(sizeof(D3D12_RASTERIZER_DESC) == sizeof(D3D12_RASTERIZER_DESC1), "Casting between these");
   D3D12_PIPELINE_STATE_SUBOBJECT_TYPE rast_type = pdev->options16.DynamicDepthBiasSupported ?
      D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER1 :
      d3d12_pipeline_state_stream_new_desc(out, MAX_GFX_PIPELINE_STATE_STREAM_SIZE, rast_type, D3D12_RASTERIZER_DESC, desc);
   pipeline->templates.desc_offsets.rast =
         desc->DepthClipEnable = !in_rast->depthClampEnable;
   desc->FillMode = translate_polygon_mode(in_rast->polygonMode);
   desc->CullMode = translate_cull_mode(in_rast->cullMode);
   desc->FrontCounterClockwise =
         if (in_rast->depthBiasEnable) {
      if (rast_type == D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER1)
         else
         desc->SlopeScaledDepthBias = in_rast->depthBiasSlopeFactor;
               /* The Vulkan conformance tests use different reference rasterizers for single-sampled
   * and multi-sampled lines. The single-sampled lines can be bresenham lines, but multi-
   * sampled need to be quadrilateral lines. This still isn't *quite* sufficient, because
   * D3D only supports a line width of 1.4 (per spec), but Vulkan requires us to support
   * 1.0 (and without claiming wide lines, that's all we can support).
   */
   if (in_ms && in_ms->rasterizationSamples > 1)
                  }
      static void
   dzn_graphics_pipeline_translate_ms(struct dzn_graphics_pipeline *pipeline,
               {
      const VkPipelineRasterizationStateCreateInfo *in_rast =
         const VkPipelineMultisampleStateCreateInfo *in_ms =
            if (!in_ms)
            /* TODO: minSampleShading (use VRS), alphaToOneEnable */
   d3d12_gfx_pipeline_state_stream_new_desc(out, SAMPLE_DESC, DXGI_SAMPLE_DESC, desc);
   desc->Count = in_ms ? in_ms->rasterizationSamples : 1;
            if (!in_ms->pSampleMask)
            d3d12_gfx_pipeline_state_stream_new_desc(out, SAMPLE_MASK, UINT, mask);
      }
      static D3D12_STENCIL_OP
   translate_stencil_op(VkStencilOp in)
   {
      switch (in) {
   case VK_STENCIL_OP_KEEP: return D3D12_STENCIL_OP_KEEP;
   case VK_STENCIL_OP_ZERO: return D3D12_STENCIL_OP_ZERO;
   case VK_STENCIL_OP_REPLACE: return D3D12_STENCIL_OP_REPLACE;
   case VK_STENCIL_OP_INCREMENT_AND_CLAMP: return D3D12_STENCIL_OP_INCR_SAT;
   case VK_STENCIL_OP_DECREMENT_AND_CLAMP: return D3D12_STENCIL_OP_DECR_SAT;
   case VK_STENCIL_OP_INCREMENT_AND_WRAP: return D3D12_STENCIL_OP_INCR;
   case VK_STENCIL_OP_DECREMENT_AND_WRAP: return D3D12_STENCIL_OP_DECR;
   case VK_STENCIL_OP_INVERT: return D3D12_STENCIL_OP_INVERT;
   default: unreachable("Invalid stencil op");
      }
      static void
   translate_stencil_test(struct dzn_graphics_pipeline *pipeline,
               {
      const VkPipelineDepthStencilStateCreateInfo *in_zsa =
            bool front_test_uses_ref =
      !(in->pRasterizationState->cullMode & VK_CULL_MODE_FRONT_BIT) &&
   in_zsa->front.compareOp != VK_COMPARE_OP_NEVER &&
   in_zsa->front.compareOp != VK_COMPARE_OP_ALWAYS &&
   (pipeline->zsa.stencil_test.dynamic_compare_mask ||
      bool back_test_uses_ref =
      !(in->pRasterizationState->cullMode & VK_CULL_MODE_BACK_BIT) &&
   in_zsa->back.compareOp != VK_COMPARE_OP_NEVER &&
   in_zsa->back.compareOp != VK_COMPARE_OP_ALWAYS &&
   (pipeline->zsa.stencil_test.dynamic_compare_mask ||
         if (front_test_uses_ref && pipeline->zsa.stencil_test.dynamic_compare_mask)
         else if (front_test_uses_ref)
         else
            if (back_test_uses_ref && pipeline->zsa.stencil_test.dynamic_compare_mask)
         else if (back_test_uses_ref)
         else
            bool back_wr_uses_ref =
      !(in->pRasterizationState->cullMode & VK_CULL_MODE_BACK_BIT) &&
   ((in_zsa->back.compareOp != VK_COMPARE_OP_ALWAYS &&
   in_zsa->back.failOp == VK_STENCIL_OP_REPLACE) ||
   (in_zsa->back.compareOp != VK_COMPARE_OP_NEVER &&
   (!in_zsa->depthTestEnable || in_zsa->depthCompareOp != VK_COMPARE_OP_NEVER) &&
   in_zsa->back.passOp == VK_STENCIL_OP_REPLACE) ||
   (in_zsa->depthTestEnable &&
   in_zsa->depthCompareOp != VK_COMPARE_OP_ALWAYS &&
      bool front_wr_uses_ref =
      !(in->pRasterizationState->cullMode & VK_CULL_MODE_FRONT_BIT) &&
   ((in_zsa->front.compareOp != VK_COMPARE_OP_ALWAYS &&
   in_zsa->front.failOp == VK_STENCIL_OP_REPLACE) ||
   (in_zsa->front.compareOp != VK_COMPARE_OP_NEVER &&
   (!in_zsa->depthTestEnable || in_zsa->depthCompareOp != VK_COMPARE_OP_NEVER) &&
   in_zsa->front.passOp == VK_STENCIL_OP_REPLACE) ||
   (in_zsa->depthTestEnable &&
   in_zsa->depthCompareOp != VK_COMPARE_OP_ALWAYS &&
         pipeline->zsa.stencil_test.front.write_mask =
      (pipeline->zsa.stencil_test.dynamic_write_mask ||
   (in->pRasterizationState->cullMode & VK_CULL_MODE_FRONT_BIT)) ?
      pipeline->zsa.stencil_test.back.write_mask =
      (pipeline->zsa.stencil_test.dynamic_write_mask ||
   (in->pRasterizationState->cullMode & VK_CULL_MODE_BACK_BIT)) ?
         pipeline->zsa.stencil_test.front.uses_ref = front_test_uses_ref || front_wr_uses_ref;
            pipeline->zsa.stencil_test.front.ref =
         pipeline->zsa.stencil_test.back.ref =
            out->FrontFace.StencilReadMask = pipeline->zsa.stencil_test.front.compare_mask;
   out->BackFace.StencilReadMask = pipeline->zsa.stencil_test.back.compare_mask;
   out->FrontFace.StencilWriteMask = pipeline->zsa.stencil_test.front.write_mask;
      }
      static void
   dzn_graphics_pipeline_translate_zsa(struct dzn_device *device,
                     {
      struct dzn_physical_device *pdev =
            const VkPipelineRasterizationStateCreateInfo *in_rast =
         const VkPipelineDepthStencilStateCreateInfo *in_zsa =
            if (!in_zsa ||
      in_rast->cullMode == VK_CULL_MODE_FRONT_AND_BACK) {
   /* Ensure depth is disabled if the rasterizer should be disabled / everything culled */
   if (pdev->options14.IndependentFrontAndBackStencilRefMaskSupported) {
      d3d12_gfx_pipeline_state_stream_new_desc(out, DEPTH_STENCIL2, D3D12_DEPTH_STENCIL_DESC2, stream_desc);
   pipeline->templates.desc_offsets.ds = (uintptr_t)stream_desc - (uintptr_t)out->pPipelineStateSubobjectStream;
      } else {
      d3d12_gfx_pipeline_state_stream_new_desc(out, DEPTH_STENCIL1, D3D12_DEPTH_STENCIL_DESC1, stream_desc);
   pipeline->templates.desc_offsets.ds = (uintptr_t)stream_desc - (uintptr_t)out->pPipelineStateSubobjectStream;
      }
               D3D12_DEPTH_STENCIL_DESC2 desc;
            desc.DepthEnable =
         desc.DepthWriteMask =
      in_zsa->depthWriteEnable ?
      desc.DepthFunc =
      in_zsa->depthTestEnable ?
   dzn_translate_compare_op(in_zsa->depthCompareOp) :
      pipeline->zsa.depth_bounds.enable = in_zsa->depthBoundsTestEnable;
   pipeline->zsa.depth_bounds.min = in_zsa->minDepthBounds;
   pipeline->zsa.depth_bounds.max = in_zsa->maxDepthBounds;
   desc.DepthBoundsTestEnable = in_zsa->depthBoundsTestEnable;
   desc.StencilEnable = in_zsa->stencilTestEnable;
   if (in_zsa->stencilTestEnable) {
      desc.FrontFace.StencilFailOp = translate_stencil_op(in_zsa->front.failOp);
   desc.FrontFace.StencilDepthFailOp = translate_stencil_op(in_zsa->front.depthFailOp);
   desc.FrontFace.StencilPassOp = translate_stencil_op(in_zsa->front.passOp);
   desc.FrontFace.StencilFunc = dzn_translate_compare_op(in_zsa->front.compareOp);
   desc.BackFace.StencilFailOp = translate_stencil_op(in_zsa->back.failOp);
   desc.BackFace.StencilDepthFailOp = translate_stencil_op(in_zsa->back.depthFailOp);
   desc.BackFace.StencilPassOp = translate_stencil_op(in_zsa->back.passOp);
                                 if (pdev->options14.IndependentFrontAndBackStencilRefMaskSupported) {
      d3d12_gfx_pipeline_state_stream_new_desc(out, DEPTH_STENCIL2, D3D12_DEPTH_STENCIL_DESC2, stream_desc);
   pipeline->templates.desc_offsets.ds =
            } else {
      d3d12_gfx_pipeline_state_stream_new_desc(out, DEPTH_STENCIL1, D3D12_DEPTH_STENCIL_DESC1, stream_desc);
   pipeline->templates.desc_offsets.ds =
            stream_desc->DepthEnable = desc.DepthEnable;
   stream_desc->DepthWriteMask = desc.DepthWriteMask;
   stream_desc->DepthFunc = desc.DepthFunc;
   stream_desc->DepthBoundsTestEnable = desc.DepthBoundsTestEnable;
   stream_desc->StencilEnable = desc.StencilEnable;
   stream_desc->FrontFace.StencilFailOp = desc.FrontFace.StencilFailOp;
   stream_desc->FrontFace.StencilDepthFailOp = desc.FrontFace.StencilDepthFailOp;
   stream_desc->FrontFace.StencilPassOp = desc.FrontFace.StencilPassOp;
   stream_desc->FrontFace.StencilFunc = desc.FrontFace.StencilFunc;
   stream_desc->BackFace.StencilFailOp = desc.BackFace.StencilFailOp;
   stream_desc->BackFace.StencilDepthFailOp = desc.BackFace.StencilDepthFailOp;
   stream_desc->BackFace.StencilPassOp = desc.BackFace.StencilPassOp;
            /* No support for independent front/back, just pick front (if set, else back) */
   stream_desc->StencilReadMask = desc.FrontFace.StencilReadMask ? desc.FrontFace.StencilReadMask : desc.BackFace.StencilReadMask;
         }
      static D3D12_BLEND
   translate_blend_factor(VkBlendFactor in, bool is_alpha, bool support_alpha_blend_factor)
   {
      switch (in) {
   case VK_BLEND_FACTOR_ZERO: return D3D12_BLEND_ZERO;
   case VK_BLEND_FACTOR_ONE: return D3D12_BLEND_ONE;
   case VK_BLEND_FACTOR_SRC_COLOR:
         case VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR:
         case VK_BLEND_FACTOR_DST_COLOR:
         case VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR:
         case VK_BLEND_FACTOR_SRC_ALPHA: return D3D12_BLEND_SRC_ALPHA;
   case VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA: return D3D12_BLEND_INV_SRC_ALPHA;
   case VK_BLEND_FACTOR_DST_ALPHA: return D3D12_BLEND_DEST_ALPHA;
   case VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA: return D3D12_BLEND_INV_DEST_ALPHA;
   case VK_BLEND_FACTOR_CONSTANT_COLOR:
         case VK_BLEND_FACTOR_CONSTANT_ALPHA:
         case VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR:
         case VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA:
         case VK_BLEND_FACTOR_SRC1_COLOR:
         case VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR:
         case VK_BLEND_FACTOR_SRC1_ALPHA: return D3D12_BLEND_SRC1_ALPHA;
   case VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA: return D3D12_BLEND_INV_SRC1_ALPHA;
   case VK_BLEND_FACTOR_SRC_ALPHA_SATURATE: return D3D12_BLEND_SRC_ALPHA_SAT;
   default: unreachable("Invalid blend factor");
      }
      static D3D12_BLEND_OP
   translate_blend_op(VkBlendOp in)
   {
      switch (in) {
   case VK_BLEND_OP_ADD: return D3D12_BLEND_OP_ADD;
   case VK_BLEND_OP_SUBTRACT: return D3D12_BLEND_OP_SUBTRACT;
   case VK_BLEND_OP_REVERSE_SUBTRACT: return D3D12_BLEND_OP_REV_SUBTRACT;
   case VK_BLEND_OP_MIN: return D3D12_BLEND_OP_MIN;
   case VK_BLEND_OP_MAX: return D3D12_BLEND_OP_MAX;
   default: unreachable("Invalid blend op");
      }
      static D3D12_LOGIC_OP
   translate_logic_op(VkLogicOp in)
   {
      switch (in) {
   case VK_LOGIC_OP_CLEAR: return D3D12_LOGIC_OP_CLEAR;
   case VK_LOGIC_OP_AND: return D3D12_LOGIC_OP_AND;
   case VK_LOGIC_OP_AND_REVERSE: return D3D12_LOGIC_OP_AND_REVERSE;
   case VK_LOGIC_OP_COPY: return D3D12_LOGIC_OP_COPY;
   case VK_LOGIC_OP_AND_INVERTED: return D3D12_LOGIC_OP_AND_INVERTED;
   case VK_LOGIC_OP_NO_OP: return D3D12_LOGIC_OP_NOOP;
   case VK_LOGIC_OP_XOR: return D3D12_LOGIC_OP_XOR;
   case VK_LOGIC_OP_OR: return D3D12_LOGIC_OP_OR;
   case VK_LOGIC_OP_NOR: return D3D12_LOGIC_OP_NOR;
   case VK_LOGIC_OP_EQUIVALENT: return D3D12_LOGIC_OP_EQUIV;
   case VK_LOGIC_OP_INVERT: return D3D12_LOGIC_OP_INVERT;
   case VK_LOGIC_OP_OR_REVERSE: return D3D12_LOGIC_OP_OR_REVERSE;
   case VK_LOGIC_OP_COPY_INVERTED: return D3D12_LOGIC_OP_COPY_INVERTED;
   case VK_LOGIC_OP_OR_INVERTED: return D3D12_LOGIC_OP_OR_INVERTED;
   case VK_LOGIC_OP_NAND: return D3D12_LOGIC_OP_NAND;
   case VK_LOGIC_OP_SET: return D3D12_LOGIC_OP_SET;
   default: unreachable("Invalid logic op");
      }
      static void
   dzn_graphics_pipeline_translate_blend(struct dzn_graphics_pipeline *pipeline,
               {
      const VkPipelineRasterizationStateCreateInfo *in_rast =
         const VkPipelineColorBlendStateCreateInfo *in_blend =
         const VkPipelineMultisampleStateCreateInfo *in_ms =
            if (!in_blend || !in_ms)
            struct dzn_device *device =
         struct dzn_physical_device *pdev =
                  d3d12_gfx_pipeline_state_stream_new_desc(out, BLEND, D3D12_BLEND_DESC, desc);
   D3D12_LOGIC_OP logicop =
      in_blend->logicOpEnable ?
      desc->AlphaToCoverageEnable = in_ms->alphaToCoverageEnable;
   memcpy(pipeline->blend.constants, in_blend->blendConstants,
            for (uint32_t i = 0; i < in_blend->attachmentCount; i++) {
      if (i > 0 &&
      memcmp(&in_blend->pAttachments[i - 1], &in_blend->pAttachments[i],
               desc->RenderTarget[i].BlendEnable =
         desc->RenderTarget[i].RenderTargetWriteMask =
            if (in_blend->logicOpEnable) {
      desc->RenderTarget[i].LogicOpEnable = true;
      } else {
      desc->RenderTarget[i].SrcBlend =
         desc->RenderTarget[i].DestBlend =
         desc->RenderTarget[i].BlendOp =
         desc->RenderTarget[i].SrcBlendAlpha =
         desc->RenderTarget[i].DestBlendAlpha =
         desc->RenderTarget[i].BlendOpAlpha =
            }
         static void
   dzn_pipeline_init(struct dzn_pipeline *pipeline,
                     struct dzn_device *device,
   {
      pipeline->type = type;
   pipeline->root.sets_param_count = layout->root.sets_param_count;
   pipeline->root.sysval_cbv_param_idx = layout->root.sysval_cbv_param_idx;
   pipeline->root.push_constant_cbv_param_idx = layout->root.push_constant_cbv_param_idx;
   pipeline->root.dynamic_buffer_bindless_param_idx = layout->root.dynamic_buffer_bindless_param_idx;
   STATIC_ASSERT(sizeof(pipeline->root.type) == sizeof(layout->root.type));
   memcpy(pipeline->root.type, layout->root.type, sizeof(pipeline->root.type));
   pipeline->root.sig = layout->root.sig;
            STATIC_ASSERT(sizeof(layout->desc_count) == sizeof(pipeline->desc_count));
            STATIC_ASSERT(sizeof(layout->sets) == sizeof(pipeline->sets));
   memcpy(pipeline->sets, layout->sets, sizeof(pipeline->sets));
   pipeline->set_count = layout->set_count;
   pipeline->dynamic_buffer_count = layout->dynamic_buffer_count;
            ASSERTED uint32_t max_streamsz =
      type == VK_PIPELINE_BIND_POINT_GRAPHICS ?
   MAX_GFX_PIPELINE_STATE_STREAM_SIZE :
         d3d12_pipeline_state_stream_new_desc_abbrev(stream_desc, max_streamsz, ROOT_SIGNATURE,
            }
      static void
   dzn_pipeline_finish(struct dzn_pipeline *pipeline)
   {
      if (pipeline->state)
         if (pipeline->root.sig)
               }
      static void dzn_graphics_pipeline_delete_variant(struct hash_entry *he)
   {
               if (variant->state)
      }
      static void
   dzn_graphics_pipeline_cleanup_nir_shaders(struct dzn_graphics_pipeline *pipeline)
   {
      for (uint32_t i = 0; i < ARRAY_SIZE(pipeline->templates.shaders); i++) {
      ralloc_free(pipeline->templates.shaders[i].nir);
         }
      static void
   dzn_graphics_pipeline_cleanup_dxil_shaders(struct dzn_graphics_pipeline *pipeline)
   {
      for (uint32_t i = 0; i < ARRAY_SIZE(pipeline->templates.shaders); i++) {
      if (pipeline->templates.shaders[i].bc) {
      free((void *)pipeline->templates.shaders[i].bc->pShaderBytecode);
            }
      static void
   dzn_graphics_pipeline_destroy(struct dzn_graphics_pipeline *pipeline,
         {
      if (!pipeline)
            _mesa_hash_table_destroy(pipeline->variants,
            dzn_graphics_pipeline_cleanup_nir_shaders(pipeline);
            for (uint32_t i = 0; i < ARRAY_SIZE(pipeline->indirect_cmd_sigs); i++) {
      if (pipeline->indirect_cmd_sigs[i])
               dzn_pipeline_finish(&pipeline->base);
      }
      static VkResult
   dzn_graphics_pipeline_create(struct dzn_device *device,
                           {
      struct dzn_physical_device *pdev =
         const VkPipelineRenderingCreateInfo *ri = (const VkPipelineRenderingCreateInfo *)
         VK_FROM_HANDLE(vk_pipeline_cache, pcache, cache);
   VK_FROM_HANDLE(vk_render_pass, pass, pCreateInfo->renderPass);
   VK_FROM_HANDLE(dzn_pipeline_layout, layout, pCreateInfo->layout);
   uint32_t color_count = 0;
   VkFormat color_fmts[MAX_RTS] = { 0 };
   VkFormat zs_fmt = VK_FORMAT_UNDEFINED;
   VkResult ret;
   HRESULT hres = 0;
            struct dzn_graphics_pipeline *pipeline =
      vk_zalloc2(&device->vk.alloc, pAllocator, sizeof(*pipeline), 8,
      if (!pipeline)
            D3D12_PIPELINE_STATE_STREAM_DESC *stream_desc = &pipeline->templates.stream_desc;
            dzn_pipeline_init(&pipeline->base, device,
               D3D12_INPUT_ELEMENT_DESC attribs[MAX_VERTEX_GENERIC_ATTRIBS] = { 0 };
            ret = dzn_graphics_pipeline_translate_vi(pipeline, pCreateInfo,
         if (ret != VK_SUCCESS)
            d3d12_gfx_pipeline_state_stream_new_desc(stream_desc, FLAGS, D3D12_PIPELINE_STATE_FLAGS, flags);
            if (pCreateInfo->pDynamicState) {
      for (uint32_t i = 0; i < pCreateInfo->pDynamicState->dynamicStateCount; i++) {
      switch (pCreateInfo->pDynamicState->pDynamicStates[i]) {
   case VK_DYNAMIC_STATE_VIEWPORT:
      pipeline->vp.dynamic = true;
      case VK_DYNAMIC_STATE_SCISSOR:
      pipeline->scissor.dynamic = true;
      case VK_DYNAMIC_STATE_STENCIL_REFERENCE:
      pipeline->zsa.stencil_test.dynamic_ref = true;
      case VK_DYNAMIC_STATE_STENCIL_COMPARE_MASK:
      pipeline->zsa.stencil_test.dynamic_compare_mask = true;
   ret = dzn_graphics_pipeline_prepare_for_variants(device, pipeline);
   if (ret)
            case VK_DYNAMIC_STATE_STENCIL_WRITE_MASK:
      pipeline->zsa.stencil_test.dynamic_write_mask = true;
   ret = dzn_graphics_pipeline_prepare_for_variants(device, pipeline);
   if (ret)
            case VK_DYNAMIC_STATE_BLEND_CONSTANTS:
      pipeline->blend.dynamic_constants = true;
      case VK_DYNAMIC_STATE_DEPTH_BOUNDS:
      pipeline->zsa.depth_bounds.dynamic = true;
      case VK_DYNAMIC_STATE_DEPTH_BIAS:
      pipeline->zsa.dynamic_depth_bias = true;
   if (pdev->options16.DynamicDepthBiasSupported) {
         } else {
      ret = dzn_graphics_pipeline_prepare_for_variants(device, pipeline);
   if (ret)
      }
      case VK_DYNAMIC_STATE_LINE_WIDTH:
      /* Nothing to do since we just support lineWidth = 1. */
      default: unreachable("Unsupported dynamic state");
                  ret = dzn_graphics_pipeline_translate_ia(device, pipeline, stream_desc, pCreateInfo);
   if (ret)
            dzn_graphics_pipeline_translate_rast(device, pipeline, stream_desc, pCreateInfo);
   dzn_graphics_pipeline_translate_ms(pipeline, stream_desc, pCreateInfo);
   dzn_graphics_pipeline_translate_zsa(device, pipeline, stream_desc, pCreateInfo);
            unsigned view_mask = 0;
   if (pass) {
      const struct vk_subpass *subpass = &pass->subpasses[pCreateInfo->subpass];
   color_count = subpass->color_count;
   for (uint32_t i = 0; i < subpass->color_count; i++) {
                                                   if (subpass->depth_stencil_attachment &&
      subpass->depth_stencil_attachment->attachment != VK_ATTACHMENT_UNUSED) {
                                 } else if (ri) {
      color_count = ri->colorAttachmentCount;
   memcpy(color_fmts, ri->pColorAttachmentFormats,
         if (ri->depthAttachmentFormat != VK_FORMAT_UNDEFINED)
         else if (ri->stencilAttachmentFormat != VK_FORMAT_UNDEFINED)
                        if (color_count > 0) {
      d3d12_gfx_pipeline_state_stream_new_desc(stream_desc, RENDER_TARGET_FORMATS, struct D3D12_RT_FORMAT_ARRAY, rts);
   rts->NumRenderTargets = color_count;
   for (uint32_t i = 0; i < color_count; i++) {
      rts->RTFormats[i] =
      dzn_image_get_dxgi_format(pdev, color_fmts[i],
                     if (zs_fmt != VK_FORMAT_UNDEFINED) {
      d3d12_gfx_pipeline_state_stream_new_desc(stream_desc, DEPTH_STENCIL_FORMAT, DXGI_FORMAT, ds_fmt);
   *ds_fmt =
      dzn_image_get_dxgi_format(pdev, zs_fmt,
                              pipeline->multiview.view_mask = MAX2(view_mask, 1);
   if (view_mask != 0 && /* Is multiview */
      view_mask != 1 && /* Is non-trivially multiview */
   (view_mask & ~((1 << D3D12_MAX_VIEW_INSTANCE_COUNT) - 1)) == 0 && /* Uses only views 0 thru 3 */
   pdev->options3.ViewInstancingTier > D3D12_VIEW_INSTANCING_TIER_NOT_SUPPORTED /* Actually supported */) {
   d3d12_gfx_pipeline_state_stream_new_desc(stream_desc, VIEW_INSTANCING, D3D12_VIEW_INSTANCING_DESC, vi);
   vi->pViewInstanceLocations = vi_locs;
   for (uint32_t i = 0; i < D3D12_MAX_VIEW_INSTANCE_COUNT; ++i) {
      vi_locs[i].RenderTargetArrayIndex = i;
   vi_locs[i].ViewportArrayIndex = 0;
   if (view_mask & (1 << i))
      }
   vi->Flags = D3D12_VIEW_INSTANCING_FLAG_ENABLE_VIEW_INSTANCE_MASKING;
               ret = dzn_graphics_pipeline_compile_shaders(device, pipeline, pcache,
                     if (ret != VK_SUCCESS)
            /* If we have no position output from a pre-rasterizer stage, we need to make sure that
   * depth is disabled, to fully disable the rasterizer. We can only know this after compiling
   * or loading the shaders.
   */
   if (pipeline->rast_disabled_from_missing_position) {
      if (pdev->options14.IndependentFrontAndBackStencilRefMaskSupported) {
      D3D12_DEPTH_STENCIL_DESC2 *ds = dzn_graphics_pipeline_get_desc(pipeline, pipeline->templates.stream_buf, ds);
   if (ds)
      } else {
      D3D12_DEPTH_STENCIL_DESC1 *ds = dzn_graphics_pipeline_get_desc(pipeline, pipeline->templates.stream_buf, ds);
   if (ds)
                  if (!pipeline->variants) {
      hres = ID3D12Device4_CreatePipelineState(device->dev, stream_desc,
               if (FAILED(hres)) {
      ret = vk_error(device, VK_ERROR_OUT_OF_HOST_MEMORY);
                           dzn_graphics_pipeline_cleanup_nir_shaders(pipeline);
         out:
      if (ret != VK_SUCCESS)
         else
               }
      static void
   mask_key_for_stencil_state(struct dzn_physical_device *pdev,
                     {
      if (pdev->options14.IndependentFrontAndBackStencilRefMaskSupported) {
      const D3D12_DEPTH_STENCIL_DESC2 *ds_templ =
         if (ds_templ && ds_templ->StencilEnable) {
      if (ds_templ->FrontFace.StencilFunc != D3D12_COMPARISON_FUNC_NEVER &&
      ds_templ->FrontFace.StencilFunc != D3D12_COMPARISON_FUNC_ALWAYS)
      if (ds_templ->BackFace.StencilFunc != D3D12_COMPARISON_FUNC_NEVER &&
      ds_templ->BackFace.StencilFunc != D3D12_COMPARISON_FUNC_ALWAYS)
      if (pipeline->zsa.stencil_test.dynamic_write_mask) {
      masked_key->stencil_test.front.write_mask = key->stencil_test.front.write_mask;
            } else {
      const D3D12_DEPTH_STENCIL_DESC1 *ds_templ =
         if (ds_templ && ds_templ->StencilEnable) {
      if (ds_templ->FrontFace.StencilFunc != D3D12_COMPARISON_FUNC_NEVER &&
      ds_templ->FrontFace.StencilFunc != D3D12_COMPARISON_FUNC_ALWAYS)
      if (ds_templ->BackFace.StencilFunc != D3D12_COMPARISON_FUNC_NEVER &&
      ds_templ->BackFace.StencilFunc != D3D12_COMPARISON_FUNC_ALWAYS)
      if (pipeline->zsa.stencil_test.dynamic_write_mask) {
      masked_key->stencil_test.front.write_mask = key->stencil_test.front.write_mask;
               }
      static void
   update_stencil_state(struct dzn_physical_device *pdev,
                     {
      if (pdev->options14.IndependentFrontAndBackStencilRefMaskSupported) {
      D3D12_DEPTH_STENCIL_DESC2 *ds =
         if (ds && ds->StencilEnable) {
      if (pipeline->zsa.stencil_test.dynamic_compare_mask) {
      if (ds->FrontFace.StencilFunc != D3D12_COMPARISON_FUNC_NEVER &&
                        if (ds->BackFace.StencilFunc != D3D12_COMPARISON_FUNC_NEVER &&
                           if (pipeline->zsa.stencil_test.dynamic_write_mask) {
      ds->FrontFace.StencilWriteMask = masked_key->stencil_test.front.write_mask;
            } else {
      D3D12_DEPTH_STENCIL_DESC1 *ds =
         if (ds && ds->StencilEnable) {
      if (pipeline->zsa.stencil_test.dynamic_compare_mask) {
      if (ds->FrontFace.StencilFunc != D3D12_COMPARISON_FUNC_NEVER &&
                        if (ds->BackFace.StencilFunc != D3D12_COMPARISON_FUNC_NEVER &&
                        if (ds->FrontFace.StencilFunc != D3D12_COMPARISON_FUNC_NEVER &&
         ds->FrontFace.StencilFunc != D3D12_COMPARISON_FUNC_ALWAYS &&
   ds->BackFace.StencilFunc != D3D12_COMPARISON_FUNC_NEVER &&
               if (pipeline->zsa.stencil_test.dynamic_write_mask) {
      assert(!masked_key->stencil_test.front.write_mask ||
               ds->StencilWriteMask =
      masked_key->stencil_test.front.write_mask |
            }
      ID3D12PipelineState *
   dzn_graphics_pipeline_get_state(struct dzn_graphics_pipeline *pipeline,
         {
      if (!pipeline->variants)
            struct dzn_device *device =
         struct dzn_physical_device *pdev =
                     if (dzn_graphics_pipeline_get_desc_template(pipeline, ib_strip_cut))
            if (!pdev->options16.DynamicDepthBiasSupported &&
      dzn_graphics_pipeline_get_desc_template(pipeline, rast) &&
   pipeline->zsa.dynamic_depth_bias)
                  struct hash_entry *he =
                     if (!he) {
      variant = rzalloc(pipeline->variants, struct dzn_graphics_pipeline_variant);
            uintptr_t stream_buf[MAX_GFX_PIPELINE_STATE_STREAM_SIZE / sizeof(uintptr_t)];
   D3D12_PIPELINE_STATE_STREAM_DESC stream_desc = {
      .SizeInBytes = pipeline->templates.stream_desc.SizeInBytes,
                        D3D12_INDEX_BUFFER_STRIP_CUT_VALUE *ib_strip_cut =
         if (ib_strip_cut)
            D3D12_RASTERIZER_DESC *rast =
         if (!pdev->options16.DynamicDepthBiasSupported && rast && pipeline->zsa.dynamic_depth_bias) {
      rast->DepthBias = translate_depth_bias(masked_key.depth_bias.constant_factor);
   rast->DepthBiasClamp = masked_key.depth_bias.clamp;
                        ASSERTED HRESULT hres = ID3D12Device4_CreatePipelineState(device->dev, &stream_desc,
               assert(!FAILED(hres));
   he = _mesa_hash_table_insert(pipeline->variants, &variant->key, variant);
      } else {
                  if (variant->state)
            if (pipeline->base.state)
            pipeline->base.state = variant->state;
      }
      #define DZN_INDIRECT_CMD_SIG_MAX_ARGS 4
      ID3D12CommandSignature *
   dzn_graphics_pipeline_get_indirect_cmd_sig(struct dzn_graphics_pipeline *pipeline,
         {
               struct dzn_device *device =
                  if (cmdsig)
            bool triangle_fan = type == DZN_INDIRECT_DRAW_TRIANGLE_FAN_CMD_SIG;
            uint32_t cmd_arg_count = 0;
            if (triangle_fan) {
      cmd_args[cmd_arg_count++] = (D3D12_INDIRECT_ARGUMENT_DESC) {
                     cmd_args[cmd_arg_count++] = (D3D12_INDIRECT_ARGUMENT_DESC) {
      .Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT,
   .Constant = {
      .RootParameterIndex = pipeline->base.root.sysval_cbv_param_idx,
   .DestOffsetIn32BitValues = offsetof(struct dxil_spirv_vertex_runtime_data, first_vertex) / 4,
                  cmd_args[cmd_arg_count++] = (D3D12_INDIRECT_ARGUMENT_DESC) {
      .Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT,
   .Constant = {
      .RootParameterIndex = pipeline->base.root.sysval_cbv_param_idx,
   .DestOffsetIn32BitValues = offsetof(struct dxil_spirv_vertex_runtime_data, draw_id) / 4,
                  cmd_args[cmd_arg_count++] = (D3D12_INDIRECT_ARGUMENT_DESC) {
      .Type = indexed ?
                     assert(cmd_arg_count <= ARRAY_SIZE(cmd_args));
            D3D12_COMMAND_SIGNATURE_DESC cmd_sig_desc = {
      .ByteStride =
      triangle_fan ?
   sizeof(struct dzn_indirect_triangle_fan_draw_exec_params) :
      .NumArgumentDescs = cmd_arg_count,
      };
   HRESULT hres =
      ID3D12Device1_CreateCommandSignature(device->dev, &cmd_sig_desc,
                  if (FAILED(hres))
            pipeline->indirect_cmd_sigs[type] = cmdsig;
      }
      VKAPI_ATTR VkResult VKAPI_CALL
   dzn_CreateGraphicsPipelines(VkDevice dev,
                                 {
      VK_FROM_HANDLE(dzn_device, device, dev);
            unsigned i;
   for (i = 0; i < count; i++) {
      result = dzn_graphics_pipeline_create(device,
                           if (result != VK_SUCCESS) {
               /* Bail out on the first error != VK_PIPELINE_COMPILE_REQUIRED_EX as it
   * is not obvious what error should be report upon 2 different failures.
   */
                  if (pCreateInfos[i].flags & VK_PIPELINE_CREATE_EARLY_RETURN_ON_FAILURE_BIT)
                  for (; i < count; i++)
               }
      static void
   dzn_compute_pipeline_destroy(struct dzn_compute_pipeline *pipeline,
         {
      if (!pipeline)
            if (pipeline->indirect_cmd_sig)
            dzn_pipeline_finish(&pipeline->base);
      }
      static VkResult
   dzn_pipeline_cache_lookup_compute_pipeline(struct vk_pipeline_cache *cache,
                           {
               if (!cache)
                     cache_obj =
      vk_pipeline_cache_lookup_object(cache, pipeline_hash, SHA1_DIGEST_LENGTH,
            if (!cache_obj)
            struct dzn_cached_blob *cached_blob =
                     const uint8_t *dxil_hash = cached_blob->data;
            VkResult ret =
            if (ret != VK_SUCCESS || stage == MESA_SHADER_NONE)
                     d3d12_compute_pipeline_state_stream_new_desc(stream_desc, CS, D3D12_SHADER_BYTECODE, slot);
   *slot = *dxil;
         out:
      vk_pipeline_cache_object_unref(cache->base.device, cache_obj);
      }
      static void
   dzn_pipeline_cache_add_compute_pipeline(struct vk_pipeline_cache *cache,
               {
      struct vk_pipeline_cache_object *cache_obj =
         if (!cache_obj)
            struct dzn_cached_blob *cached_blob =
                     cache_obj = vk_pipeline_cache_add_object(cache, cache_obj);
      }
      static VkResult
   dzn_compute_pipeline_compile_shader(struct dzn_device *device,
                                       {
      struct dzn_physical_device *pdev =
         uint8_t spirv_hash[SHA1_DIGEST_LENGTH], pipeline_hash[SHA1_DIGEST_LENGTH], nir_hash[SHA1_DIGEST_LENGTH];
   VkResult ret = VK_SUCCESS;
            const VkPipelineShaderStageRequiredSubgroupSizeCreateInfo *subgroup_size =
      (const VkPipelineShaderStageRequiredSubgroupSizeCreateInfo *)
      enum gl_subgroup_size subgroup_enum = subgroup_size && subgroup_size->requiredSubgroupSize >= 8 ?
            if (cache) {
               _mesa_sha1_init(&pipeline_hash_ctx);
   vk_pipeline_hash_shader_stage(&info->stage, NULL, spirv_hash);
   _mesa_sha1_update(&pipeline_hash_ctx, &device->bindless, sizeof(device->bindless));
   _mesa_sha1_update(&pipeline_hash_ctx, &subgroup_enum, sizeof(subgroup_enum));
   _mesa_sha1_update(&pipeline_hash_ctx, spirv_hash, sizeof(spirv_hash));
   _mesa_sha1_update(&pipeline_hash_ctx, layout->stages[MESA_SHADER_COMPUTE].hash,
                  bool cache_hit = false;
   ret = dzn_pipeline_cache_lookup_compute_pipeline(cache, pipeline_hash,
               if (ret != VK_SUCCESS || cache_hit)
               if (cache) {
      struct mesa_sha1 nir_hash_ctx;
   _mesa_sha1_init(&nir_hash_ctx);
   _mesa_sha1_update(&nir_hash_ctx, &device->bindless, sizeof(device->bindless));
   _mesa_sha1_update(&nir_hash_ctx, &subgroup_enum, sizeof(subgroup_enum));
   _mesa_sha1_update(&nir_hash_ctx, spirv_hash, sizeof(spirv_hash));
      }
   nir_shader_compiler_options nir_opts;
   const unsigned supported_bit_sizes = 16 | 32 | 64;
   dxil_get_nir_compiler_options(&nir_opts, dzn_get_shader_model(pdev), supported_bit_sizes, supported_bit_sizes);
   struct dzn_nir_options options = {
      .nir_opts = &nir_opts,
      };
   ret = dzn_pipeline_get_nir_shader(device, layout, cache, nir_hash,
               if (ret != VK_SUCCESS)
                              if (cache) {
               _mesa_sha1_init(&dxil_hash_ctx);
   _mesa_sha1_update(&dxil_hash_ctx, nir_hash, sizeof(nir_hash));
   _mesa_sha1_update(&dxil_hash_ctx, spirv_hash, sizeof(spirv_hash));
   _mesa_sha1_update(&dxil_hash_ctx, bindings_hash, sizeof(bindings_hash));
                     ret = dzn_pipeline_cache_lookup_dxil_shader(cache, dxil_hash, &stage, shader);
   if (ret != VK_SUCCESS)
            if (stage != MESA_SHADER_NONE) {
      assert(stage == MESA_SHADER_COMPUTE);
   d3d12_compute_pipeline_state_stream_new_desc(stream_desc, CS, D3D12_SHADER_BYTECODE, cs);
   *cs = *shader;
   dzn_pipeline_cache_add_compute_pipeline(cache, pipeline_hash, dxil_hash);
                  ret = dzn_pipeline_compile_shader(device, nir, 0, shader);
   if (ret != VK_SUCCESS)
            d3d12_compute_pipeline_state_stream_new_desc(stream_desc, CS, D3D12_SHADER_BYTECODE, cs);
            if (cache) {
      dzn_pipeline_cache_add_dxil_shader(cache, dxil_hash, MESA_SHADER_COMPUTE, shader);
            out:
      ralloc_free(nir);
      }
      static VkResult
   dzn_compute_pipeline_create(struct dzn_device *device,
                           {
      VK_FROM_HANDLE(dzn_pipeline_layout, layout, pCreateInfo->layout);
            struct dzn_compute_pipeline *pipeline =
      vk_zalloc2(&device->vk.alloc, pAllocator, sizeof(*pipeline), 8,
      if (!pipeline)
            uintptr_t state_buf[MAX_COMPUTE_PIPELINE_STATE_STREAM_SIZE / sizeof(uintptr_t)];
   D3D12_PIPELINE_STATE_STREAM_DESC stream_desc = {
                  dzn_pipeline_init(&pipeline->base, device,
                  D3D12_SHADER_BYTECODE shader = { 0 };
   VkResult ret =
      dzn_compute_pipeline_compile_shader(device, pipeline, pcache, layout,
      if (ret != VK_SUCCESS)
            if (FAILED(ID3D12Device4_CreatePipelineState(device->dev, &stream_desc,
                     out:
      free((void *)shader.pShaderBytecode);
   if (ret != VK_SUCCESS)
         else
               }
      ID3D12CommandSignature *
   dzn_compute_pipeline_get_indirect_cmd_sig(struct dzn_compute_pipeline *pipeline)
   {
      if (pipeline->indirect_cmd_sig)
            struct dzn_device *device =
            D3D12_INDIRECT_ARGUMENT_DESC indirect_dispatch_args[] = {
      {
      .Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT,
   .Constant = {
      .RootParameterIndex = pipeline->base.root.sysval_cbv_param_idx,
   .DestOffsetIn32BitValues = 0,
         },
   {
                     D3D12_COMMAND_SIGNATURE_DESC indirect_dispatch_desc = {
      .ByteStride = sizeof(D3D12_DISPATCH_ARGUMENTS) * 2,
   .NumArgumentDescs = ARRAY_SIZE(indirect_dispatch_args),
               HRESULT hres =
      ID3D12Device1_CreateCommandSignature(device->dev, &indirect_dispatch_desc,
                  if (FAILED(hres))
               }
      VKAPI_ATTR VkResult VKAPI_CALL
   dzn_CreateComputePipelines(VkDevice dev,
                                 {
      VK_FROM_HANDLE(dzn_device, device, dev);
            unsigned i;
   for (i = 0; i < count; i++) {
      result = dzn_compute_pipeline_create(device,
                           if (result != VK_SUCCESS) {
               /* Bail out on the first error != VK_PIPELINE_COMPILE_REQUIRED_EX as it
   * is not obvious what error should be report upon 2 different failures.
   */
                  if (pCreateInfos[i].flags & VK_PIPELINE_CREATE_EARLY_RETURN_ON_FAILURE_BIT)
                  for (; i < count; i++)
               }
      VKAPI_ATTR void VKAPI_CALL
   dzn_DestroyPipeline(VkDevice device,
               {
               if (!pipe)
            if (pipe->type == VK_PIPELINE_BIND_POINT_GRAPHICS) {
      struct dzn_graphics_pipeline *gfx = container_of(pipe, struct dzn_graphics_pipeline, base);
      } else {
      assert(pipe->type == VK_PIPELINE_BIND_POINT_COMPUTE);
   struct dzn_compute_pipeline *compute = container_of(pipe, struct dzn_compute_pipeline, base);
         }
