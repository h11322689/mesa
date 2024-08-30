   /*
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
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   * IN THE SOFTWARE.
   */
      #include "anv_nir.h"
   #include "nir/nir_builder.h"
   #include "compiler/brw_nir.h"
   #include "util/mesa-sha1.h"
   #include "util/set.h"
      /* Sampler tables don't actually have a maximum size but we pick one just so
   * that we don't end up emitting too much state on-the-fly.
   */
   #define MAX_SAMPLER_TABLE_SIZE 128
   #define BINDLESS_OFFSET        255
      #define sizeof_field(type, field) sizeof(((type *)0)->field)
      struct apply_pipeline_layout_state {
               const struct anv_pipeline_layout *layout;
   nir_address_format ssbo_addr_format;
            /* Place to flag lowered instructions so we don't lower them twice */
            bool uses_constants;
   bool has_dynamic_buffers;
   uint8_t constants_offset;
   struct {
      bool desc_buffer_used;
            uint8_t *use_count;
   uint8_t *surface_offsets;
         };
      static nir_address_format
   addr_format_for_desc_type(VkDescriptorType desc_type,
         {
      switch (desc_type) {
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
            case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
   case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
            case VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK:
            default:
            }
      static void
   add_binding(struct apply_pipeline_layout_state *state,
         {
      const struct anv_descriptor_set_binding_layout *bind_layout =
            if (state->set[set].use_count[binding] < UINT8_MAX)
            /* Only flag the descriptor buffer as used if there's actually data for
   * this binding.  This lets us be lazy and call this function constantly
   * without worrying about unnecessarily enabling the buffer.
   */
   if (bind_layout->descriptor_stride)
      }
      static void
   add_deref_src_binding(struct apply_pipeline_layout_state *state, nir_src src)
   {
      nir_deref_instr *deref = nir_src_as_deref(src);
   nir_variable *var = nir_deref_instr_get_variable(deref);
      }
      static void
   add_tex_src_binding(struct apply_pipeline_layout_state *state,
         {
      int deref_src_idx = nir_tex_instr_src_index(tex, deref_src_type);
   if (deref_src_idx < 0)
               }
      static bool
   get_used_bindings(UNUSED nir_builder *_b, nir_instr *instr, void *_state)
   {
               switch (instr->type) {
   case nir_instr_type_intrinsic: {
      nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(instr);
   switch (intrin->intrinsic) {
   case nir_intrinsic_vulkan_resource_index:
      add_binding(state, nir_intrinsic_desc_set(intrin),
               case nir_intrinsic_image_deref_load:
   case nir_intrinsic_image_deref_store:
   case nir_intrinsic_image_deref_atomic:
   case nir_intrinsic_image_deref_atomic_swap:
   case nir_intrinsic_image_deref_size:
   case nir_intrinsic_image_deref_samples:
   case nir_intrinsic_image_deref_load_param_intel:
   case nir_intrinsic_image_deref_load_raw_intel:
   case nir_intrinsic_image_deref_store_raw_intel:
                  case nir_intrinsic_load_constant:
                  default:
         }
      }
   case nir_instr_type_tex: {
      nir_tex_instr *tex = nir_instr_as_tex(instr);
   add_tex_src_binding(state, tex, nir_tex_src_texture_deref);
   add_tex_src_binding(state, tex, nir_tex_src_sampler_deref);
      }
   default:
                     }
      static nir_intrinsic_instr *
   find_descriptor_for_index_src(nir_src src,
         {
               while (intrin && intrin->intrinsic == nir_intrinsic_vulkan_resource_reindex)
            if (!intrin || intrin->intrinsic != nir_intrinsic_vulkan_resource_index)
               }
      static bool
   descriptor_has_bti(nir_intrinsic_instr *intrin,
         {
               uint32_t set = nir_intrinsic_desc_set(intrin);
   uint32_t binding = nir_intrinsic_binding(intrin);
   const struct anv_descriptor_set_binding_layout *bind_layout =
            uint32_t surface_index;
   if (bind_layout->data & ANV_DESCRIPTOR_INLINE_UNIFORM)
         else
            /* Only lower to a BTI message if we have a valid binding table index. */
      }
      static nir_address_format
   descriptor_address_format(nir_intrinsic_instr *intrin,
         {
                  }
      static nir_intrinsic_instr *
   nir_deref_find_descriptor(nir_deref_instr *deref,
         {
      while (1) {
      /* Nothing we will use this on has a variable */
            nir_deref_instr *parent = nir_src_as_deref(deref->parent);
   if (!parent)
               }
            nir_intrinsic_instr *intrin = nir_src_as_intrinsic(deref->parent);
   if (!intrin || intrin->intrinsic != nir_intrinsic_load_vulkan_descriptor)
               }
      static nir_def *
   build_load_descriptor_mem(nir_builder *b,
                        {
      nir_def *surface_index = nir_channel(b, desc_addr, 0);
   nir_def *offset32 =
            return nir_load_ubo(b, num_components, bit_size,
                     surface_index, offset32,
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
   * indices, it's an entirely internal to ANV encoding which describes, in some
   * sense, the address of the descriptor.  Thanks to the NIR/SPIR-V rules, it
   * must be packed into the same size SSA values as a memory address.  For this
   * reason, the actual encoding may depend both on the address format for
   * memory derefs and the descriptor address format.
   *
   * The load_vulkan_descriptor intrinsic exists to provide a transition point
   * between these two forms of derefs: descriptor and memory.
   */
   static nir_def *
   build_res_index(nir_builder *b, uint32_t set, uint32_t binding,
               {
      const struct anv_descriptor_set_binding_layout *bind_layout =
                     switch (addr_format) {
   case nir_address_format_64bit_global_32bit_offset:
   case nir_address_format_64bit_bounded_global: {
      assert(state->set[set].desc_offset < MAX_BINDING_TABLE_SIZE);
            assert(bind_layout->dynamic_offset_index < MAX_DYNAMIC_BUFFERS);
   uint32_t dynamic_offset_index = 0xff; /* No dynamic offset */
   if (bind_layout->dynamic_offset_index >= 0) {
      dynamic_offset_index =
      state->layout->set[set].dynamic_offset_start +
                     return nir_vec4(b, nir_imm_int(b, packed),
                           case nir_address_format_32bit_index_offset: {
      if (bind_layout->type == VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK) {
      uint32_t surface_index = state->set[set].desc_offset;
   return nir_imm_ivec2(b, surface_index,
      } else {
      uint32_t surface_index = state->set[set].surface_offsets[binding];
   assert(array_size > 0 && array_size <= UINT16_MAX);
   assert(surface_index <= UINT16_MAX);
   uint32_t packed = ((array_size - 1) << 16) | surface_index;
                  default:
            }
      struct res_index_defs {
      nir_def *set_idx;
   nir_def *dyn_offset_base;
   nir_def *desc_offset_base;
   nir_def *array_index;
      };
      static struct res_index_defs
   unpack_res_index(nir_builder *b, nir_def *index)
   {
               nir_def *packed = nir_channel(b, index, 0);
   defs.desc_stride = nir_extract_u8(b, packed, nir_imm_int(b, 2));
   defs.set_idx = nir_extract_u8(b, packed, nir_imm_int(b, 1));
            defs.desc_offset_base = nir_channel(b, index, 1);
   defs.array_index = nir_umin(b, nir_channel(b, index, 2),
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
   case nir_address_format_64bit_global_32bit_offset:
   case nir_address_format_64bit_bounded_global:
      return nir_vec4(b, nir_channel(b, orig, 0),
                     case nir_address_format_32bit_index_offset:
      return nir_vec2(b, nir_iadd(b, nir_channel(b, orig, 0), delta),
         default:
            }
      /** Get the address for a descriptor given its resource index
   *
   * Because of the re-indexing operations, we can't bounds check descriptor
   * array access until we have the final index.  That means we end up doing the
   * bounds check here, if needed.  See unpack_res_index() for more details.
   *
   * This function takes both a bind_layout and a desc_type which are used to
   * determine the descriptor stride for array descriptors.  The bind_layout is
   * optional for buffer descriptor types.
   */
   static nir_def *
   build_desc_addr(nir_builder *b,
                  const struct anv_descriptor_set_binding_layout *bind_layout,
      {
      switch (addr_format) {
   case nir_address_format_64bit_global_32bit_offset:
   case nir_address_format_64bit_bounded_global: {
               nir_def *desc_offset = res.desc_offset_base;
   if (desc_type != VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK) {
      /* Compute the actual descriptor offset.  For inline uniform blocks,
   * the array index is ignored as they are only allowed to be a single
   * descriptor (not an array) and there is no concept of a "stride".
   *
   */
   desc_offset =
                           case nir_address_format_32bit_index_offset:
      assert(desc_type == VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK);
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
   build_buffer_addr_for_res_index(nir_builder *b,
                           {
      if (desc_type == VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK) {
      assert(addr_format == nir_address_format_32bit_index_offset);
      } else if (addr_format == nir_address_format_32bit_index_offset) {
      nir_def *array_index = nir_channel(b, res_index, 0);
   nir_def *packed = nir_channel(b, res_index, 1);
            return nir_vec2(b, nir_iadd(b, surface_index, array_index),
               nir_def *desc_addr =
                     if (state->has_dynamic_buffers) {
               /* This shader has dynamic offsets and we have no way of knowing
   * (save from the dynamic offset base index) if this buffer has a
   * dynamic offset.
   */
   nir_def *dyn_offset_idx =
            nir_def *dyn_load =
      nir_load_push_constant(b, 1, 32, nir_imul_imm(b, dyn_offset_idx, 4),
               nir_def *dynamic_offset =
                  /* The dynamic offset gets added to the base pointer so that we
   * have a sliding window range.
   */
   nir_def *base_ptr =
         base_ptr = nir_iadd(b, base_ptr, nir_u2u64(b, dynamic_offset));
   desc = nir_vec4(b, nir_unpack_64_2x32_split_x(b, base_ptr),
                           /* The last element of the vec4 is always zero.
   *
   * See also struct anv_address_range_descriptor
   */
   return nir_vec4(b, nir_channel(b, desc, 0),
                  }
      /** Loads descriptor memory for a variable-based deref chain
   *
   * The deref chain has to terminate at a variable with a descriptor_set and
   * binding set.  This is used for images, textures, and samplers.
   */
   static nir_def *
   build_load_var_deref_descriptor_mem(nir_builder *b, nir_deref_instr *deref,
                     {
               const uint32_t set = var->data.descriptor_set;
   const uint32_t binding = var->data.binding;
   const struct anv_descriptor_set_binding_layout *bind_layout =
            nir_def *array_index;
   if (deref->deref_type != nir_deref_type_var) {
      assert(deref->deref_type == nir_deref_type_array);
   assert(nir_deref_instr_parent(deref)->deref_type == nir_deref_type_var);
      } else {
                  /* It doesn't really matter what address format we choose as everything
   * will constant-fold nicely.  Choose one that uses the actual descriptor
   * buffer so we don't run into issues index/offset assumptions.
   */
   const nir_address_format addr_format =
            nir_def *res_index =
            nir_def *desc_addr =
      build_desc_addr(b, bind_layout, bind_layout->type,
         return build_load_descriptor_mem(b, desc_addr, desc_offset,
      }
      /** A recursive form of build_res_index()
   *
   * This recursively walks a resource [re]index chain and builds the resource
   * index.  It places the new code with the resource [re]index operation in the
   * hopes of better CSE.  This means the cursor is not where you left it when
   * this function returns.
   */
   static nir_def *
   build_res_index_for_chain(nir_builder *b, nir_intrinsic_instr *intrin,
                     {
      if (intrin->intrinsic == nir_intrinsic_vulkan_resource_index) {
      b->cursor = nir_before_instr(&intrin->instr);
   *set = nir_intrinsic_desc_set(intrin);
   *binding = nir_intrinsic_binding(intrin);
   return build_res_index(b, *set, *binding, intrin->src[0].ssa,
      } else {
      assert(intrin->intrinsic == nir_intrinsic_vulkan_resource_reindex);
   nir_intrinsic_instr *parent = nir_src_as_intrinsic(intrin->src[0]);
   nir_def *index =
                                 }
      /** Builds a buffer address for a given vulkan [re]index intrinsic
   *
   * The cursor is not where you left it when this function returns.
   */
   static nir_def *
   build_buffer_addr_for_idx_intrin(nir_builder *b,
                     {
      uint32_t set = UINT32_MAX, binding = UINT32_MAX;
   nir_def *res_index =
      build_res_index_for_chain(b, idx_intrin, addr_format,
         const struct anv_descriptor_set_binding_layout *bind_layout =
            return build_buffer_addr_for_res_index(b, bind_layout->type,
      }
      /** Builds a buffer address for deref chain
   *
   * This assumes that you can chase the chain all the way back to the original
   * vulkan_resource_index intrinsic.
   *
   * The cursor is not where you left it when this function returns.
   */
   static nir_def *
   build_buffer_addr_for_deref(nir_builder *b, nir_deref_instr *deref,
               {
      nir_deref_instr *parent = nir_deref_instr_parent(deref);
   if (parent) {
      nir_def *addr =
            b->cursor = nir_before_instr(&deref->instr);
               nir_intrinsic_instr *load_desc = nir_src_as_intrinsic(deref->parent);
                                 }
      static bool
   try_lower_direct_buffer_intrinsic(nir_builder *b,
               {
      nir_deref_instr *deref = nir_src_as_deref(intrin->src[0]);
   if (!nir_deref_mode_is_one_of(deref, nir_var_mem_ubo | nir_var_mem_ssbo))
            nir_intrinsic_instr *desc = nir_deref_find_descriptor(deref, state);
   if (desc == NULL) {
      /* We should always be able to find the descriptor for UBO access. */
   assert(nir_deref_mode_is_one_of(deref, nir_var_mem_ssbo));
                        if (nir_deref_mode_is(deref, nir_var_mem_ssbo)) {
      /* Normal binding table-based messages can't handle non-uniform access
   * so we have to fall back to A64.
   */
   if (nir_intrinsic_access(intrin) & ACCESS_NON_UNIFORM)
            if (!descriptor_has_bti(desc, state))
            /* Rewrite to 32bit_index_offset whenever we can */
      } else {
               /* Rewrite to 32bit_index_offset whenever we can */
   if (descriptor_has_bti(desc, state))
               nir_def *addr =
            b->cursor = nir_before_instr(&intrin->instr);
               }
      static bool
   lower_load_accel_struct_desc(nir_builder *b,
               {
                        /* It doesn't really matter what address format we choose as
   * everything will constant-fold nicely.  Choose one that uses the
   * actual descriptor buffer.
   */
   const nir_address_format addr_format =
            uint32_t set = UINT32_MAX, binding = UINT32_MAX;
   nir_def *res_index =
      build_res_index_for_chain(b, idx_intrin, addr_format,
         const struct anv_descriptor_set_binding_layout *bind_layout =
                     nir_def *desc_addr =
      build_desc_addr(b, bind_layout, bind_layout->type,
         /* Acceleration structure descriptors are always uint64_t */
            assert(load_desc->def.bit_size == 64);
   assert(load_desc->def.num_components == 1);
   nir_def_rewrite_uses(&load_desc->def, desc);
               }
      static bool
   lower_direct_buffer_instr(nir_builder *b, nir_instr *instr, void *_state)
   {
               if (instr->type != nir_instr_type_intrinsic)
            nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(instr);
   switch (intrin->intrinsic) {
   case nir_intrinsic_load_deref:
   case nir_intrinsic_store_deref:
   case nir_intrinsic_deref_atomic:
   case nir_intrinsic_deref_atomic_swap:
            case nir_intrinsic_load_vulkan_descriptor:
      if (nir_intrinsic_desc_type(intrin) ==
      VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR)
            default:
            }
      static bool
   lower_res_index_intrinsic(nir_builder *b, nir_intrinsic_instr *intrin,
         {
               nir_address_format addr_format =
            nir_def *index =
      build_res_index(b, nir_intrinsic_desc_set(intrin),
                     assert(intrin->def.bit_size == index->bit_size);
   assert(intrin->def.num_components == index->num_components);
   nir_def_rewrite_uses(&intrin->def, index);
               }
      static bool
   lower_res_reindex_intrinsic(nir_builder *b, nir_intrinsic_instr *intrin,
         {
               nir_address_format addr_format =
            nir_def *index =
      build_res_reindex(b, intrin->src[0].ssa,
               assert(intrin->def.bit_size == index->bit_size);
   assert(intrin->def.num_components == index->num_components);
   nir_def_rewrite_uses(&intrin->def, index);
               }
      static bool
   lower_load_vulkan_descriptor(nir_builder *b, nir_intrinsic_instr *intrin,
         {
               const VkDescriptorType desc_type = nir_intrinsic_desc_type(intrin);
            nir_def *desc =
      build_buffer_addr_for_res_index(b, desc_type, intrin->src[0].ssa,
         assert(intrin->def.bit_size == desc->bit_size);
   assert(intrin->def.num_components == desc->num_components);
   nir_def_rewrite_uses(&intrin->def, desc);
               }
      static bool
   lower_get_ssbo_size(nir_builder *b, nir_intrinsic_instr *intrin,
         {
      if (_mesa_set_search(state->lowered_instrs, intrin))
                     nir_address_format addr_format =
            nir_def *desc =
      build_buffer_addr_for_res_index(b, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
         switch (addr_format) {
   case nir_address_format_64bit_global_32bit_offset:
   case nir_address_format_64bit_bounded_global: {
      nir_def *size = nir_channel(b, desc, 2);
   nir_def_rewrite_uses(&intrin->def, size);
   nir_instr_remove(&intrin->instr);
               case nir_address_format_32bit_index_offset:
      /* The binding table index is the first component of the address.  The
   * back-end wants a scalar binding table index source.
   */
   nir_src_rewrite(&intrin->src[0], nir_channel(b, desc, 0));
         default:
                     }
      static bool
   image_binding_needs_lowered_surface(nir_variable *var)
   {
      return !(var->data.access & ACCESS_NON_READABLE) &&
      }
      static bool
   lower_image_intrinsic(nir_builder *b, nir_intrinsic_instr *intrin,
         {
      nir_deref_instr *deref = nir_src_as_deref(intrin->src[0]);
            unsigned set = var->data.descriptor_set;
   unsigned binding = var->data.binding;
                     if (intrin->intrinsic == nir_intrinsic_image_deref_load_param_intel) {
                        nir_def *desc =
      build_load_var_deref_descriptor_mem(b, deref, param * 16,
                  } else {
      nir_def *index = NULL;
   if (deref->deref_type != nir_deref_type_var) {
      assert(deref->deref_type == nir_deref_type_array);
      } else {
                  index = nir_iadd_imm(b, index, binding_offset);
                  }
      static bool
   lower_load_constant(nir_builder *b, nir_intrinsic_instr *intrin,
         {
               /* Any constant-offset load_constant instructions should have been removed
   * by constant folding.
   */
   assert(!nir_src_is_const(intrin->src[0]));
   nir_def *offset = nir_iadd_imm(b, intrin->src[0].ssa,
            nir_def *data;
   if (!anv_use_relocations(state->pdevice)) {
      unsigned load_size = intrin->def.num_components *
                  assert(load_size < b->shader->constant_data_size);
   unsigned max_offset = b->shader->constant_data_size - load_size;
            nir_def *const_data_base_addr = nir_pack_64_2x32_split(b,
                  data = nir_load_global_constant(b, nir_iadd(b, const_data_base_addr,
                        } else {
               data = nir_load_ubo(b, intrin->num_components, intrin->def.bit_size,
                     index, offset,
                           }
      static bool
   lower_base_workgroup_id(nir_builder *b, nir_intrinsic_instr *intrin,
         {
               nir_def *base_workgroup_id =
      nir_load_push_constant(b, 3, 32, nir_imm_int(b, 0),
                        }
      static void
   lower_tex_deref(nir_builder *b, nir_tex_instr *tex,
                     {
      int deref_src_idx = nir_tex_instr_src_index(tex, deref_src_type);
   if (deref_src_idx < 0)
            nir_deref_instr *deref = nir_src_as_deref(tex->src[deref_src_idx].src);
            unsigned set = var->data.descriptor_set;
   unsigned binding = var->data.binding;
   unsigned array_size =
            unsigned binding_offset;
   if (deref_src_type == nir_tex_src_texture_deref) {
         } else {
      assert(deref_src_type == nir_tex_src_sampler_deref);
               nir_tex_src_type offset_src_type;
   nir_def *index = NULL;
   if (binding_offset > MAX_BINDING_TABLE_SIZE) {
      const unsigned plane_offset =
            nir_def *desc =
                  if (deref_src_type == nir_tex_src_texture_deref) {
      offset_src_type = nir_tex_src_texture_handle;
      } else {
      assert(deref_src_type == nir_tex_src_sampler_deref);
   offset_src_type = nir_tex_src_sampler_handle;
         } else {
      if (deref_src_type == nir_tex_src_texture_deref) {
         } else {
      assert(deref_src_type == nir_tex_src_sampler_deref);
                        if (deref->deref_type != nir_deref_type_var) {
               if (nir_src_is_const(deref->arr.index)) {
      unsigned arr_index = MIN2(nir_src_as_uint(deref->arr.index), array_size - 1);
   struct anv_sampler **immutable_samplers =
         if (immutable_samplers) {
      /* Array of YCbCr samplers are tightly packed in the binding
   * tables, compute the offset of an element in the array by
   * adding the number of planes of all preceding elements.
   */
   unsigned desc_arr_index = 0;
   for (int i = 0; i < arr_index; i++)
            } else {
            } else {
      /* From VK_KHR_sampler_ycbcr_conversion:
   *
   * If sampler Y’CBCR conversion is enabled, the combined image
   * sampler must be indexed only by constant integral expressions
   * when aggregated into arrays in shader code, irrespective of
   * the shaderSampledImageArrayDynamicIndexing feature.
                                    if (index) {
      nir_src_rewrite(&tex->src[deref_src_idx].src, index);
      } else {
            }
      static uint32_t
   tex_instr_get_and_remove_plane_src(nir_tex_instr *tex)
   {
      int plane_src_idx = nir_tex_instr_src_index(tex, nir_tex_src_plane);
   if (plane_src_idx < 0)
                                 }
      static nir_def *
   build_def_array_select(nir_builder *b, nir_def **srcs, nir_def *idx,
         {
      if (start == end - 1) {
         } else {
      unsigned mid = start + (end - start) / 2;
   return nir_bcsel(b, nir_ilt_imm(b, idx, mid),
               }
      static void
   lower_gfx7_tex_swizzle(nir_builder *b, nir_tex_instr *tex, unsigned plane,
         {
      assert(state->pdevice->info.verx10 == 70);
   if (tex->sampler_dim == GLSL_SAMPLER_DIM_BUF ||
      nir_tex_instr_is_query(tex) ||
   tex->op == nir_texop_tg4 || /* We can't swizzle TG4 */
   (tex->is_shadow && tex->is_new_style_shadow))
         int deref_src_idx = nir_tex_instr_src_index(tex, nir_tex_src_texture_deref);
            nir_deref_instr *deref = nir_src_as_deref(tex->src[deref_src_idx].src);
            unsigned set = var->data.descriptor_set;
   unsigned binding = var->data.binding;
   const struct anv_descriptor_set_binding_layout *bind_layout =
            if ((bind_layout->data & ANV_DESCRIPTOR_TEXTURE_SWIZZLE) == 0)
                     const unsigned plane_offset =
         nir_def *swiz =
      build_load_var_deref_descriptor_mem(b, deref, plane_offset,
                  assert(tex->def.bit_size == 32);
            /* Initializing to undef is ok; nir_opt_undef will clean it up. */
   nir_def *undef = nir_undef(b, 1, 32);
   nir_def *comps[8];
   for (unsigned i = 0; i < ARRAY_SIZE(comps); i++)
            comps[ISL_CHANNEL_SELECT_ZERO] = nir_imm_int(b, 0);
   if (nir_alu_type_get_base_type(tex->dest_type) == nir_type_float)
         else
         comps[ISL_CHANNEL_SELECT_RED] = nir_channel(b, &tex->def, 0);
   comps[ISL_CHANNEL_SELECT_GREEN] = nir_channel(b, &tex->def, 1);
   comps[ISL_CHANNEL_SELECT_BLUE] = nir_channel(b, &tex->def, 2);
            nir_def *swiz_comps[4];
   for (unsigned i = 0; i < 4; i++) {
      nir_def *comp_swiz = nir_extract_u8(b, swiz, nir_imm_int(b, i));
      }
            /* Rewrite uses before we insert so we don't rewrite this use */
   nir_def_rewrite_uses_after(&tex->def,
            }
      static bool
   lower_tex(nir_builder *b, nir_tex_instr *tex,
         {
               /* On Ivy Bridge and Bay Trail, we have to swizzle in the shader.  Do this
   * before we lower the derefs away so we can still find the descriptor.
   */
   if (state->pdevice->info.verx10 == 70)
                     lower_tex_deref(b, tex, nir_tex_src_texture_deref,
            lower_tex_deref(b, tex, nir_tex_src_sampler_deref,
               }
      static bool
   apply_pipeline_layout(nir_builder *b, nir_instr *instr, void *_state)
   {
               switch (instr->type) {
   case nir_instr_type_intrinsic: {
      nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(instr);
   switch (intrin->intrinsic) {
   case nir_intrinsic_vulkan_resource_index:
         case nir_intrinsic_vulkan_resource_reindex:
         case nir_intrinsic_load_vulkan_descriptor:
         case nir_intrinsic_get_ssbo_size:
         case nir_intrinsic_image_deref_load:
   case nir_intrinsic_image_deref_store:
   case nir_intrinsic_image_deref_atomic:
   case nir_intrinsic_image_deref_atomic_swap:
   case nir_intrinsic_image_deref_size:
   case nir_intrinsic_image_deref_samples:
   case nir_intrinsic_image_deref_load_param_intel:
   case nir_intrinsic_image_deref_load_raw_intel:
   case nir_intrinsic_image_deref_store_raw_intel:
         case nir_intrinsic_load_constant:
         case nir_intrinsic_load_base_workgroup_id:
         default:
         }
      }
   case nir_instr_type_tex:
         default:
            }
      struct binding_info {
      uint32_t binding;
   uint8_t set;
      };
      static int
   compare_binding_infos(const void *_a, const void *_b)
   {
      const struct binding_info *a = _a, *b = _b;
   if (a->score != b->score)
            if (a->set != b->set)
               }
      void
   anv_nir_apply_pipeline_layout(nir_shader *shader,
                           {
               struct apply_pipeline_layout_state state = {
      .pdevice = pdevice,
   .layout = layout,
   .ssbo_addr_format = anv_nir_ssbo_addr_format(pdevice, robust_flags),
   .ubo_addr_format = anv_nir_ubo_addr_format(pdevice, robust_flags),
               for (unsigned s = 0; s < layout->num_sets; s++) {
      const unsigned count = layout->set[s].layout->binding_count;
   state.set[s].use_count = rzalloc_array(mem_ctx, uint8_t, count);
   state.set[s].surface_offsets = rzalloc_array(mem_ctx, uint8_t, count);
               nir_shader_instructions_pass(shader, get_used_bindings,
            for (unsigned s = 0; s < layout->num_sets; s++) {
      if (state.set[s].desc_buffer_used) {
      map->surface_to_descriptor[map->surface_count] =
      (struct anv_pipeline_binding) {
      .set = ANV_DESCRIPTOR_SET_DESCRIPTORS,
         state.set[s].desc_offset = map->surface_count;
                  if (state.uses_constants && anv_use_relocations(pdevice)) {
      state.constants_offset = map->surface_count;
   map->surface_to_descriptor[map->surface_count].set =
                     unsigned used_binding_count = 0;
   for (uint32_t set = 0; set < layout->num_sets; set++) {
      struct anv_descriptor_set_layout *set_layout = layout->set[set].layout;
   for (unsigned b = 0; b < set_layout->binding_count; b++) {
                                    struct binding_info *infos =
         used_binding_count = 0;
   for (uint32_t set = 0; set < layout->num_sets; set++) {
      const struct anv_descriptor_set_layout *set_layout = layout->set[set].layout;
   for (unsigned b = 0; b < set_layout->binding_count; b++) {
                                    /* Do a fixed-point calculation to generate a score based on the
   * number of uses and the binding array size.  We shift by 7 instead
   * of 8 because we're going to use the top bit below to make
   * everything which does not support bindless super higher priority
   * than things which do.
   */
                  /* If the descriptor type doesn't support bindless then put it at the
   * beginning so we guarantee it gets a slot.
   */
   if (!anv_descriptor_supports_bindless(pdevice, binding, true) ||
                  infos[used_binding_count++] = (struct binding_info) {
      .set = set,
   .binding = b,
                     /* Order the binding infos based on score with highest scores first.  If
   * scores are equal we then order by set and binding.
   */
   qsort(infos, used_binding_count, sizeof(struct binding_info),
            for (unsigned i = 0; i < used_binding_count; i++) {
      unsigned set = infos[i].set, b = infos[i].binding;
   const struct anv_descriptor_set_binding_layout *binding =
                     if (binding->dynamic_offset_index >= 0)
            if (binding->data & ANV_DESCRIPTOR_SURFACE_STATE) {
      assert(map->surface_count + array_size <= MAX_BINDING_TABLE_SIZE);
   assert(!anv_descriptor_requires_bindless(pdevice, binding, false));
   state.set[set].surface_offsets[b] = map->surface_count;
   if (binding->dynamic_offset_index < 0) {
      struct anv_sampler **samplers = binding->immutable_samplers;
   for (unsigned i = 0; i < binding->array_size; i++) {
      uint8_t planes = samplers ? samplers[i]->n_planes : 1;
   for (uint8_t p = 0; p < planes; p++) {
      map->surface_to_descriptor[map->surface_count++] =
      (struct anv_pipeline_binding) {
      .set = set,
   .index = binding->descriptor_index + i,
            } else {
      for (unsigned i = 0; i < binding->array_size; i++) {
      map->surface_to_descriptor[map->surface_count++] =
      (struct anv_pipeline_binding) {
      .set = set,
   .index = binding->descriptor_index + i,
   .dynamic_offset_index =
      layout->set[set].dynamic_offset_start +
      }
               if (binding->data & ANV_DESCRIPTOR_SAMPLER_STATE) {
      if (map->sampler_count + array_size > MAX_SAMPLER_TABLE_SIZE ||
      anv_descriptor_requires_bindless(pdevice, binding, true)) {
   /* If this descriptor doesn't fit in the binding table or if it
   * requires bindless for some reason, flag it as bindless.
   *
   * We also make large sampler arrays bindless because we can avoid
   * using indirect sends thanks to bindless samplers being packed
   * less tightly than the sampler table.
   */
   assert(anv_descriptor_supports_bindless(pdevice, binding, true));
      } else {
      state.set[set].sampler_offsets[b] = map->sampler_count;
   struct anv_sampler **samplers = binding->immutable_samplers;
   for (unsigned i = 0; i < binding->array_size; i++) {
      uint8_t planes = samplers ? samplers[i]->n_planes : 1;
   for (uint8_t p = 0; p < planes; p++) {
      map->sampler_to_descriptor[map->sampler_count++] =
      (struct anv_pipeline_binding) {
      .set = set,
   .index = binding->descriptor_index + i,
                           nir_foreach_image_variable(var, shader) {
      const uint32_t set = var->data.descriptor_set;
   const uint32_t binding = var->data.binding;
   const struct anv_descriptor_set_binding_layout *bind_layout =
                  if (state.set[set].use_count[binding] == 0)
            if (state.set[set].surface_offsets[binding] >= MAX_BINDING_TABLE_SIZE)
            struct anv_pipeline_binding *pipe_binding =
         for (unsigned i = 0; i < array_size; i++) {
                     pipe_binding[i].lowered_storage_surface =
                  /* Before we do the normal lowering, we look for any SSBO operations
   * that we can lower to the BTI model and lower them up-front.  The BTI
   * model can perform better than the A64 model for a couple reasons:
   *
   *  1. 48-bit address calculations are potentially expensive and using
   *     the BTI model lets us simply compute 32-bit offsets and the
   *     hardware adds the 64-bit surface base address.
   *
   *  2. The BTI messages, because they use surface states, do bounds
   *     checking for us.  With the A64 model, we have to do our own
   *     bounds checking and this means wider pointers and extra
   *     calculations and branching in the shader.
   *
   * The solution to both of these is to convert things to the BTI model
   * opportunistically.  The reason why we need to do this as a pre-pass
   * is for two reasons:
   *
   *  1. The BTI model requires nir_address_format_32bit_index_offset
   *     pointers which are not the same type as the pointers needed for
   *     the A64 model.  Because all our derefs are set up for the A64
   *     model (in case we have variable pointers), we have to crawl all
   *     the way back to the vulkan_resource_index intrinsic and build a
   *     completely fresh index+offset calculation.
   *
   *  2. Because the variable-pointers-capable lowering that we do as part
   *     of apply_pipeline_layout_block is destructive (It really has to
   *     be to handle variable pointers properly), we've lost the deref
   *     information by the time we get to the load/store/atomic
   *     intrinsics in that pass.
   */
   nir_shader_instructions_pass(shader, lower_direct_buffer_instr,
                        /* We just got rid of all the direct access.  Delete it so it's not in the
   * way when we do our indirect lowering.
   */
            nir_shader_instructions_pass(shader, apply_pipeline_layout,
                                 /* Now that we're done computing the surface and sampler portions of the
   * bind map, hash them.  This lets us quickly determine if the actual
   * mapping has changed and not just a no-op pipeline change.
   */
   _mesa_sha1_compute(map->surface_to_descriptor,
               _mesa_sha1_compute(map->sampler_to_descriptor,
            }
