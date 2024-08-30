   /*
   * Copyright © 2022 Collabora Ltd. and Red Hat Inc.
   * SPDX-License-Identifier: MIT
   */
   #include "nvk_cmd_buffer.h"
   #include "nvk_descriptor_set.h"
   #include "nvk_descriptor_set_layout.h"
   #include "nvk_shader.h"
      #include "vk_pipeline.h"
   #include "vk_pipeline_layout.h"
      #include "nir_builder.h"
   #include "nir_deref.h"
      struct lower_descriptors_ctx {
      const struct vk_pipeline_layout *layout;
   bool clamp_desc_array_bounds;
   nir_address_format ubo_addr_format;
      };
      static nir_def *
   load_descriptor_set_addr(nir_builder *b, uint32_t set,
         {
      uint32_t set_addr_offset =
            return nir_load_ubo(b, 1, 64, nir_imm_int(b, 0),
            }
      static const struct nvk_descriptor_set_binding_layout *
   get_binding_layout(uint32_t set, uint32_t binding,
         {
               assert(set < layout->set_count);
   const struct nvk_descriptor_set_layout *set_layout =
            assert(binding < set_layout->binding_count);
      }
      static nir_def *
   load_descriptor(nir_builder *b, unsigned num_components, unsigned bit_size,
               {
      const struct nvk_descriptor_set_binding_layout *binding_layout =
            if (ctx->clamp_desc_array_bounds)
            switch (binding_layout->type) {
   case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC: {
      /* Get the index in the root descriptor table dynamic_buffers array. */
   uint8_t dynamic_buffer_start =
            index = nir_iadd_imm(b, index,
                  nir_def *root_desc_offset =
                  return nir_load_ubo(b, num_components, bit_size,
                     case VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK: {
      nir_def *base_addr =
                  assert(binding_layout->stride == 1);
            /* Convert it to nir_address_format_64bit_bounded_global */
   assert(num_components == 4 && bit_size == 32);
   return nir_vec4(b, nir_unpack_64_2x32_split_x(b, base_addr),
                           default: {
      assert(binding_layout->stride > 0);
   nir_def *desc_ubo_offset =
                  unsigned desc_align = (1 << (ffs(binding_layout->stride) - 1));
            nir_def *set_addr = load_descriptor_set_addr(b, set, ctx);
   return nir_load_global_constant_offset(b, num_components, bit_size,
                  }
      }
      static bool
   is_idx_intrin(nir_intrinsic_instr *intrin)
   {
      while (intrin->intrinsic == nir_intrinsic_vulkan_resource_reindex) {
      intrin = nir_src_as_intrinsic(intrin->src[0]);
   if (intrin == NULL)
                  }
      static nir_def *
   load_descriptor_for_idx_intrin(nir_builder *b, nir_intrinsic_instr *intrin,
         {
               while (intrin->intrinsic == nir_intrinsic_vulkan_resource_reindex) {
      index = nir_iadd(b, index, intrin->src[1].ssa);
               assert(intrin->intrinsic == nir_intrinsic_vulkan_resource_index);
   uint32_t set = nir_intrinsic_desc_set(intrin);
   uint32_t binding = nir_intrinsic_binding(intrin);
               }
      static bool
   try_lower_load_vulkan_descriptor(nir_builder *b, nir_intrinsic_instr *intrin,
         {
      ASSERTED const VkDescriptorType desc_type = nir_intrinsic_desc_type(intrin);
            nir_intrinsic_instr *idx_intrin = nir_src_as_intrinsic(intrin->src[0]);
   if (idx_intrin == NULL || !is_idx_intrin(idx_intrin)) {
      assert(desc_type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ||
                                          }
      static bool
   lower_num_workgroups(nir_builder *b, nir_intrinsic_instr *load,
         {
      const uint32_t root_table_offset =
                     nir_def *val = nir_load_ubo(b, 3, 32,
                                                }
      static bool
   lower_load_base_workgroup_id(nir_builder *b, nir_intrinsic_instr *load,
         {
      const uint32_t root_table_offset =
                     nir_def *val = nir_load_ubo(b, 3, 32,
                                                }
      static bool
   lower_load_push_constant(nir_builder *b, nir_intrinsic_instr *load,
         {
      const uint32_t push_region_offset =
                           nir_def *offset = nir_iadd_imm(b, load->src[0].ssa,
            nir_def *val =
      nir_load_ubo(b, load->def.num_components, load->def.bit_size,
               nir_imm_int(b, 0), offset,
   .align_mul = load->def.bit_size / 8,
                     }
      static bool
   lower_load_view_index(nir_builder *b, nir_intrinsic_instr *load,
         {
      const uint32_t root_table_offset =
                     nir_def *val = nir_load_ubo(b, 1, 32,
                                                }
      static void
   get_resource_deref_binding(nir_builder *b, nir_deref_instr *deref,
               {
      if (deref->deref_type == nir_deref_type_array) {
      *index = deref->arr.index.ssa;
      } else {
                  assert(deref->deref_type == nir_deref_type_var);
            *set = var->data.descriptor_set;
      }
      static nir_def *
   load_resource_deref_desc(nir_builder *b, 
                     {
      uint32_t set, binding;
   nir_def *index;
   get_resource_deref_binding(b, deref, &set, &binding, &index);
   return load_descriptor(b, num_components, bit_size,
      }
      static bool
   lower_image_intrin(nir_builder *b, nir_intrinsic_instr *intrin,
         {
      b->cursor = nir_before_instr(&intrin->instr);
   nir_deref_instr *deref = nir_src_as_deref(intrin->src[0]);
   nir_def *desc = load_resource_deref_desc(b, 1, 32, deref, 0, ctx);
            /* We treat 3D images as 2D arrays */
   if (nir_intrinsic_image_dim(intrin) == GLSL_SAMPLER_DIM_3D) {
      assert(!nir_intrinsic_image_array(intrin));
   nir_intrinsic_set_image_dim(intrin, GLSL_SAMPLER_DIM_2D);
               /* We don't support ReadWithoutFormat yet */
   if (intrin->intrinsic == nir_intrinsic_image_deref_load)
               }
      static bool
   lower_interp_at_sample(nir_builder *b, nir_intrinsic_instr *interp,
         {
      const uint32_t root_table_offset =
                              nir_def *loc = nir_load_ubo(b, 1, 64,
                                    /* Yay little endian */
   loc = nir_ushr(b, loc, nir_imul_imm(b, sample, 8));
   nir_def *loc_x_u4 = nir_iand_imm(b, loc, 0xf);
   nir_def *loc_y_u4 = nir_iand_imm(b, nir_ushr_imm(b, loc, 4), 0xf);
   nir_def *loc_u4 = nir_vec2(b, loc_x_u4, loc_y_u4);
   nir_def *loc_f = nir_fmul_imm(b, nir_i2f32(b, loc_u4), 1.0 / 16.0);
            assert(interp->intrinsic == nir_intrinsic_interp_deref_at_sample);
   interp->intrinsic = nir_intrinsic_interp_deref_at_offset;
               }
      static bool
   try_lower_intrin(nir_builder *b, nir_intrinsic_instr *intrin,
         {
      switch (intrin->intrinsic) {
   case nir_intrinsic_load_vulkan_descriptor:
            case nir_intrinsic_load_workgroup_size:
            case nir_intrinsic_load_num_workgroups:
            case nir_intrinsic_load_base_workgroup_id:
            case nir_intrinsic_load_push_constant:
            case nir_intrinsic_load_view_index:
            case nir_intrinsic_image_deref_load:
   case nir_intrinsic_image_deref_store:
   case nir_intrinsic_image_deref_atomic:
   case nir_intrinsic_image_deref_atomic_swap:
   case nir_intrinsic_image_deref_size:
   case nir_intrinsic_image_deref_samples:
   case nir_intrinsic_image_deref_load_param_intel:
   case nir_intrinsic_image_deref_load_raw_intel:
   case nir_intrinsic_image_deref_store_raw_intel:
            case nir_intrinsic_interp_deref_at_sample:
            default:
            }
      static bool
   lower_tex(nir_builder *b, nir_tex_instr *tex,
         {
               const int texture_src_idx =
         const int sampler_src_idx =
         if (texture_src_idx < 0) {
      assert(sampler_src_idx < 0);
               nir_deref_instr *texture = nir_src_as_deref(tex->src[texture_src_idx].src);
   nir_deref_instr *sampler = sampler_src_idx < 0 ? NULL :
                  nir_def *plane_ssa = nir_steal_tex_src(tex, nir_tex_src_plane);
   const uint32_t plane =
                  nir_def *combined_handle;
   if (texture == sampler) {
         } else {
      nir_def *texture_desc =
         combined_handle = nir_iand_imm(b, texture_desc,
            if (sampler != NULL) {
      nir_def *sampler_desc =
         nir_def *sampler_index =
      nir_iand_imm(b, sampler_desc,
                     /* TODO: The nv50 back-end assumes it's 64-bit because of GL */
            /* TODO: The nv50 back-end assumes it gets handles both places, even for
   * texelFetch.
   */
   nir_src_rewrite(&tex->src[texture_src_idx].src, combined_handle);
            if (sampler_src_idx < 0) {
         } else {
      nir_src_rewrite(&tex->src[sampler_src_idx].src, combined_handle);
                  }
      static bool
   try_lower_descriptors_instr(nir_builder *b, nir_instr *instr,
         {
               switch (instr->type) {
   case nir_instr_type_tex:
         case nir_instr_type_intrinsic:
         default:
            }
      static bool
   lower_ssbo_resource_index(nir_builder *b, nir_intrinsic_instr *intrin,
         {
      const VkDescriptorType desc_type = nir_intrinsic_desc_type(intrin);
   if (desc_type != VK_DESCRIPTOR_TYPE_STORAGE_BUFFER &&
      desc_type != VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC)
                  uint32_t set = nir_intrinsic_desc_set(intrin);
   uint32_t binding = nir_intrinsic_binding(intrin);
            const struct nvk_descriptor_set_binding_layout *binding_layout =
            nir_def *binding_addr;
   uint8_t binding_stride;
   switch (binding_layout->type) {
   case VK_DESCRIPTOR_TYPE_MUTABLE_EXT:
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER: {
      nir_def *set_addr = load_descriptor_set_addr(b, set, ctx);
   binding_addr = nir_iadd_imm(b, set_addr, binding_layout->offset);
   binding_stride = binding_layout->stride;
               case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC: {
      const uint32_t root_desc_addr_offset =
            nir_def *root_desc_addr =
      nir_load_ubo(b, 1, 64, nir_imm_int(b, 0),
               const uint8_t dynamic_buffer_start =
                  const uint32_t dynamic_binding_offset =
                  binding_addr = nir_iadd_imm(b, root_desc_addr, dynamic_binding_offset);
   binding_stride = sizeof(struct nvk_buffer_address);
               default:
                  /* Tuck the stride in the top 8 bits of the binding address */
            const uint32_t binding_size = binding_layout->array_size * binding_stride;
            nir_def *addr;
   switch (ctx->ssbo_addr_format) {
   case nir_address_format_64bit_global:
      addr = nir_iadd(b, binding_addr, nir_u2u64(b, offset_in_binding));
         case nir_address_format_64bit_global_32bit_offset:
   case nir_address_format_64bit_bounded_global:
      addr = nir_vec4(b, nir_unpack_64_2x32_split_x(b, binding_addr),
                           default:
                              }
      static bool
   lower_ssbo_resource_reindex(nir_builder *b, nir_intrinsic_instr *intrin,
         {
      const VkDescriptorType desc_type = nir_intrinsic_desc_type(intrin);
   if (desc_type != VK_DESCRIPTOR_TYPE_STORAGE_BUFFER &&
      desc_type != VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC)
                  nir_def *addr = intrin->src[0].ssa;
            nir_def *addr_high32;
   switch (ctx->ssbo_addr_format) {
   case nir_address_format_64bit_global:
      addr_high32 = nir_unpack_64_2x32_split_y(b, addr);
         case nir_address_format_64bit_global_32bit_offset:
   case nir_address_format_64bit_bounded_global:
      addr_high32 = nir_channel(b, addr, 1);
         default:
                  nir_def *stride = nir_ushr_imm(b, addr_high32, 24);
            addr = nir_build_addr_iadd(b, addr, ctx->ssbo_addr_format,
                     }
      static bool
   lower_load_ssbo_descriptor(nir_builder *b, nir_intrinsic_instr *intrin,
         {
      const VkDescriptorType desc_type = nir_intrinsic_desc_type(intrin);
   if (desc_type != VK_DESCRIPTOR_TYPE_STORAGE_BUFFER &&
      desc_type != VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC)
                           nir_def *desc;
   switch (ctx->ssbo_addr_format) {
   case nir_address_format_64bit_global:
      /* Mask off the binding stride */
   addr = nir_iand_imm(b, addr, BITFIELD64_MASK(56));
   desc = nir_build_load_global(b, 1, 64, addr,
                     case nir_address_format_64bit_global_32bit_offset: {
      nir_def *base = nir_pack_64_2x32(b, nir_trim_vector(b, addr, 2));
   nir_def *offset = nir_channel(b, addr, 3);
   /* Mask off the binding stride */
   base = nir_iand_imm(b, base, BITFIELD64_MASK(56));
   desc = nir_load_global_constant_offset(b, 4, 32, base, offset,
                           case nir_address_format_64bit_bounded_global: {
      nir_def *base = nir_pack_64_2x32(b, nir_trim_vector(b, addr, 2));
   nir_def *size = nir_channel(b, addr, 2);
   nir_def *offset = nir_channel(b, addr, 3);
   /* Mask off the binding stride */
   base = nir_iand_imm(b, base, BITFIELD64_MASK(56));
   desc = nir_load_global_constant_bounded(b, 4, 32, base, offset, size,
                           default:
                              }
      static bool
   lower_ssbo_descriptor_instr(nir_builder *b, nir_instr *instr,
         {
               if (instr->type != nir_instr_type_intrinsic)
            nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(instr);
   switch (intrin->intrinsic) {
   case nir_intrinsic_vulkan_resource_index:
         case nir_intrinsic_vulkan_resource_reindex:
         case nir_intrinsic_load_vulkan_descriptor:
         default:
            }
      bool
   nvk_nir_lower_descriptors(nir_shader *nir,
               {
      struct lower_descriptors_ctx ctx = {
      .layout = layout,
   .clamp_desc_array_bounds =
      rs->storage_buffers != VK_PIPELINE_ROBUSTNESS_BUFFER_BEHAVIOR_DISABLED_EXT ||
   rs->uniform_buffers != VK_PIPELINE_ROBUSTNESS_BUFFER_BEHAVIOR_DISABLED_EXT ||
      .ssbo_addr_format = nvk_buffer_addr_format(rs->storage_buffers),
               /* We run in two passes.  The first attempts to lower everything it can.
   * In the variable pointers case, some SSBO intrinsics may fail to lower
   * but that's okay.  The second pass cleans up any SSBO intrinsics which
   * are left and lowers them to slightly less efficient but variable-
   * pointers-correct versions.
   */
   bool pass_lower_descriptors =
      nir_shader_instructions_pass(nir, try_lower_descriptors_instr,
                  bool pass_lower_ssbo =
      nir_shader_instructions_pass(nir, lower_ssbo_descriptor_instr,
                     }
