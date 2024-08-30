   /*
   * Copyright © 2020 Valve Corporation
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
   *
   */
   #include "ac_shader_util.h"
   #include "nir.h"
   #include "nir_builder.h"
   #include "radv_nir.h"
   #include "radv_private.h"
   #include "radv_shader.h"
   #include "radv_shader_args.h"
      typedef struct {
      enum amd_gfx_level gfx_level;
   uint32_t address32_hi;
   bool disable_aniso_single_level;
   bool has_image_load_dcc_bug;
            const struct radv_shader_args *args;
   const struct radv_shader_info *info;
      } apply_layout_state;
      static nir_def *
   get_scalar_arg(nir_builder *b, unsigned size, struct ac_arg arg)
   {
      assert(arg.used);
      }
      static nir_def *
   convert_pointer_to_64_bit(nir_builder *b, apply_layout_state *state, nir_def *ptr)
   {
         }
      static nir_def *
   load_desc_ptr(nir_builder *b, apply_layout_state *state, unsigned set)
   {
      const struct radv_userdata_locations *user_sgprs_locs = &state->info->user_sgprs_locs;
   if (user_sgprs_locs->shader_data[AC_UD_INDIRECT_DESCRIPTOR_SETS].sgpr_idx != -1) {
      nir_def *addr = get_scalar_arg(b, 1, state->args->descriptor_sets[0]);
   addr = convert_pointer_to_64_bit(b, state, addr);
               assert(state->args->descriptor_sets[set].used);
      }
      static void
   visit_vulkan_resource_index(nir_builder *b, apply_layout_state *state, nir_intrinsic_instr *intrin)
   {
      unsigned desc_set = nir_intrinsic_desc_set(intrin);
   unsigned binding = nir_intrinsic_binding(intrin);
   struct radv_descriptor_set_layout *layout = state->layout->set[desc_set].layout;
   unsigned offset = layout->binding[binding].offset;
            nir_def *set_ptr;
   if (layout->binding[binding].type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC ||
      layout->binding[binding].type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC) {
   unsigned idx = state->layout->set[desc_set].dynamic_offset_start + layout->binding[binding].dynamic_offset_offset;
   set_ptr = get_scalar_arg(b, 1, state->args->ac.push_constants);
   offset = state->layout->push_constant_size + idx * 16;
      } else {
      set_ptr = load_desc_ptr(b, state, desc_set);
               nir_def *binding_ptr = nir_imul_imm(b, intrin->src[0].ssa, stride);
            binding_ptr = nir_iadd_imm(b, binding_ptr, offset);
            if (layout->binding[binding].type == VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR) {
      assert(stride == 16);
      } else {
         }
      }
      static void
   visit_vulkan_resource_reindex(nir_builder *b, apply_layout_state *state, nir_intrinsic_instr *intrin)
   {
      VkDescriptorType desc_type = nir_intrinsic_desc_type(intrin);
   if (desc_type == VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR) {
      nir_def *set_ptr = nir_unpack_64_2x32_split_x(b, intrin->src[0].ssa);
            nir_def *index = nir_imul_imm(b, intrin->src[1].ssa, 16);
                        } else {
               nir_def *binding_ptr = nir_channel(b, intrin->src[0].ssa, 1);
            nir_def *index = nir_imul(b, intrin->src[1].ssa, stride);
                        }
      }
      static void
   visit_load_vulkan_descriptor(nir_builder *b, apply_layout_state *state, nir_intrinsic_instr *intrin)
   {
      if (nir_intrinsic_desc_type(intrin) == VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR) {
      nir_def *addr = convert_pointer_to_64_bit(b, state,
                           } else {
         }
      }
      static nir_def *
   load_inline_buffer_descriptor(nir_builder *b, apply_layout_state *state, nir_def *rsrc)
   {
      uint32_t desc_type = S_008F0C_DST_SEL_X(V_008F0C_SQ_SEL_X) | S_008F0C_DST_SEL_Y(V_008F0C_SQ_SEL_Y) |
         if (state->gfx_level >= GFX11) {
         } else if (state->gfx_level >= GFX10) {
      desc_type |= S_008F0C_FORMAT(V_008F0C_GFX10_FORMAT_32_FLOAT) | S_008F0C_OOB_SELECT(V_008F0C_OOB_SELECT_RAW) |
      } else {
      desc_type |=
               return nir_vec4(b, rsrc, nir_imm_int(b, S_008F04_BASE_ADDRESS_HI(state->address32_hi)), nir_imm_int(b, 0xffffffff),
      }
      static nir_def *
   load_buffer_descriptor(nir_builder *b, apply_layout_state *state, nir_def *rsrc, unsigned access)
   {
               /* If binding.success=false, then this is a variable pointer, which we don't support with
   * VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK.
   */
   if (binding.success) {
      struct radv_descriptor_set_layout *layout = state->layout->set[binding.desc_set].layout;
   if (layout->binding[binding.binding].type == VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK) {
      rsrc = nir_iadd(b, nir_channel(b, rsrc, 0), nir_channel(b, rsrc, 1));
                  if (access & ACCESS_NON_UNIFORM)
            nir_def *desc_set = convert_pointer_to_64_bit(b, state, nir_channel(b, rsrc, 0));
      }
      static void
   visit_get_ssbo_size(nir_builder *b, apply_layout_state *state, nir_intrinsic_instr *intrin)
   {
               nir_def *size;
   if (nir_intrinsic_access(intrin) & ACCESS_NON_UNIFORM) {
      nir_def *ptr = nir_iadd(b, nir_channel(b, rsrc, 0), nir_channel(b, rsrc, 1));
   ptr = nir_iadd_imm(b, ptr, 8);
   ptr = convert_pointer_to_64_bit(b, state, ptr);
   size = nir_build_load_global(b, 4, 32, ptr, .access = ACCESS_NON_WRITEABLE | ACCESS_CAN_REORDER, .align_mul = 16,
      } else {
      /* load the entire descriptor so it can be CSE'd */
   nir_def *ptr = convert_pointer_to_64_bit(b, state, nir_channel(b, rsrc, 0));
   nir_def *desc = nir_load_smem_amd(b, 4, ptr, nir_channel(b, rsrc, 1), .align_mul = 16);
               nir_def_rewrite_uses(&intrin->def, size);
      }
      static nir_def *
   get_sampler_desc(nir_builder *b, apply_layout_state *state, nir_deref_instr *deref, enum ac_descriptor_type desc_type,
         {
      nir_variable *var = nir_deref_instr_get_variable(deref);
   assert(var);
   unsigned desc_set = var->data.descriptor_set;
   unsigned binding_index = var->data.binding;
            struct radv_descriptor_set_layout *layout = state->layout->set[desc_set].layout;
            /* Handle immutable and embedded (compile-time) samplers
   * (VkDescriptorSetLayoutBinding::pImmutableSamplers) We can only do this for constant array
   * index or if all samplers in the array are the same. Note that indexing is forbidden with
   * embedded samplers.
   */
   if (desc_type == AC_DESC_SAMPLER && binding->immutable_samplers_offset &&
      (!indirect || binding->immutable_samplers_equal)) {
   unsigned constant_index = 0;
   if (!binding->immutable_samplers_equal) {
      while (deref->deref_type != nir_deref_type_var) {
      assert(deref->deref_type == nir_deref_type_array);
   unsigned array_size = MAX2(glsl_get_aoa_size(deref->type), 1);
   constant_index += nir_src_as_uint(deref->arr.index) * array_size;
                  uint32_t dword0_mask =
         const uint32_t *samplers = radv_immutable_samplers(layout, binding);
   return nir_imm_ivec4(b, samplers[constant_index * 4 + 0] & dword0_mask, samplers[constant_index * 4 + 1],
               unsigned size = 8;
   unsigned offset = binding->offset;
   switch (desc_type) {
   case AC_DESC_IMAGE:
   case AC_DESC_PLANE_0:
         case AC_DESC_FMASK:
   case AC_DESC_PLANE_1:
      offset += 32;
      case AC_DESC_SAMPLER:
      size = 4;
   if (binding->type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
            case AC_DESC_BUFFER:
      size = 4;
      case AC_DESC_PLANE_2:
      size = 4;
   offset += 64;
               nir_def *index = NULL;
   while (deref->deref_type != nir_deref_type_var) {
      assert(deref->deref_type == nir_deref_type_array);
   unsigned array_size = MAX2(glsl_get_aoa_size(deref->type), 1);
            nir_def *tmp = nir_imul_imm(b, deref->arr.index.ssa, array_size);
   if (tmp != deref->arr.index.ssa)
            if (index) {
      index = nir_iadd(b, tmp, index);
      } else {
                              nir_def *index_offset = index ? nir_iadd_imm(b, index, offset) : nir_imm_int(b, offset);
   if (index && index_offset != index)
            if (non_uniform)
            nir_def *addr = convert_pointer_to_64_bit(b, state, load_desc_ptr(b, state, desc_set));
            /* 3 plane formats always have same size and format for plane 1 & 2, so
   * use the tail from plane 1 so that we can store only the first 16 bytes
   * of the last plane. */
   if (desc_type == AC_DESC_PLANE_2) {
               nir_def *comp[8];
   for (unsigned i = 0; i < 4; i++)
         for (unsigned i = 4; i < 8; i++)
               } else if (desc_type == AC_DESC_IMAGE && state->has_image_load_dcc_bug && !tex && !write) {
      nir_def *comp[8];
   for (unsigned i = 0; i < 8; i++)
            /* WRITE_COMPRESS_ENABLE must be 0 for all image loads to workaround a
   * hardware bug.
   */
               } else if (desc_type == AC_DESC_SAMPLER && tex->op == nir_texop_tg4 && state->disable_tg4_trunc_coord) {
      nir_def *comp[4];
   for (unsigned i = 0; i < 4; i++)
            /* We want to always use the linear filtering truncation behaviour for
   * nir_texop_tg4, even if the sampler uses nearest/point filtering.
   */
                           }
      static void
   update_image_intrinsic(nir_builder *b, apply_layout_state *state, nir_intrinsic_instr *intrin)
   {
      nir_deref_instr *deref = nir_src_as_deref(intrin->src[0]);
   const enum glsl_sampler_dim dim = glsl_get_sampler_dim(deref->type);
   bool is_load =
            nir_def *desc = get_sampler_desc(b, state, deref, dim == GLSL_SAMPLER_DIM_BUF ? AC_DESC_BUFFER : AC_DESC_IMAGE,
            if (intrin->intrinsic == nir_intrinsic_image_deref_descriptor_amd) {
      nir_def_rewrite_uses(&intrin->def, desc);
      } else {
            }
      static void
   apply_layout_to_intrin(nir_builder *b, apply_layout_state *state, nir_intrinsic_instr *intrin)
   {
               nir_def *rsrc;
   switch (intrin->intrinsic) {
   case nir_intrinsic_vulkan_resource_index:
      visit_vulkan_resource_index(b, state, intrin);
      case nir_intrinsic_vulkan_resource_reindex:
      visit_vulkan_resource_reindex(b, state, intrin);
      case nir_intrinsic_load_vulkan_descriptor:
      visit_load_vulkan_descriptor(b, state, intrin);
      case nir_intrinsic_load_ubo:
   case nir_intrinsic_load_ssbo:
   case nir_intrinsic_ssbo_atomic:
   case nir_intrinsic_ssbo_atomic_swap:
      rsrc = load_buffer_descriptor(b, state, intrin->src[0].ssa, nir_intrinsic_access(intrin));
   nir_src_rewrite(&intrin->src[0], rsrc);
      case nir_intrinsic_store_ssbo:
      rsrc = load_buffer_descriptor(b, state, intrin->src[1].ssa, nir_intrinsic_access(intrin));
   nir_src_rewrite(&intrin->src[1], rsrc);
      case nir_intrinsic_get_ssbo_size:
      visit_get_ssbo_size(b, state, intrin);
      case nir_intrinsic_image_deref_load:
   case nir_intrinsic_image_deref_sparse_load:
   case nir_intrinsic_image_deref_store:
   case nir_intrinsic_image_deref_atomic:
   case nir_intrinsic_image_deref_atomic_swap:
   case nir_intrinsic_image_deref_size:
   case nir_intrinsic_image_deref_samples:
   case nir_intrinsic_image_deref_descriptor_amd:
      update_image_intrinsic(b, state, intrin);
      default:
            }
      static void
   apply_layout_to_tex(nir_builder *b, apply_layout_state *state, nir_tex_instr *tex)
   {
               nir_deref_instr *texture_deref_instr = NULL;
   nir_deref_instr *sampler_deref_instr = NULL;
            for (unsigned i = 0; i < tex->num_srcs; i++) {
      switch (tex->src[i].src_type) {
   case nir_tex_src_texture_deref:
      texture_deref_instr = nir_src_as_deref(tex->src[i].src);
      case nir_tex_src_sampler_deref:
      sampler_deref_instr = nir_src_as_deref(tex->src[i].src);
      case nir_tex_src_plane:
      plane = nir_src_as_int(tex->src[i].src);
      default:
                     nir_def *image = NULL;
   nir_def *sampler = NULL;
   if (plane >= 0) {
      assert(tex->op != nir_texop_txf_ms && tex->op != nir_texop_samples_identical);
   assert(tex->sampler_dim != GLSL_SAMPLER_DIM_BUF);
   image =
      } else if (tex->sampler_dim == GLSL_SAMPLER_DIM_BUF) {
         } else if (tex->op == nir_texop_fragment_mask_fetch_amd) {
         } else {
                  if (sampler_deref_instr) {
               if (state->disable_aniso_single_level && tex->sampler_dim < GLSL_SAMPLER_DIM_RECT && state->gfx_level < GFX8) {
      /* Disable anisotropic filtering if BASE_LEVEL == LAST_LEVEL.
   *
   * GFX6-GFX7:
   *   If BASE_LEVEL == LAST_LEVEL, the shader must disable anisotropic
   *   filtering manually. The driver sets img7 to a mask clearing
   *   MAX_ANISO_RATIO if BASE_LEVEL == LAST_LEVEL. The shader must do:
   *     s_and_b32 samp0, samp0, img7
   *
   * GFX8:
   *   The ANISO_OVERRIDE sampler field enables this fix in TA.
   */
   /* TODO: This is unnecessary for combined image+sampler.
   * We can do this when updating the desc set. */
   nir_def *comp[4];
   for (unsigned i = 0; i < 4; i++)
                                 if (tex->op == nir_texop_descriptor_amd) {
      nir_def_rewrite_uses(&tex->def, image);
   nir_instr_remove(&tex->instr);
               for (unsigned i = 0; i < tex->num_srcs; i++) {
      switch (tex->src[i].src_type) {
   case nir_tex_src_texture_deref:
      tex->src[i].src_type = nir_tex_src_texture_handle;
   nir_src_rewrite(&tex->src[i].src, image);
      case nir_tex_src_sampler_deref:
      tex->src[i].src_type = nir_tex_src_sampler_handle;
   nir_src_rewrite(&tex->src[i].src, sampler);
      default:
               }
      void
   radv_nir_apply_pipeline_layout(nir_shader *shader, struct radv_device *device, const struct radv_shader_info *info,
         {
      apply_layout_state state = {
      .gfx_level = device->physical_device->rad_info.gfx_level,
   .address32_hi = device->physical_device->rad_info.address32_hi,
   .disable_aniso_single_level = device->instance->disable_aniso_single_level,
   .has_image_load_dcc_bug = device->physical_device->rad_info.has_image_load_dcc_bug,
   .disable_tg4_trunc_coord =
         .args = args,
   .info = info,
                        nir_foreach_function (function, shader) {
      if (!function->impl)
                     /* Iterate in reverse so load_ubo lowering can look at
   * the vulkan_resource_index to tell if it's an inline
   * ubo.
   */
   nir_foreach_block_reverse (block, function->impl) {
      nir_foreach_instr_reverse_safe (instr, block) {
      if (instr->type == nir_instr_type_tex)
         else if (instr->type == nir_instr_type_intrinsic)
                        }
