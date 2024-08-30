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
      #include "spirv_to_dxil.h"
   #include "nir_to_dxil.h"
      #include "dxil_nir.h"
   #include "dxil_nir_lower_int_samplers.h"
   #include "dxil_validator.h"
      static void
   dzn_meta_compile_shader(struct dzn_device *device, nir_shader *nir,
         {
      struct dzn_instance *instance =
         struct dzn_physical_device *pdev =
                     if ((instance->debug_flags & DZN_DEBUG_NIR) &&
      (instance->debug_flags & DZN_DEBUG_INTERNAL))
         struct nir_to_dxil_options opts = {
      .environment = DXIL_ENVIRONMENT_VULKAN,
   #ifdef _WIN32
         #endif
      };
   struct blob dxil_blob;
   ASSERTED bool ret = nir_to_dxil(nir, &opts, NULL, &dxil_blob);
         #ifdef _WIN32
      char *err = NULL;
   bool res = dxil_validate_module(instance->dxil_validator,
                  if ((instance->debug_flags & DZN_DEBUG_DXIL) &&
      (instance->debug_flags & DZN_DEBUG_INTERNAL)) {
   char *disasm = dxil_disasm_module(instance->dxil_validator,
               if (disasm) {
      fprintf(stderr,
         "== BEGIN SHADER ============================================\n"
   "%s\n"
   "== END SHADER ==============================================\n",
                  if ((instance->debug_flags & DZN_DEBUG_DXIL) &&
      (instance->debug_flags & DZN_DEBUG_INTERNAL) &&
   !res) {
   fprintf(stderr,
         "== VALIDATION ERROR =============================================\n"
   "%s\n"
   "== END ==========================================================\n",
      }
      #endif
         void *data;
   size_t size;
   blob_finish_get_buffer(&dxil_blob, &data, &size);
   slot->pShaderBytecode = data;
      }
      #define DZN_META_INDIRECT_DRAW_MAX_PARAM_COUNT 5
      static void
   dzn_meta_indirect_draw_finish(struct dzn_device *device, enum dzn_indirect_draw_type type)
   {
               if (meta->root_sig)
            if (meta->pipeline_state)
      }
      static VkResult
   dzn_meta_indirect_draw_init(struct dzn_device *device,
         {
      struct dzn_meta_indirect_draw *meta = &device->indirect_draws[type];
   struct dzn_instance *instance =
                           nir_shader *nir = dzn_nir_indirect_draw_shader(type);
   bool triangle_fan = type == DZN_INDIRECT_DRAW_TRIANGLE_FAN ||
                     type == DZN_INDIRECT_DRAW_COUNT_TRIANGLE_FAN ||
   type == DZN_INDIRECT_INDEXED_DRAW_TRIANGLE_FAN ||
   bool indirect_count = type == DZN_INDIRECT_DRAW_COUNT ||
                           bool prim_restart = type == DZN_INDIRECT_INDEXED_DRAW_TRIANGLE_FAN_PRIM_RESTART ||
         uint32_t shader_params_size =
      triangle_fan && prim_restart ?
   sizeof(struct dzn_indirect_draw_triangle_fan_prim_restart_rewrite_params) :
   triangle_fan ?
   sizeof(struct dzn_indirect_draw_triangle_fan_rewrite_params) :
         uint32_t root_param_count = 0;
            root_params[root_param_count++] = (D3D12_ROOT_PARAMETER1) {
      .ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS,
   .Constants = {
      .ShaderRegister = 0,
   .RegisterSpace = 0,
      },
               root_params[root_param_count++] = (D3D12_ROOT_PARAMETER1) {
      .ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV,
   .Descriptor = {
      .ShaderRegister = 1,
   .RegisterSpace = 0,
      },
               root_params[root_param_count++] = (D3D12_ROOT_PARAMETER1) {
      .ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV,
   .Descriptor = {
      .ShaderRegister = 2,
   .RegisterSpace = 0,
      },
               if (indirect_count) {
      root_params[root_param_count++] = (D3D12_ROOT_PARAMETER1) {
      .ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV,
   .Descriptor = {
      .ShaderRegister = 3,
   .RegisterSpace = 0,
      },
                     if (triangle_fan) {
      root_params[root_param_count++] = (D3D12_ROOT_PARAMETER1) {
      .ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV,
   .Descriptor = {
      .ShaderRegister = 4,
   .RegisterSpace = 0,
      },
                           D3D12_VERSIONED_ROOT_SIGNATURE_DESC root_sig_desc = {
      .Version = D3D_ROOT_SIGNATURE_VERSION_1_1,
   .Desc_1_1 = {
      .NumParameters = root_param_count,
   .pParameters = root_params,
                  D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {
                  meta->root_sig =
         if (!meta->root_sig) {
      ret = vk_error(instance, VK_ERROR_INITIALIZATION_FAILED);
               desc.pRootSignature = meta->root_sig;
   dzn_meta_compile_shader(device, nir, &desc.CS);
            if (FAILED(ID3D12Device1_CreateComputePipelineState(device->dev, &desc,
                     out:
      if (ret != VK_SUCCESS)
            free((void *)desc.CS.pShaderBytecode);
   ralloc_free(nir);
               }
      #define DZN_META_TRIANGLE_FAN_REWRITE_IDX_MAX_PARAM_COUNT 4
      static void
   dzn_meta_triangle_fan_rewrite_index_finish(struct dzn_device *device,
         {
      struct dzn_meta_triangle_fan_rewrite_index *meta =
            if (meta->root_sig)
         if (meta->pipeline_state)
         if (meta->cmd_sig)
      }
      static VkResult
   dzn_meta_triangle_fan_rewrite_index_init(struct dzn_device *device,
         {
      struct dzn_meta_triangle_fan_rewrite_index *meta =
         struct dzn_instance *instance =
                           uint8_t old_index_size = dzn_index_size(old_index_type);
   bool prim_restart =
      old_index_type == DZN_INDEX_2B_WITH_PRIM_RESTART ||
         nir_shader *nir =
      prim_restart ?
   dzn_nir_triangle_fan_prim_restart_rewrite_index_shader(old_index_size) :
         uint32_t root_param_count = 0;
            root_params[root_param_count++] = (D3D12_ROOT_PARAMETER1) {
      .ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV,
   .Descriptor = {
      .ShaderRegister = 1,
   .RegisterSpace = 0,
      },
               uint32_t params_size =
      prim_restart ?
   sizeof(struct dzn_triangle_fan_prim_restart_rewrite_index_params) :
         root_params[root_param_count++] = (D3D12_ROOT_PARAMETER1) {
      .ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS,
   .Constants = {
      .ShaderRegister = 0,
   .RegisterSpace = 0,
      },
               if (old_index_type != DZN_NO_INDEX) {
      root_params[root_param_count++] = (D3D12_ROOT_PARAMETER1) {
      .ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV,
   .Descriptor = {
      .ShaderRegister = 2,
   .RegisterSpace = 0,
      },
                  if (prim_restart) {
      root_params[root_param_count++] = (D3D12_ROOT_PARAMETER1) {
      .ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV,
   .Descriptor = {
      .ShaderRegister = 3,
   .RegisterSpace = 0,
      },
                           D3D12_VERSIONED_ROOT_SIGNATURE_DESC root_sig_desc = {
      .Version = D3D_ROOT_SIGNATURE_VERSION_1_1,
   .Desc_1_1 = {
      .NumParameters = root_param_count,
   .pParameters = root_params,
                  D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {
                  uint32_t cmd_arg_count = 0;
            cmd_args[cmd_arg_count++] = (D3D12_INDIRECT_ARGUMENT_DESC) {
      .Type = D3D12_INDIRECT_ARGUMENT_TYPE_UNORDERED_ACCESS_VIEW,
   .UnorderedAccessView = {
                     cmd_args[cmd_arg_count++] = (D3D12_INDIRECT_ARGUMENT_DESC) {
      .Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT,
   .Constant = {
      .RootParameterIndex = 1,
   .DestOffsetIn32BitValues = 0,
                  if (prim_restart) {
      cmd_args[cmd_arg_count++] = (D3D12_INDIRECT_ARGUMENT_DESC) {
      .Type = D3D12_INDIRECT_ARGUMENT_TYPE_UNORDERED_ACCESS_VIEW,
   .UnorderedAccessView = {
                        cmd_args[cmd_arg_count++] = (D3D12_INDIRECT_ARGUMENT_DESC) {
                           uint32_t exec_params_size =
      prim_restart ?
   sizeof(struct dzn_indirect_triangle_fan_prim_restart_rewrite_index_exec_params) :
         D3D12_COMMAND_SIGNATURE_DESC cmd_sig_desc = {
      .ByteStride = exec_params_size,
   .NumArgumentDescs = cmd_arg_count,
                        meta->root_sig = dzn_device_create_root_sig(device, &root_sig_desc);
   if (!meta->root_sig) {
      ret = vk_error(instance, VK_ERROR_INITIALIZATION_FAILED);
                  desc.pRootSignature = meta->root_sig;
            if (FAILED(ID3D12Device1_CreateComputePipelineState(device->dev, &desc,
                  ret = vk_error(instance, VK_ERROR_INITIALIZATION_FAILED);
               if (FAILED(ID3D12Device1_CreateCommandSignature(device->dev, &cmd_sig_desc,
                           out:
      if (ret != VK_SUCCESS)
            free((void *)desc.CS.pShaderBytecode);
   ralloc_free(nir);
               }
      static const D3D12_SHADER_BYTECODE *
   dzn_meta_blits_get_vs(struct dzn_device *device)
   {
                        if (meta->vs.pShaderBytecode == NULL) {
                        gl_system_value system_values[] = {
      SYSTEM_VALUE_FIRST_VERTEX,
               NIR_PASS_V(nir, dxil_nir_lower_system_values_to_zero, system_values,
                     dzn_meta_compile_shader(device, nir, &bc);
   meta->vs.pShaderBytecode =
      vk_alloc(&device->vk.alloc, bc.BytecodeLength, 8,
      if (meta->vs.pShaderBytecode) {
      meta->vs.BytecodeLength = bc.BytecodeLength;
      }
   free((void *)bc.pShaderBytecode);
                           }
      static const D3D12_SHADER_BYTECODE *
   dzn_meta_blits_get_fs(struct dzn_device *device,
         {
      struct dzn_meta_blits *meta = &device->blits;
                              struct hash_entry *he =
            if (!he) {
               if (info->out_type != GLSL_TYPE_FLOAT) {
      dxil_wrap_sampler_state wrap_state = {
      .is_int_sampler = 1,
   .is_linear_filtering = 0,
      };
                                 out = vk_alloc(&device->vk.alloc,
               if (out) {
      out->pShaderBytecode = out + 1;
   memcpy((void *)out->pShaderBytecode, bc.pShaderBytecode, bc.BytecodeLength);
   out->BytecodeLength = bc.BytecodeLength;
      }
   free((void *)bc.pShaderBytecode);
      } else {
                              }
      static void
   dzn_meta_blit_destroy(struct dzn_device *device, struct dzn_meta_blit *blit)
   {
      if (!blit)
            if (blit->root_sig)
         if (blit->pipeline_state)
               }
      static struct dzn_meta_blit *
   dzn_meta_blit_create(struct dzn_device *device, const struct dzn_meta_blit_key *key)
   {
      struct dzn_meta_blit *blit =
      vk_zalloc(&device->vk.alloc, sizeof(*blit), 8,
         if (!blit)
            D3D12_DESCRIPTOR_RANGE1 ranges[] = {
      {
      .RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
   .NumDescriptors = 1,
   .BaseShaderRegister = 0,
   .RegisterSpace = 0,
   .Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_STATIC_KEEPING_BUFFER_BOUNDS_CHECKS,
                  D3D12_STATIC_SAMPLER_DESC samplers[] = {
      {
      .Filter = key->linear_filter ?
               .AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
   .AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
   .AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
   .MipLODBias = 0,
   .MaxAnisotropy = 0,
   .MinLOD = 0,
   .MaxLOD = D3D12_FLOAT32_MAX,
   .ShaderRegister = 0,
   .RegisterSpace = 0,
                  D3D12_ROOT_PARAMETER1 root_params[] = {
      {
      .ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
   .DescriptorTable = {
      .NumDescriptorRanges = ARRAY_SIZE(ranges),
      },
      },
   {
      .ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS,
   .Constants = {
      .ShaderRegister = 0,
   .RegisterSpace = 0,
      },
                  D3D12_VERSIONED_ROOT_SIGNATURE_DESC root_sig_desc = {
      .Version = D3D_ROOT_SIGNATURE_VERSION_1_1,
   .Desc_1_1 = {
      .NumParameters = ARRAY_SIZE(root_params),
   .pParameters = root_params,
   .NumStaticSamplers = ARRAY_SIZE(samplers),
   .pStaticSamplers = samplers,
                  uint32_t samples = key->resolve_mode == dzn_blit_resolve_none ?
         D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {
      .SampleMask = (1ULL << samples) - 1,
   .RasterizerState = {
      .FillMode = D3D12_FILL_MODE_SOLID,
   .CullMode = D3D12_CULL_MODE_NONE,
      },
   .PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
   .SampleDesc = {
      .Count = samples,
      },
               struct dzn_nir_blit_info blit_fs_info = {
      .src_samples = key->samples,
   .loc = key->loc,
   .out_type = key->out_type,
   .sampler_dim = key->sampler_dim,
   .src_is_array = key->src_is_array,
   .resolve_mode = key->resolve_mode,
               blit->root_sig = dzn_device_create_root_sig(device, &root_sig_desc);
   if (!blit->root_sig) {
      dzn_meta_blit_destroy(device, blit);
                                 vs = dzn_meta_blits_get_vs(device);
   if (!vs) {
      dzn_meta_blit_destroy(device, blit);
               desc.VS = *vs;
            fs = dzn_meta_blits_get_fs(device, &blit_fs_info);
   if (!fs) {
      dzn_meta_blit_destroy(device, blit);
               desc.PS = *fs;
            assert(key->loc == FRAG_RESULT_DATA0 ||
                  if (key->loc == FRAG_RESULT_DATA0) {
      desc.NumRenderTargets = 1;
   desc.RTVFormats[0] = key->out_format;
      } else {
      desc.DSVFormat = key->out_format;
   if (key->loc == FRAG_RESULT_DEPTH) {
      desc.DepthStencilState.DepthEnable = true;
   desc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
      } else {
      assert(key->loc == FRAG_RESULT_STENCIL);
   desc.DepthStencilState.StencilEnable = true;
   desc.DepthStencilState.StencilWriteMask = 0xff;
   desc.DepthStencilState.FrontFace.StencilFailOp = D3D12_STENCIL_OP_REPLACE;
   desc.DepthStencilState.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_REPLACE;
   desc.DepthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP_REPLACE;
   desc.DepthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
                  if (FAILED(ID3D12Device1_CreateGraphicsPipelineState(device->dev, &desc,
                  dzn_meta_blit_destroy(device, blit);
                  }
      const struct dzn_meta_blit *
   dzn_meta_blits_get_context(struct dzn_device *device,
         {
                                 out =
         if (!out) {
               if (out)
                           }
      static void
   dzn_meta_blits_finish(struct dzn_device *device)
   {
                        if (meta->fs) {
      hash_table_foreach(meta->fs, he)
                     if (meta->contexts) {
      hash_table_foreach(meta->contexts->table, he)
                     mtx_destroy(&meta->shaders_lock);
      }
      static VkResult
   dzn_meta_blits_init(struct dzn_device *device)
   {
      struct dzn_instance *instance =
                  mtx_init(&meta->shaders_lock, mtx_plain);
            meta->fs = _mesa_hash_table_create_u32_keys(NULL);
   if (!meta->fs) {
      dzn_meta_blits_finish(device);
               meta->contexts = _mesa_hash_table_u64_create(NULL);
   if (!meta->contexts) {
      dzn_meta_blits_finish(device);
                  }
      void
   dzn_meta_finish(struct dzn_device *device)
   {
      for (uint32_t i = 0; i < ARRAY_SIZE(device->triangle_fan); i++)
            for (uint32_t i = 0; i < ARRAY_SIZE(device->indirect_draws); i++)
               }
      VkResult
   dzn_meta_init(struct dzn_device *device)
   {
      VkResult result = dzn_meta_blits_init(device);
   if (result != VK_SUCCESS)
            for (uint32_t i = 0; i < ARRAY_SIZE(device->indirect_draws); i++) {
      VkResult result =
         if (result != VK_SUCCESS)
               for (uint32_t i = 0; i < ARRAY_SIZE(device->triangle_fan); i++) {
      VkResult result =
         if (result != VK_SUCCESS)
            out:
      if (result != VK_SUCCESS) {
      dzn_meta_finish(device);
                  }
