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
      #include "panvk_private.h"
      #include "nir.h"
   #include "nir_builder.h"
      struct apply_descriptors_ctx {
      const struct panvk_pipeline_layout *layout;
   bool add_bounds_checks;
   bool has_img_access;
   nir_address_format desc_addr_format;
   nir_address_format ubo_addr_format;
      };
      static nir_address_format
   addr_format_for_desc_type(VkDescriptorType desc_type,
         {
      switch (desc_type) {
   case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
   case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
            case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
            default:
            }
      static const struct panvk_descriptor_set_layout *
   get_set_layout(uint32_t set, const struct apply_descriptors_ctx *ctx)
   {
         }
      static const struct panvk_descriptor_set_binding_layout *
   get_binding_layout(uint32_t set, uint32_t binding,
         {
         }
      /** Build a Vulkan resource index
   *
   * A "resource index" is the term used by our SPIR-V parser and the relevant
   * NIR intrinsics for a reference into a descriptor set.  It acts much like a
   * deref in NIR except that it accesses opaque descriptors instead of memory.
   *
   * Coming out of SPIR-V, both the resource indices (in the form of
   * vulkan_resource_[re]index intrinsics) and the memory derefs (in the form
   * of nir_deref_instr) use the same vector component/bit size.  The meaning
   * of those values for memory derefs (nir_deref_instr) is given by the
   * nir_address_format associated with the descriptor type.  For resource
   * indices, it's an entirely internal to panvk encoding which describes, in
   * some sense, the address of the descriptor.  Thanks to the NIR/SPIR-V rules,
   * it must be packed into the same size SSA values as a memory address.  For
   * this reason, the actual encoding may depend both on the address format for
   * memory derefs and the descriptor address format.
   *
   * The load_vulkan_descriptor intrinsic exists to provide a transition point
   * between these two forms of derefs: descriptor and memory.
   */
   static nir_def *
   build_res_index(nir_builder *b, uint32_t set, uint32_t binding,
               {
      const struct panvk_descriptor_set_layout *set_layout =
         const struct panvk_descriptor_set_binding_layout *bind_layout =
                     switch (bind_layout->type) {
   case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
   case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC: {
               const unsigned ubo_idx =
                                 case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER: {
      assert(addr_format == nir_address_format_64bit_bounded_global ||
            const unsigned set_ubo_idx =
                  const uint32_t packed =
            return nir_vec4(b, nir_imm_int(b, packed),
                     case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC: {
      assert(addr_format == nir_address_format_64bit_bounded_global ||
            const unsigned dyn_ssbo_idx =
            const unsigned ubo_idx = PANVK_SYSVAL_UBO_INDEX;
   const unsigned desc_stride = sizeof(struct panvk_ssbo_addr);
   const uint32_t ubo_offset =
                     return nir_vec4(b, nir_imm_int(b, packed), nir_imm_int(b, ubo_offset),
               default:
            }
      /** Adjust a Vulkan resource index
   *
   * This is the equivalent of nir_deref_type_ptr_as_array for resource indices.
   * For array descriptors, it allows us to adjust the array index.  Thanks to
   * variable pointers, we cannot always fold this re-index operation into the
   * vulkan_resource_index intrinsic and we have to do it based on nothing but
   * the address format.
   */
   static nir_def *
   build_res_reindex(nir_builder *b, nir_def *orig, nir_def *delta,
         {
      switch (addr_format) {
   case nir_address_format_32bit_index_offset:
      return nir_vec2(b, nir_channel(b, orig, 0),
         case nir_address_format_64bit_bounded_global:
   case nir_address_format_64bit_global_32bit_offset:
      return nir_vec4(b, nir_channel(b, orig, 0), nir_channel(b, orig, 1),
               default:
            }
      /** Convert a Vulkan resource index into a buffer address
   *
   * In some cases, this does a  memory load from the descriptor set and, in
   * others, it simply converts from one form to another.
   *
   * See build_res_index for details about each resource index format.
   */
   static nir_def *
   build_buffer_addr_for_res_index(nir_builder *b, nir_def *res_index,
               {
      switch (addr_format) {
   case nir_address_format_32bit_index_offset: {
      nir_def *packed = nir_channel(b, res_index, 0);
   nir_def *array_index = nir_channel(b, res_index, 1);
   nir_def *surface_index = nir_extract_u16(b, packed, nir_imm_int(b, 0));
            if (ctx->add_bounds_checks)
            return nir_vec2(b, nir_iadd(b, surface_index, array_index),
               case nir_address_format_64bit_bounded_global:
   case nir_address_format_64bit_global_32bit_offset: {
      nir_def *packed = nir_channel(b, res_index, 0);
   nir_def *desc_ubo_offset = nir_channel(b, res_index, 1);
   nir_def *array_max = nir_channel(b, res_index, 2);
            nir_def *desc_ubo_idx = nir_extract_u16(b, packed, nir_imm_int(b, 0));
            if (ctx->add_bounds_checks)
            desc_ubo_offset = nir_iadd(b, desc_ubo_offset,
            nir_def *desc = nir_load_ubo(b, 4, 32, desc_ubo_idx, desc_ubo_offset,
            /* The offset in the descriptor is guaranteed to be zero when it's
   * written into the descriptor set.  This lets us avoid some unnecessary
   * adds.
   */
   return nir_vec4(b, nir_channel(b, desc, 0), nir_channel(b, desc, 1),
               default:
            }
      static bool
   lower_res_intrinsic(nir_builder *b, nir_intrinsic_instr *intrin,
         {
               const VkDescriptorType desc_type = nir_intrinsic_desc_type(intrin);
            nir_def *res;
   switch (intrin->intrinsic) {
   case nir_intrinsic_vulkan_resource_index:
      res = build_res_index(b, nir_intrinsic_desc_set(intrin),
                     case nir_intrinsic_vulkan_resource_reindex:
      res = build_res_reindex(b, intrin->src[0].ssa, intrin->src[1].ssa,
               case nir_intrinsic_load_vulkan_descriptor:
      res = build_buffer_addr_for_res_index(b, intrin->src[0].ssa, addr_format,
               default:
                  assert(intrin->def.bit_size == res->bit_size);
   assert(intrin->def.num_components == res->num_components);
   nir_def_rewrite_uses(&intrin->def, res);
               }
      static void
   get_resource_deref_binding(nir_deref_instr *deref, uint32_t *set,
               {
      *index_imm = 0;
            if (deref->deref_type == nir_deref_type_array) {
      if (index_imm != NULL && nir_src_is_const(deref->arr.index))
         else
                        assert(deref->deref_type == nir_deref_type_var);
            *set = var->data.descriptor_set;
      }
      static nir_def *
   load_resource_deref_desc(nir_builder *b, nir_deref_instr *deref,
                     {
      uint32_t set, binding, index_imm;
   nir_def *index_ssa;
            const struct panvk_descriptor_set_layout *set_layout =
         const struct panvk_descriptor_set_binding_layout *bind_layout =
            assert(index_ssa == NULL || index_imm == 0);
   if (index_ssa == NULL)
            const unsigned set_ubo_idx =
      panvk_pipeline_layout_ubo_start(ctx->layout, set, false) +
         nir_def *desc_ubo_offset =
      nir_iadd_imm(b, nir_imul_imm(b, index_ssa, bind_layout->desc_ubo_stride),
         assert(bind_layout->desc_ubo_stride > 0);
   unsigned desc_align = (1 << (ffs(bind_layout->desc_ubo_stride) - 1));
            return nir_load_ubo(b, num_components, bit_size, nir_imm_int(b, set_ubo_idx),
            }
      static nir_def *
   load_tex_img_size(nir_builder *b, nir_deref_instr *deref,
               {
      if (dim == GLSL_SAMPLER_DIM_BUF) {
         } else {
               /* The sizes are provided as 16-bit values with 1 subtracted so
   * convert to 32-bit and add 1.
   */
         }
      static nir_def *
   load_tex_img_levels(nir_builder *b, nir_deref_instr *deref,
               {
      assert(dim != GLSL_SAMPLER_DIM_BUF);
   nir_def *desc = load_resource_deref_desc(b, deref, 0, 4, 16, ctx);
      }
      static nir_def *
   load_tex_img_samples(nir_builder *b, nir_deref_instr *deref,
               {
      assert(dim != GLSL_SAMPLER_DIM_BUF);
   nir_def *desc = load_resource_deref_desc(b, deref, 0, 4, 16, ctx);
      }
      static bool
   lower_tex(nir_builder *b, nir_tex_instr *tex,
         {
                        if (tex->op == nir_texop_txs || tex->op == nir_texop_query_levels ||
      tex->op == nir_texop_texture_samples) {
   int tex_src_idx = nir_tex_instr_src_index(tex, nir_tex_src_texture_deref);
   assert(tex_src_idx >= 0);
                     nir_def *res;
   switch (tex->op) {
   case nir_texop_txs:
      res = nir_channels(b, load_tex_img_size(b, deref, dim, ctx),
            case nir_texop_query_levels:
      assert(tex->def.num_components == 1);
   res = load_tex_img_levels(b, deref, dim, ctx);
      case nir_texop_texture_samples:
      assert(tex->def.num_components == 1);
   res = load_tex_img_samples(b, deref, dim, ctx);
      default:
                  nir_def_rewrite_uses(&tex->def, res);
   nir_instr_remove(&tex->instr);
               int sampler_src_idx =
         if (sampler_src_idx >= 0) {
      nir_deref_instr *deref = nir_src_as_deref(tex->src[sampler_src_idx].src);
            uint32_t set, binding, index_imm;
   nir_def *index_ssa;
            const struct panvk_descriptor_set_binding_layout *bind_layout =
            tex->sampler_index = ctx->layout->sets[set].sampler_offset +
            if (index_ssa != NULL) {
         }
               int tex_src_idx = nir_tex_instr_src_index(tex, nir_tex_src_texture_deref);
   if (tex_src_idx >= 0) {
      nir_deref_instr *deref = nir_src_as_deref(tex->src[tex_src_idx].src);
            uint32_t set, binding, index_imm;
   nir_def *index_ssa;
            const struct panvk_descriptor_set_binding_layout *bind_layout =
            tex->texture_index =
            if (index_ssa != NULL) {
         }
                  }
      static nir_def *
   get_img_index(nir_builder *b, nir_deref_instr *deref,
         {
      uint32_t set, binding, index_imm;
   nir_def *index_ssa;
            const struct panvk_descriptor_set_binding_layout *bind_layout =
         assert(bind_layout->type == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE ||
                  unsigned img_offset =
            if (index_ssa == NULL) {
         } else {
      assert(index_imm == 0);
         }
      static bool
   lower_img_intrinsic(nir_builder *b, nir_intrinsic_instr *intr,
         {
      b->cursor = nir_before_instr(&intr->instr);
            if (intr->intrinsic == nir_intrinsic_image_deref_size ||
      intr->intrinsic == nir_intrinsic_image_deref_samples) {
            nir_def *res;
   switch (intr->intrinsic) {
   case nir_intrinsic_image_deref_size:
      res = nir_channels(b, load_tex_img_size(b, deref, dim, ctx),
            case nir_intrinsic_image_deref_samples:
      res = load_tex_img_samples(b, deref, dim, ctx);
      default:
                  nir_def_rewrite_uses(&intr->def, res);
      } else {
      nir_rewrite_image_intrinsic(intr, get_img_index(b, deref, ctx), false);
                  }
      static bool
   lower_intrinsic(nir_builder *b, nir_intrinsic_instr *intr,
         {
      switch (intr->intrinsic) {
   case nir_intrinsic_vulkan_resource_index:
   case nir_intrinsic_vulkan_resource_reindex:
   case nir_intrinsic_load_vulkan_descriptor:
         case nir_intrinsic_image_deref_store:
   case nir_intrinsic_image_deref_load:
   case nir_intrinsic_image_deref_atomic:
   case nir_intrinsic_image_deref_atomic_swap:
   case nir_intrinsic_image_deref_size:
   case nir_intrinsic_image_deref_samples:
         default:
            }
      static bool
   lower_descriptors_instr(nir_builder *b, nir_instr *instr, void *data)
   {
               switch (instr->type) {
   case nir_instr_type_tex:
         case nir_instr_type_intrinsic:
         default:
            }
      bool
   panvk_per_arch(nir_lower_descriptors)(nir_shader *nir, struct panvk_device *dev,
               {
      struct apply_descriptors_ctx ctx = {
      .layout = layout,
   .desc_addr_format = nir_address_format_32bit_index_offset,
   .ubo_addr_format = nir_address_format_32bit_index_offset,
   .ssbo_addr_format = dev->vk.enabled_features.robustBufferAccess
                     bool progress = nir_shader_instructions_pass(
      nir, lower_descriptors_instr,
      if (has_img_access_out)
               }
