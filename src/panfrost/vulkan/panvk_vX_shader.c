   /*
   * Copyright © 2021 Collabora Ltd.
   *
   * Derived from tu_shader.c which is:
   * Copyright © 2019 Google LLC
   *
   * Also derived from anv_pipeline.c which is
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
      #include "genxml/gen_macros.h"
      #include "panvk_private.h"
      #include "spirv/nir_spirv.h"
   #include "util/mesa-sha1.h"
   #include "nir_builder.h"
   #include "nir_conversion_builder.h"
   #include "nir_deref.h"
   #include "nir_lower_blend.h"
   #include "vk_shader_module.h"
      #include "compiler/bifrost_nir.h"
   #include "util/pan_lower_framebuffer.h"
   #include "pan_shader.h"
      #include "vk_util.h"
      static nir_def *
   load_sysval_from_ubo(nir_builder *b, nir_intrinsic_instr *intr, unsigned offset)
   {
      return nir_load_ubo(b, intr->def.num_components, intr->def.bit_size,
                        }
      struct sysval_options {
      /* If non-null, a vec4 of blend constants known at pipeline compile time. If
   * null, blend constants are dynamic.
   */
      };
      static bool
   panvk_lower_sysvals(nir_builder *b, nir_instr *instr, void *data)
   {
      if (instr->type != nir_instr_type_intrinsic)
            struct sysval_options *opts = data;
   nir_intrinsic_instr *intr = nir_instr_as_intrinsic(instr);
   nir_def *val = NULL;
         #define SYSVAL(name) offsetof(struct panvk_sysvals, name)
      switch (intr->intrinsic) {
   case nir_intrinsic_load_num_workgroups:
      val = load_sysval_from_ubo(b, intr, SYSVAL(num_work_groups));
      case nir_intrinsic_load_workgroup_size:
      val = load_sysval_from_ubo(b, intr, SYSVAL(local_group_size));
      case nir_intrinsic_load_viewport_scale:
      val = load_sysval_from_ubo(b, intr, SYSVAL(viewport_scale));
      case nir_intrinsic_load_viewport_offset:
      val = load_sysval_from_ubo(b, intr, SYSVAL(viewport_offset));
      case nir_intrinsic_load_first_vertex:
      val = load_sysval_from_ubo(b, intr, SYSVAL(first_vertex));
      case nir_intrinsic_load_base_vertex:
      val = load_sysval_from_ubo(b, intr, SYSVAL(base_vertex));
      case nir_intrinsic_load_base_instance:
      val = load_sysval_from_ubo(b, intr, SYSVAL(base_instance));
      case nir_intrinsic_load_blend_const_color_rgba:
      if (opts->static_blend_constants) {
      const nir_const_value constants[4] = {
      {.f32 = opts->static_blend_constants[0]},
   {.f32 = opts->static_blend_constants[1]},
   {.f32 = opts->static_blend_constants[2]},
                  } else {
         }
      default:
            #undef SYSVAL
         b->cursor = nir_after_instr(instr);
   nir_def_rewrite_uses(&intr->def, val);
      }
      static void
   panvk_lower_blend(struct panfrost_device *pdev, nir_shader *nir,
               {
      nir_lower_blend_options options = {
      .logicop_enable = blend_state->logicop_enable,
                        for (unsigned rt = 0; rt < blend_state->rt_count; rt++) {
               if (!panvk_per_arch(blend_needs_lowering)(pdev, blend_state, rt))
                     options.format[rt] = fmt;
            if (!rt_state->equation.blend_enable) {
      static const nir_lower_blend_channel replace = {
      .func = PIPE_BLEND_ADD,
   .src_factor = PIPE_BLENDFACTOR_ONE,
               options.rt[rt].rgb = replace;
      } else {
      options.rt[rt].rgb.func = rt_state->equation.rgb_func;
   options.rt[rt].rgb.src_factor = rt_state->equation.rgb_src_factor;
   options.rt[rt].rgb.dst_factor = rt_state->equation.rgb_dst_factor;
   options.rt[rt].alpha.func = rt_state->equation.alpha_func;
   options.rt[rt].alpha.src_factor = rt_state->equation.alpha_src_factor;
               /* Update the equation to force a color replacement */
   rt_state->equation.color_mask = 0xf;
   rt_state->equation.rgb_func = PIPE_BLEND_ADD;
   rt_state->equation.rgb_src_factor = PIPE_BLENDFACTOR_ONE;
   rt_state->equation.rgb_dst_factor = PIPE_BLENDFACTOR_ZERO;
   rt_state->equation.alpha_func = PIPE_BLEND_ADD;
   rt_state->equation.alpha_src_factor = PIPE_BLENDFACTOR_ONE;
   rt_state->equation.alpha_dst_factor = PIPE_BLENDFACTOR_ZERO;
               if (lower_blend) {
      NIR_PASS_V(nir, nir_lower_blend, &options);
         }
      static bool
   panvk_lower_load_push_constant(nir_builder *b, nir_intrinsic_instr *intr,
         {
      if (intr->intrinsic != nir_intrinsic_load_push_constant)
            b->cursor = nir_before_instr(&intr->instr);
   nir_def *ubo_load =
      nir_load_ubo(b, intr->def.num_components, intr->def.bit_size,
               nir_imm_int(b, PANVK_PUSH_CONST_UBO_INDEX), intr->src[0].ssa,
      nir_def_rewrite_uses(&intr->def, ubo_load);
   nir_instr_remove(&intr->instr);
      }
      static void
   shared_type_info(const struct glsl_type *type, unsigned *size, unsigned *align)
   {
               uint32_t comp_size =
         unsigned length = glsl_get_vector_elements(type);
      }
      struct panvk_shader *
   panvk_per_arch(shader_create)(struct panvk_device *dev, gl_shader_stage stage,
                                 const VkPipelineShaderStageCreateInfo *stage_info,
   {
      VK_FROM_HANDLE(vk_shader_module, module, stage_info->module);
   struct panfrost_device *pdev = &dev->physical_device->pdev;
            shader = vk_zalloc2(&dev->vk.alloc, alloc, sizeof(*shader), 8,
         if (!shader)
                     /* TODO these are made-up */
   const struct spirv_to_nir_options spirv_options = {
      .caps =
      {
            .ubo_addr_format = nir_address_format_32bit_index_offset,
   .ssbo_addr_format = dev->vk.enabled_features.robustBufferAccess
                     nir_shader *nir;
   VkResult result = vk_shader_module_to_nir(
      &dev->vk, module, stage, stage_info->pName,
   stage_info->pSpecializationInfo, &spirv_options,
      if (result != VK_SUCCESS) {
      vk_free2(&dev->vk.alloc, alloc, shader);
               NIR_PASS_V(nir, nir_lower_io_to_temporaries, nir_shader_get_entrypoint(nir),
            struct panfrost_compile_inputs inputs = {
      .gpu_id = pdev->gpu_id,
   .no_ubo_to_push = true,
               NIR_PASS_V(nir, nir_lower_indirect_derefs,
            NIR_PASS_V(nir, nir_opt_copy_prop_vars);
   NIR_PASS_V(nir, nir_opt_combine_stores, nir_var_all);
            /* Do texture lowering here.  Yes, it's a duplication of the texture
   * lowering in bifrost_compile.  However, we need to lower texture stuff
   * now, before we call panvk_per_arch(nir_lower_descriptors)() because some
   * of the texture lowering generates nir_texop_txs which we handle as part
   * of descriptor lowering.
   *
   * TODO: We really should be doing this in common code, not dpulicated in
   * panvk.  In order to do that, we need to rework the panfrost compile
   * flow to look more like the Intel flow:
   *
   *  1. Compile SPIR-V to NIR and maybe do a tiny bit of lowering that needs
   *     to be done really early.
   *
   *  2. bi_preprocess_nir: Does common lowering and runs the optimization
   *     loop.  Nothing here should be API-specific.
   *
   *  3. Do additional lowering in panvk
   *
   *  4. bi_postprocess_nir: Does final lowering and runs the optimization
   *     loop again.  This can happen as part of the final compile.
   *
   * This would give us a better place to do panvk-specific lowering.
   */
   nir_lower_tex_options lower_tex_options = {
      .lower_txs_lod = true,
   .lower_txp = ~0,
   .lower_tg4_broadcom_swizzle = true,
   .lower_txd = true,
      };
            NIR_PASS_V(nir, panvk_per_arch(nir_lower_descriptors), dev, layout,
            NIR_PASS_V(nir, nir_lower_explicit_io, nir_var_mem_ubo,
         NIR_PASS_V(nir, nir_lower_explicit_io, nir_var_mem_ssbo,
         NIR_PASS_V(nir, nir_lower_explicit_io, nir_var_mem_push_const,
            if (gl_shader_stage_uses_workgroup(stage)) {
      if (!nir->info.shared_memory_explicit_layout) {
      NIR_PASS_V(nir, nir_lower_vars_to_explicit_types, nir_var_mem_shared,
               NIR_PASS_V(nir, nir_lower_explicit_io, nir_var_mem_shared,
               NIR_PASS_V(nir, nir_shader_intrinsics_pass, panvk_lower_load_push_constant,
                  NIR_PASS_V(nir, nir_lower_system_values);
            NIR_PASS_V(nir, nir_split_var_copies);
            nir_assign_io_var_locations(nir, nir_var_shader_in, &nir->num_inputs, stage);
   nir_assign_io_var_locations(nir, nir_var_shader_out, &nir->num_outputs,
            /* Needed to turn shader_temp into function_temp since the backend only
   * handles the latter for now.
   */
            nir_shader_gather_info(nir, nir_shader_get_entrypoint(nir));
   if (unlikely(dev->physical_device->instance->debug_flags &
            fprintf(stderr, "translated nir:\n");
                        if (stage == MESA_SHADER_FRAGMENT) {
                  struct sysval_options sysval_options = {
      .static_blend_constants =
               NIR_PASS_V(nir, nir_shader_instructions_pass, panvk_lower_sysvals,
                  if (stage == MESA_SHADER_FRAGMENT) {
               for (unsigned rt = 0; rt < MAX_RTS; ++rt)
                                 /* Patch the descriptor count */
   shader->info.ubo_count =
         shader->info.sampler_count = layout->num_samplers;
   shader->info.texture_count = layout->num_textures;
   if (shader->has_img_access)
            shader->sysval_ubo = sysval_ubo;
   shader->local_size.x = nir->info.workgroup_size[0];
   shader->local_size.y = nir->info.workgroup_size[1];
                        }
