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
   #include "nir.h"
   #include "nir_builder.h"
   #include "lvp_lower_vulkan_resource.h"
      static bool
   lower_vulkan_resource_index(const nir_instr *instr, const void *data_cb)
   {
      if (instr->type == nir_instr_type_intrinsic) {
      nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(instr);
   switch (intrin->intrinsic) {
   case nir_intrinsic_vulkan_resource_index:
   case nir_intrinsic_vulkan_resource_reindex:
   case nir_intrinsic_load_vulkan_descriptor:
   case nir_intrinsic_get_ssbo_size:
   case nir_intrinsic_image_deref_sparse_load:
   case nir_intrinsic_image_deref_load:
   case nir_intrinsic_image_deref_store:
   case nir_intrinsic_image_deref_atomic:
   case nir_intrinsic_image_deref_atomic_swap:
   case nir_intrinsic_image_deref_size:
   case nir_intrinsic_image_deref_samples:
         default:
            }
   if (instr->type == nir_instr_type_tex) {
         }
      }
      static nir_def *lower_vri_intrin_vri(struct nir_builder *b,
         {
      nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(instr);
   unsigned desc_set_idx = nir_intrinsic_desc_set(intrin);
   unsigned binding_idx = nir_intrinsic_binding(intrin);
   const struct lvp_descriptor_set_binding_layout *binding =
            return nir_vec3(b, nir_imm_int(b, desc_set_idx + 1),
            }
      static nir_def *lower_vri_intrin_vrri(struct nir_builder *b,
         {
      nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(instr);
   nir_def *old_index = intrin->src[0].ssa;
   nir_def *delta = intrin->src[1].ssa;
   return nir_vec3(b, nir_channel(b, old_index, 0),
            }
      static nir_def *lower_vri_intrin_lvd(struct nir_builder *b,
         {
      nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(instr);
      }
      static nir_def *
   vulkan_resource_from_deref(nir_builder *b, nir_deref_instr *deref, const struct lvp_pipeline_layout *layout,
         {
               while (deref->deref_type != nir_deref_type_var) {
      assert(deref->deref_type == nir_deref_type_array);
                                          const struct lvp_descriptor_set_binding_layout *binding = get_binding_layout(layout, var->data.descriptor_set, var->data.binding);
   uint32_t binding_base = binding->descriptor_index + plane;
            return nir_vec3(b, nir_imm_int(b, var->data.descriptor_set + 1),
            }
      static void lower_vri_instr_tex(struct nir_builder *b,
         {
      struct lvp_pipeline_layout *layout = data_cb;
   nir_def *plane_ssa = nir_steal_tex_src(tex, nir_tex_src_plane);
   const uint32_t plane =
            for (unsigned i = 0; i < tex->num_srcs; i++) {
      nir_deref_instr *deref;
   switch (tex->src[i].src_type) {
   case nir_tex_src_texture_deref:
      tex->src[i].src_type = nir_tex_src_texture_handle;
   deref = nir_src_as_deref(tex->src[i].src);
      case nir_tex_src_sampler_deref:
      tex->src[i].src_type = nir_tex_src_sampler_handle;
   deref = nir_src_as_deref(tex->src[i].src);
      default:
                  nir_def *resource = vulkan_resource_from_deref(b, deref, layout, plane);
         }
      static void
   lower_image_intrinsic(nir_builder *b,
               {
                        nir_def *resource = vulkan_resource_from_deref(b, deref, layout, 0);
      }
      static bool
   lower_load_ubo(nir_builder *b, nir_intrinsic_instr *intrin, void *data_cb)
   {
      if (intrin->intrinsic != nir_intrinsic_load_ubo)
            nir_binding binding = nir_chase_binding(intrin->src[0]);
   /* If binding.success=false, then this is a variable pointer, which we don't support with
   * VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK.
   */
   if (!binding.success)
            const struct lvp_descriptor_set_binding_layout *bind_layout =
         if (bind_layout->type != VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK)
                              nir_def *offset = nir_iadd_imm(b, intrin->src[1].ssa, bind_layout->uniform_block_offset);
               }
      static nir_def *lower_vri_instr(struct nir_builder *b,
         {
      if (instr->type == nir_instr_type_intrinsic) {
      nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(instr);
   switch (intrin->intrinsic) {
   case nir_intrinsic_vulkan_resource_index:
            case nir_intrinsic_vulkan_resource_reindex:
            case nir_intrinsic_load_vulkan_descriptor:
            case nir_intrinsic_get_ssbo_size: {
      /* Ignore the offset component. */
   b->cursor = nir_before_instr(instr);
   nir_def *resource = intrin->src[0].ssa;
   nir_src_rewrite(&intrin->src[0], resource);
      }
   case nir_intrinsic_image_deref_sparse_load:
   case nir_intrinsic_image_deref_load:
   case nir_intrinsic_image_deref_store:
   case nir_intrinsic_image_deref_atomic:
   case nir_intrinsic_image_deref_atomic_swap:
   case nir_intrinsic_image_deref_size:
   case nir_intrinsic_image_deref_samples:
      b->cursor = nir_before_instr(instr);
               default:
                     if (instr->type == nir_instr_type_tex) {
      b->cursor = nir_before_instr(instr);
                  }
      void lvp_lower_pipeline_layout(const struct lvp_device *device,
               {
      nir_shader_intrinsics_pass(shader, lower_load_ubo,
                  }
