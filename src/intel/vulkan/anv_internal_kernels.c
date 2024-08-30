   /*
   * Copyright © 2022 Intel Corporation
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
      #include "anv_private.h"
      #include "compiler/brw_compiler.h"
   #include "compiler/brw_nir.h"
   #include "compiler/spirv/nir_spirv.h"
   #include "dev/intel_debug.h"
   #include "util/macros.h"
      #include "vk_nir.h"
      #include "anv_internal_kernels.h"
      #include "shaders/generated_draws_spv.h"
   #include "shaders/query_copy_compute_spv.h"
   #include "shaders/query_copy_fragment_spv.h"
   #include "shaders/memcpy_compute_spv.h"
      static bool
   lower_vulkan_descriptors_instr(nir_builder *b, nir_intrinsic_instr *intrin,
         {
      if (intrin->intrinsic != nir_intrinsic_load_vulkan_descriptor)
            nir_instr *res_index_instr = intrin->src[0].ssa->parent_instr;
   assert(res_index_instr->type == nir_instr_type_intrinsic);
   nir_intrinsic_instr *res_index_intrin =
                           const struct anv_internal_kernel_bind_map *bind_map = cb_data;
   uint32_t binding = nir_intrinsic_binding(res_index_intrin);
            nir_def *desc_value = NULL;
   if (bind_map->bindings[binding].push_constant) {
      desc_value =
      nir_vec2(b,
         } else {
      int push_constant_binding = -1;
   for (uint32_t i = 0; i < bind_map->num_bindings; i++) {
      if (bind_map->bindings[i].push_constant) {
      push_constant_binding = i;
         }
            desc_value =
      nir_load_ubo(b, 1, 64,
               nir_imm_int(b, push_constant_binding),
   nir_imm_int(b,
         .align_mul = 8,
      desc_value =
      nir_vec4(b,
            nir_unpack_64_2x32_split_x(b, desc_value),
                           }
      static bool
   lower_vulkan_descriptors(nir_shader *shader,
         {
      return nir_shader_intrinsics_pass(shader, lower_vulkan_descriptors_instr,
                  }
      static bool
   lower_base_workgroup_id(nir_builder *b, nir_intrinsic_instr *intrin,
         {
      if (intrin->intrinsic != nir_intrinsic_load_base_workgroup_id)
            b->cursor = nir_instr_remove(&intrin->instr);
   nir_def_rewrite_uses(&intrin->def, nir_imm_zero(b, 3, 32));
      }
      static bool
   lower_load_ubo_to_uniforms(nir_builder *b, nir_intrinsic_instr *intrin,
         {
      if (intrin->intrinsic != nir_intrinsic_load_ubo)
                     nir_def_rewrite_uses(
      &intrin->def,
   nir_load_uniform(b,
                  intrin->def.num_components,
   intrin->def.bit_size,
               }
      static struct anv_shader_bin *
   compile_upload_spirv(struct anv_device *device,
                        gl_shader_stage stage,
   const char *name,
   const void *hash_key,
   uint32_t hash_key_size,
      {
      struct spirv_to_nir_options spirv_options = {
      .caps = {
         },
   .ubo_addr_format = nir_address_format_32bit_index_offset,
   .ssbo_addr_format = nir_address_format_64bit_global_32bit_offset,
   .environment = NIR_SPIRV_VULKAN,
      };
   const nir_shader_compiler_options *nir_options =
            nir_shader* nir =
      vk_spirv_to_nir(&device->vk, spirv_source, spirv_source_size * 4,
                                       NIR_PASS_V(nir, nir_lower_vars_to_ssa);
   NIR_PASS_V(nir, nir_opt_cse);
   NIR_PASS_V(nir, nir_opt_gcm, true);
                     NIR_PASS_V(nir, nir_split_var_copies);
            struct brw_compiler *compiler = device->physical->compiler;
   struct brw_nir_compiler_opts opts = {};
                     if (stage == MESA_SHADER_FRAGMENT) {
      NIR_PASS_V(nir, nir_lower_input_attachments,
            &(nir_input_attachment_options) {
         } else {
      nir_lower_compute_system_values_options options = {
      .has_base_workgroup_id = true,
   .lower_cs_local_id_to_index = true,
      };
   NIR_PASS_V(nir, nir_lower_compute_system_values, &options);
   NIR_PASS_V(nir, nir_shader_intrinsics_pass, lower_base_workgroup_id,
                        /* Do vectorizing here. For some reason when trying to do it in the back
   * this just isn't working.
   */
   nir_load_store_vectorize_options options = {
      .modes = nir_var_mem_ubo | nir_var_mem_ssbo,
   .callback = brw_nir_should_vectorize_mem,
      };
                     NIR_PASS_V(nir, nir_lower_explicit_io, nir_var_mem_ubo,
         NIR_PASS_V(nir, nir_lower_explicit_io, nir_var_mem_ssbo,
            NIR_PASS_V(nir, nir_copy_prop);
   NIR_PASS_V(nir, nir_opt_constant_folding);
            if (stage == MESA_SHADER_COMPUTE) {
      NIR_PASS_V(nir, nir_shader_intrinsics_pass, lower_load_ubo_to_uniforms,
               NIR_PASS_V(nir, brw_nir_lower_cs_intrinsics);
               union brw_any_prog_key key;
            union brw_any_prog_data prog_data;
   memset(&prog_data, 0, sizeof(prog_data));
                              const unsigned *program;
   if (stage == MESA_SHADER_FRAGMENT) {
      struct brw_compile_stats stats[3];
   struct brw_compile_fs_params params = {
      .base = {
      .nir = nir,
   .log_data = device,
   .debug_flag = DEBUG_WM,
   .stats = stats,
      },
   .key = &key.wm,
      };
            unsigned stat_idx = 0;
   if (prog_data.wm.dispatch_8) {
      assert(stats[stat_idx].spills == 0);
   assert(stats[stat_idx].fills == 0);
   assert(stats[stat_idx].sends == sends_count_expectation);
      }
   if (prog_data.wm.dispatch_16) {
      assert(stats[stat_idx].spills == 0);
   assert(stats[stat_idx].fills == 0);
   assert(stats[stat_idx].sends == sends_count_expectation);
      }
   if (prog_data.wm.dispatch_32) {
      assert(stats[stat_idx].spills == 0);
   assert(stats[stat_idx].fills == 0);
   assert(stats[stat_idx].sends == sends_count_expectation * 2);
         } else {
      struct brw_compile_stats stats;
   struct brw_compile_cs_params params = {
      .base = {
      .nir = nir,
   .stats = &stats,
   .log_data = device,
   .debug_flag = DEBUG_CS,
      },
   .key = &key.cs,
      };
            assert(stats.spills == 0);
   assert(stats.fills == 0);
               struct anv_pipeline_bind_map dummy_bind_map;
                     struct anv_shader_bin *kernel =
      anv_device_upload_kernel(device,
                           device->internal_cache,
   nir->info.stage,
   hash_key, hash_key_size, program,
         ralloc_free(temp_ctx);
               }
      VkResult
   anv_device_init_internal_kernels(struct anv_device *device)
   {
      const struct intel_l3_weights w =
      intel_get_default_l3_weights(device->info,
                     const struct {
      struct {
                           const uint32_t *spirv_data;
                        } internal_kernels[] = {
      [ANV_INTERNAL_KERNEL_GENERATED_DRAWS] = {
      .key        = {
         },
   .stage      = MESA_SHADER_FRAGMENT,
   .spirv_data = generated_draws_spv_source,
   .spirv_size = ARRAY_SIZE(generated_draws_spv_source),
   .send_count = /* 2 * (2 loads + 3 stores) +  ** gfx11 **
                     .bind_map   = {
      .num_bindings = 5,
   .bindings     = {
      {
      .address_offset = offsetof(struct anv_generated_indirect_params,
      },
   {
      .address_offset = offsetof(struct anv_generated_indirect_params,
      },
   {
      .address_offset = offsetof(struct anv_generated_indirect_params,
      },
   {
      .address_offset = offsetof(struct anv_generated_indirect_params,
      },
   {
            },
         },
   [ANV_INTERNAL_KERNEL_COPY_QUERY_RESULTS_COMPUTE] = {
      .key        = {
         },
   .stage      = MESA_SHADER_COMPUTE,
   .spirv_data = query_copy_compute_spv_source,
   .spirv_size = ARRAY_SIZE(query_copy_compute_spv_source),
   .send_count = device->info->verx10 >= 125 ?
               .bind_map   = {
      .num_bindings = 3,
   .bindings     = {
      {
      .address_offset = offsetof(struct anv_query_copy_params,
      },
   {
      .address_offset = offsetof(struct anv_query_copy_params,
      },
   {
            },
         },
   [ANV_INTERNAL_KERNEL_COPY_QUERY_RESULTS_FRAGMENT] = {
      .key        = {
         },
   .stage      = MESA_SHADER_FRAGMENT,
   .spirv_data = query_copy_fragment_spv_source,
   .spirv_size = ARRAY_SIZE(query_copy_fragment_spv_source),
   .send_count = 8 /* 3 loads + 4 stores + 1 EOT */,
   .bind_map   = {
      .num_bindings = 3,
   .bindings     = {
      {
      .address_offset = offsetof(struct anv_query_copy_params,
      },
   {
      .address_offset = offsetof(struct anv_query_copy_params,
      },
   {
            },
         },
   [ANV_INTERNAL_KERNEL_MEMCPY_COMPUTE] = {
      .key        = {
         },
   .stage      = MESA_SHADER_COMPUTE,
   .spirv_data = memcpy_compute_spv_source,
   .spirv_size = ARRAY_SIZE(memcpy_compute_spv_source),
   .send_count = device->info->verx10 >= 125 ?
               .bind_map   = {
      .num_bindings = 3,
   .bindings     = {
      {
      .address_offset = offsetof(struct anv_memcpy_params,
      },
   {
      .address_offset = offsetof(struct anv_memcpy_params,
      },
   {
            },
                     for (uint32_t i = 0; i < ARRAY_SIZE(internal_kernels); i++) {
      device->internal_kernels[i] =
      anv_device_search_for_kernel(device,
                        if (device->internal_kernels[i] == NULL) {
      device->internal_kernels[i] =
      compile_upload_spirv(device,
                        internal_kernels[i].stage,
   internal_kernels[i].key.name,
   &internal_kernels[i].key,
      }
   if (device->internal_kernels[i] == NULL)
            /* The cache already has a reference and it's not going anywhere so
   * there is no need to hold a second reference.
   */
                  }
      void
   anv_device_finish_internal_kernels(struct anv_device *device)
   {
   }
