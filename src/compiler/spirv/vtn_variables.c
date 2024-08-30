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
      #include "vtn_private.h"
   #include "spirv_info.h"
   #include "nir_deref.h"
   #include <vulkan/vulkan_core.h>
      static struct vtn_pointer*
   vtn_align_pointer(struct vtn_builder *b, struct vtn_pointer *ptr,
         {
      if (alignment == 0)
            if (!util_is_power_of_two_nonzero(alignment)) {
      vtn_warn("Provided alignment is not a power of two");
               /* If this pointer doesn't have a deref, bail.  This either means we're
   * using the old offset+alignment pointers which don't support carrying
   * alignment information or we're a pointer that is below the block
   * boundary in our access chain in which case alignment is meaningless.
   */
   if (ptr->deref == NULL)
            /* Ignore alignment information on logical pointers.  This way, we don't
   * trip up drivers with unnecessary casts.
   */
   nir_address_format addr_format = vtn_mode_to_address_format(b, ptr->mode);
   if (addr_format == nir_address_format_logical)
            struct vtn_pointer *copy = ralloc(b, struct vtn_pointer);
   *copy = *ptr;
               }
      static void
   ptr_decoration_cb(struct vtn_builder *b, struct vtn_value *val, int member,
         {
               switch (dec->decoration) {
   case SpvDecorationNonUniformEXT:
      ptr->access |= ACCESS_NON_UNIFORM;
         default:
            }
      struct access_align {
      enum gl_access_qualifier access;
      };
      static void
   access_align_cb(struct vtn_builder *b, struct vtn_value *val, int member,
         {
               switch (dec->decoration) {
   case SpvDecorationAlignment:
      aa->alignment = dec->operands[0];
         case SpvDecorationNonUniformEXT:
      aa->access |= ACCESS_NON_UNIFORM;
         default:
            }
      static struct vtn_pointer*
   vtn_decorate_pointer(struct vtn_builder *b, struct vtn_value *val,
         {
      struct access_align aa = { 0, };
                     /* If we're adding access flags, make a copy of the pointer.  We could
   * probably just OR them in without doing so but this prevents us from
   * leaking them any further than actually specified in the SPIR-V.
   */
   if (aa.access & ~ptr->access) {
      struct vtn_pointer *copy = ralloc(b, struct vtn_pointer);
   *copy = *ptr;
   copy->access |= aa.access;
                  }
      struct vtn_value *
   vtn_push_pointer(struct vtn_builder *b, uint32_t value_id,
         {
      struct vtn_value *val = vtn_push_value(b, value_id, vtn_value_type_pointer);
   val->pointer = vtn_decorate_pointer(b, val, ptr);
      }
      void
   vtn_copy_value(struct vtn_builder *b, uint32_t src_value_id,
         {
      struct vtn_value *src = vtn_untyped_value(b, src_value_id);
            vtn_fail_if(dst->value_type != vtn_value_type_invalid,
                  vtn_fail_if(dst->type->id != src->type->id,
            if (src->value_type == vtn_value_type_ssa && src->ssa->is_variable) {
      nir_variable *dst_var =
         nir_deref_instr *dst_deref = nir_build_deref_var(&b->nb, dst_var);
                     vtn_push_var_ssa(b, dst_value_id, dst_var);
               struct vtn_value src_copy = *src;
   src_copy.name = dst->name;
   src_copy.decoration = dst->decoration;
   src_copy.type = dst->type;
            if (dst->value_type == vtn_value_type_pointer)
      }
      static struct vtn_access_chain *
   vtn_access_chain_create(struct vtn_builder *b, unsigned length)
   {
               /* Subtract 1 from the length since there's already one built in */
   size_t size = sizeof(*chain) +
         chain = rzalloc_size(b, size);
               }
      static bool
   vtn_mode_is_cross_invocation(struct vtn_builder *b,
         {
      /* TODO: add TCS here once nir_remove_unused_io_vars() can handle vector indexing. */
   bool cross_invocation_outputs = b->shader->info.stage == MESA_SHADER_MESH;
   return mode == vtn_variable_mode_ssbo ||
         mode == vtn_variable_mode_ubo ||
   mode == vtn_variable_mode_phys_ssbo ||
   mode == vtn_variable_mode_push_constant ||
   mode == vtn_variable_mode_workgroup ||
   mode == vtn_variable_mode_cross_workgroup ||
   mode == vtn_variable_mode_node_payload ||
      }
      static bool
   vtn_pointer_is_external_block(struct vtn_builder *b,
         {
      return ptr->mode == vtn_variable_mode_ssbo ||
            }
      static nir_def *
   vtn_access_link_as_ssa(struct vtn_builder *b, struct vtn_access_link link,
         {
      vtn_assert(stride > 0);
   if (link.mode == vtn_access_mode_literal) {
         } else {
      nir_def *ssa = vtn_ssa_value(b, link.id)->def;
   if (ssa->bit_size != bit_size)
               }
      static VkDescriptorType
   vk_desc_type_for_mode(struct vtn_builder *b, enum vtn_variable_mode mode)
   {
      switch (mode) {
   case vtn_variable_mode_ubo:
         case vtn_variable_mode_ssbo:
         case vtn_variable_mode_accel_struct:
         default:
            }
      static nir_def *
   vtn_variable_resource_index(struct vtn_builder *b, struct vtn_variable *var,
         {
               if (!desc_array_index)
            if (b->vars_used_indirectly) {
      vtn_assert(var->var);
               nir_intrinsic_instr *instr =
      nir_intrinsic_instr_create(b->nb.shader,
      instr->src[0] = nir_src_for_ssa(desc_array_index);
   nir_intrinsic_set_desc_set(instr, var->descriptor_set);
   nir_intrinsic_set_binding(instr, var->binding);
            nir_address_format addr_format = vtn_mode_to_address_format(b, var->mode);
   nir_def_init(&instr->instr, &instr->def,
               instr->num_components = instr->def.num_components;
               }
      static nir_def *
   vtn_resource_reindex(struct vtn_builder *b, enum vtn_variable_mode mode,
         {
               nir_intrinsic_instr *instr =
      nir_intrinsic_instr_create(b->nb.shader,
      instr->src[0] = nir_src_for_ssa(base_index);
   instr->src[1] = nir_src_for_ssa(offset_index);
            nir_address_format addr_format = vtn_mode_to_address_format(b, mode);
   nir_def_init(&instr->instr, &instr->def,
               instr->num_components = instr->def.num_components;
               }
      static nir_def *
   vtn_descriptor_load(struct vtn_builder *b, enum vtn_variable_mode mode,
         {
               nir_intrinsic_instr *desc_load =
      nir_intrinsic_instr_create(b->nb.shader,
      desc_load->src[0] = nir_src_for_ssa(desc_index);
            nir_address_format addr_format = vtn_mode_to_address_format(b, mode);
   nir_def_init(&desc_load->instr, &desc_load->def,
               desc_load->num_components = desc_load->def.num_components;
               }
      static struct vtn_pointer *
   vtn_pointer_dereference(struct vtn_builder *b,
               {
      struct vtn_type *type = base->type;
   enum gl_access_qualifier access = base->access | deref_chain->access;
            nir_deref_instr *tail;
   if (base->deref) {
         } else if (b->options->environment == NIR_SPIRV_VULKAN &&
            (vtn_pointer_is_external_block(b, base) ||
            /* We dereferencing an external block pointer.  Correctness of this
   * operation relies on one particular line in the SPIR-V spec, section
   * entitled "Validation Rules for Shader Capabilities":
   *
   *    "Block and BufferBlock decorations cannot decorate a structure
   *    type that is nested at any level inside another structure type
   *    decorated with Block or BufferBlock."
   *
   * This means that we can detect the point where we cross over from
   * descriptor indexing to buffer indexing by looking for the block
   * decorated struct type.  Anything before the block decorated struct
   * type is a descriptor indexing operation and anything after the block
   * decorated struct is a buffer offset operation.
            /* Figure out the descriptor array index if any
   *
   * Some of the Vulkan CTS tests with hand-rolled SPIR-V have been known
   * to forget the Block or BufferBlock decoration from time to time.
   * It's more robust if we check for both !block_index and for the type
   * to contain a block.  This way there's a decent chance that arrays of
   * UBOs/SSBOs will work correctly even if variable pointers are
   * completley toast.
   */
   nir_def *desc_arr_idx = NULL;
   if (!block_index || vtn_type_contains_block(b, type) ||
      base->mode == vtn_variable_mode_accel_struct) {
   /* If our type contains a block, then we're still outside the block
   * and we need to process enough levels of dereferences to get inside
   * of it.  Same applies to acceleration structures.
   */
   if (deref_chain->ptr_as_array) {
      unsigned aoa_size = glsl_get_aoa_size(type->type);
   desc_arr_idx = vtn_access_link_as_ssa(b, deref_chain->link[idx],
                     for (; idx < deref_chain->length; idx++) {
      if (type->base_type != vtn_base_type_array) {
                        unsigned aoa_size = glsl_get_aoa_size(type->array_element->type);
   nir_def *arr_offset =
      vtn_access_link_as_ssa(b, deref_chain->link[idx],
      if (desc_arr_idx)
                        type = type->array_element;
                  if (!block_index) {
      vtn_assert(base->var && base->type);
      } else if (desc_arr_idx) {
      block_index = vtn_resource_reindex(b, base->mode,
               if (idx == deref_chain->length) {
      /* The entire deref was consumed in finding the block index.  Return
   * a pointer which just has a block index and a later access chain
   * will dereference deeper.
   */
   struct vtn_pointer *ptr = rzalloc(b, struct vtn_pointer);
   ptr->mode = base->mode;
   ptr->type = type;
   ptr->block_index = block_index;
   ptr->access = access;
               /* If we got here, there's more access chain to handle and we have the
   * final block index.  Insert a descriptor load and cast to a deref to
   * start the deref chain.
   */
            assert(base->mode == vtn_variable_mode_ssbo ||
         nir_variable_mode nir_mode =
         const uint32_t align = base->mode == vtn_variable_mode_ssbo ?
            tail = nir_build_deref_cast(&b->nb, desc, nir_mode,
               tail->cast.align_mul = align;
         } else if (base->mode == vtn_variable_mode_shader_record) {
      /* For ShaderRecordBufferKHR variables, we don't have a nir_variable.
   * It's just a fancy handle around a pointer to the shader record for
   * the current shader.
   */
   tail = nir_build_deref_cast(&b->nb, nir_load_shader_record_ptr(&b->nb),
                        } else {
      assert(base->var && base->var->var);
   tail = nir_build_deref_var(&b->nb, base->var->var);
   if (base->ptr_type && base->ptr_type->type) {
      tail->def.num_components =
                        if (idx == 0 && deref_chain->ptr_as_array) {
      /* We start with a deref cast to get the stride.  Hopefully, we'll be
   * able to delete that cast eventually.
   */
   tail = nir_build_deref_cast(&b->nb, &tail->def, tail->modes,
            nir_def *index = vtn_access_link_as_ssa(b, deref_chain->link[0], 1,
         tail = nir_build_deref_ptr_as_array(&b->nb, tail, index);
               for (; idx < deref_chain->length; idx++) {
      if (glsl_type_is_struct_or_ifc(type->type)) {
      vtn_assert(deref_chain->link[idx].mode == vtn_access_mode_literal);
   unsigned field = deref_chain->link[idx].id;
   tail = nir_build_deref_struct(&b->nb, tail, field);
      } else {
      nir_def *arr_index =
      vtn_access_link_as_ssa(b, deref_chain->link[idx], 1,
      if (type->base_type == vtn_base_type_cooperative_matrix) {
      const struct glsl_type *element_type = glsl_get_cmat_element(type->type);
   tail = nir_build_deref_cast(&b->nb, &tail->def, tail->modes,
            } else {
         }
      }
                        struct vtn_pointer *ptr = rzalloc(b, struct vtn_pointer);
   ptr->mode = base->mode;
   ptr->type = type;
   ptr->var = base->var;
   ptr->deref = tail;
               }
      nir_deref_instr *
   vtn_pointer_to_deref(struct vtn_builder *b, struct vtn_pointer *ptr)
   {
      if (!ptr->deref) {
      struct vtn_access_chain chain = {
         };
                  }
      static void
   _vtn_local_load_store(struct vtn_builder *b, bool load, nir_deref_instr *deref,
               {
      if (glsl_type_is_cmat(deref->type)) {
      if (load) {
      nir_deref_instr *temp = vtn_create_cmat_temporary(b, deref->type, "cmat_ssa");
   nir_cmat_copy(&b->nb, &temp->def, &deref->def);
      } else {
      nir_deref_instr *src_deref = vtn_get_deref_for_ssa_value(b, inout);
         } else if (glsl_type_is_vector_or_scalar(deref->type)) {
      if (load) {
         } else {
            } else if (glsl_type_is_array(deref->type) ||
            unsigned elems = glsl_get_length(deref->type);
   for (unsigned i = 0; i < elems; i++) {
      nir_deref_instr *child =
               } else {
      vtn_assert(glsl_type_is_struct_or_ifc(deref->type));
   unsigned elems = glsl_get_length(deref->type);
   for (unsigned i = 0; i < elems; i++) {
      nir_deref_instr *child = nir_build_deref_struct(&b->nb, deref, i);
            }
      nir_deref_instr *
   vtn_nir_deref(struct vtn_builder *b, uint32_t id)
   {
      struct vtn_pointer *ptr = vtn_pointer(b, id);
      }
      /*
   * Gets the NIR-level deref tail, which may have as a child an array deref
   * selecting which component due to OpAccessChain supporting per-component
   * indexing in SPIR-V.
   */
   static nir_deref_instr *
   get_deref_tail(nir_deref_instr *deref)
   {
      if (deref->deref_type != nir_deref_type_array)
            nir_deref_instr *parent =
            if (parent->deref_type == nir_deref_type_cast &&
      parent->parent.ssa->parent_instr->type == nir_instr_type_deref) {
   nir_deref_instr *grandparent =
            if (glsl_type_is_cmat(grandparent->type))
               if (glsl_type_is_vector(parent->type) ||
      glsl_type_is_cmat(parent->type))
      else
      }
      struct vtn_ssa_value *
   vtn_local_load(struct vtn_builder *b, nir_deref_instr *src,
         {
      nir_deref_instr *src_tail = get_deref_tail(src);
   struct vtn_ssa_value *val = vtn_create_ssa_value(b, src_tail->type);
            if (src_tail != src) {
               if (glsl_type_is_cmat(src_tail->type)) {
                     /* Reset is_variable because we are repurposing val. */
   val->is_variable = false;
   val->def = nir_cmat_extract(&b->nb,
            } else {
                        }
      void
   vtn_local_store(struct vtn_builder *b, struct vtn_ssa_value *src,
         {
               if (dest_tail != dest) {
      struct vtn_ssa_value *val = vtn_create_ssa_value(b, dest_tail->type);
            if (glsl_type_is_cmat(dest_tail->type)) {
      nir_deref_instr *mat = vtn_get_deref_for_ssa_value(b, val);
   nir_deref_instr *dst = vtn_create_cmat_temporary(b, dest_tail->type, "cmat_insert");
   nir_cmat_insert(&b->nb, &dst->def, src->def, &mat->def, dest->arr.index.ssa);
      } else {
      val->def = nir_vector_insert(&b->nb, val->def, src->def,
                  } else {
            }
      static nir_def *
   vtn_pointer_to_descriptor(struct vtn_builder *b, struct vtn_pointer *ptr)
   {
      assert(ptr->mode == vtn_variable_mode_accel_struct);
   if (!ptr->block_index) {
      struct vtn_access_chain chain = {
         };
               vtn_assert(ptr->deref == NULL && ptr->block_index != NULL);
      }
      static void
   _vtn_variable_load_store(struct vtn_builder *b, bool load,
                     {
      if (ptr->mode == vtn_variable_mode_uniform ||
      ptr->mode == vtn_variable_mode_image) {
   if (ptr->type->base_type == vtn_base_type_image ||
      ptr->type->base_type == vtn_base_type_sampler) {
   /* See also our handling of OpTypeSampler and OpTypeImage */
   vtn_assert(load);
   (*inout)->def = vtn_pointer_to_ssa(b, ptr);
      } else if (ptr->type->base_type == vtn_base_type_sampled_image) {
      /* See also our handling of OpTypeSampledImage */
   vtn_assert(load);
   struct vtn_sampled_image si = {
      .image = vtn_pointer_to_deref(b, ptr),
      };
   (*inout)->def = vtn_sampled_image_to_nir_ssa(b, si);
         } else if (ptr->mode == vtn_variable_mode_accel_struct) {
      vtn_assert(load);
   (*inout)->def = vtn_pointer_to_descriptor(b, ptr);
               enum glsl_base_type base_type = glsl_get_base_type(ptr->type->type);
   switch (base_type) {
   case GLSL_TYPE_UINT:
   case GLSL_TYPE_INT:
   case GLSL_TYPE_UINT16:
   case GLSL_TYPE_INT16:
   case GLSL_TYPE_UINT8:
   case GLSL_TYPE_INT8:
   case GLSL_TYPE_UINT64:
   case GLSL_TYPE_INT64:
   case GLSL_TYPE_FLOAT:
   case GLSL_TYPE_FLOAT16:
   case GLSL_TYPE_BOOL:
   case GLSL_TYPE_DOUBLE:
   case GLSL_TYPE_COOPERATIVE_MATRIX:
      if (glsl_type_is_vector_or_scalar(ptr->type->type)) {
      /* We hit a vector or scalar; go ahead and emit the load[s] */
   nir_deref_instr *deref = vtn_pointer_to_deref(b, ptr);
   if (vtn_mode_is_cross_invocation(b, ptr->mode)) {
      /* If it's cross-invocation, we call nir_load/store_deref
   * directly.  The vtn_local_load/store helpers are too clever and
   * do magic to avoid array derefs of vectors.  That magic is both
   * less efficient than the direct load/store and, in the case of
   * stores, is broken because it creates a race condition if two
   * threads are writing to different components of the same vector
   * due to the load+insert+store it uses to emulate the array
   * deref.
   */
   if (load) {
      (*inout)->def = nir_load_deref_with_access(&b->nb, deref,
      } else {
      nir_store_deref_with_access(&b->nb, deref, (*inout)->def, ~0,
         } else {
      if (load) {
         } else {
            }
      }
         case GLSL_TYPE_INTERFACE:
   case GLSL_TYPE_ARRAY:
   case GLSL_TYPE_STRUCT: {
      unsigned elems = glsl_get_length(ptr->type->type);
   struct vtn_access_chain chain = {
      .length = 1,
   .link = {
            };
   for (unsigned i = 0; i < elems; i++) {
      chain.link[0].id = i;
   struct vtn_pointer *elem = vtn_pointer_dereference(b, ptr, &chain);
   _vtn_variable_load_store(b, load, elem, ptr->type->access | access,
      }
               default:
            }
      struct vtn_ssa_value *
   vtn_variable_load(struct vtn_builder *b, struct vtn_pointer *src,
         {
      struct vtn_ssa_value *val = vtn_create_ssa_value(b, src->type->type);
   _vtn_variable_load_store(b, true, src, src->access | access, &val);
      }
      void
   vtn_variable_store(struct vtn_builder *b, struct vtn_ssa_value *src,
         {
         }
      static void
   _vtn_variable_copy(struct vtn_builder *b, struct vtn_pointer *dest,
               {
      vtn_assert(glsl_get_bare_type(src->type->type) ==
         enum glsl_base_type base_type = glsl_get_base_type(src->type->type);
   switch (base_type) {
   case GLSL_TYPE_UINT:
   case GLSL_TYPE_INT:
   case GLSL_TYPE_UINT16:
   case GLSL_TYPE_INT16:
   case GLSL_TYPE_UINT8:
   case GLSL_TYPE_INT8:
   case GLSL_TYPE_UINT64:
   case GLSL_TYPE_INT64:
   case GLSL_TYPE_FLOAT:
   case GLSL_TYPE_FLOAT16:
   case GLSL_TYPE_DOUBLE:
   case GLSL_TYPE_BOOL:
      /* At this point, we have a scalar, vector, or matrix so we know that
   * there cannot be any structure splitting still in the way.  By
   * stopping at the matrix level rather than the vector level, we
   * ensure that matrices get loaded in the optimal way even if they
   * are storred row-major in a UBO.
   */
   vtn_variable_store(b, vtn_variable_load(b, src, src_access), dest, dest_access);
         case GLSL_TYPE_INTERFACE:
   case GLSL_TYPE_ARRAY:
   case GLSL_TYPE_STRUCT: {
      struct vtn_access_chain chain = {
      .length = 1,
   .link = {
            };
   unsigned elems = glsl_get_length(src->type->type);
   for (unsigned i = 0; i < elems; i++) {
      chain.link[0].id = i;
   struct vtn_pointer *src_elem =
                           }
               default:
            }
      static void
   vtn_variable_copy(struct vtn_builder *b, struct vtn_pointer *dest,
               {
      /* TODO: At some point, we should add a special-case for when we can
   * just emit a copy_var intrinsic.
   */
      }
      static void
   set_mode_system_value(struct vtn_builder *b, nir_variable_mode *mode)
   {
      vtn_assert(*mode == nir_var_system_value || *mode == nir_var_shader_in ||
                  }
      static void
   vtn_get_builtin_location(struct vtn_builder *b,
               {
      switch (builtin) {
   case SpvBuiltInPosition:
   case SpvBuiltInPositionPerViewNV:
      *location = VARYING_SLOT_POS;
      case SpvBuiltInPointSize:
      *location = VARYING_SLOT_PSIZ;
      case SpvBuiltInClipDistance:
   case SpvBuiltInClipDistancePerViewNV:
      *location = VARYING_SLOT_CLIP_DIST0;
      case SpvBuiltInCullDistance:
   case SpvBuiltInCullDistancePerViewNV:
      *location = VARYING_SLOT_CULL_DIST0;
      case SpvBuiltInVertexId:
   case SpvBuiltInVertexIndex:
      /* The Vulkan spec defines VertexIndex to be non-zero-based and doesn't
   * allow VertexId.  The ARB_gl_spirv spec defines VertexId to be the
   * same as gl_VertexID, which is non-zero-based, and removes
   * VertexIndex.  Since they're both defined to be non-zero-based, we use
   * SYSTEM_VALUE_VERTEX_ID for both.
   */
   *location = SYSTEM_VALUE_VERTEX_ID;
   set_mode_system_value(b, mode);
      case SpvBuiltInInstanceIndex:
      *location = SYSTEM_VALUE_INSTANCE_INDEX;
   set_mode_system_value(b, mode);
      case SpvBuiltInInstanceId:
      *location = SYSTEM_VALUE_INSTANCE_ID;
   set_mode_system_value(b, mode);
      case SpvBuiltInPrimitiveId:
      if (b->shader->info.stage == MESA_SHADER_FRAGMENT) {
      vtn_assert(*mode == nir_var_shader_in);
      } else if (*mode == nir_var_shader_out) {
         } else {
      *location = SYSTEM_VALUE_PRIMITIVE_ID;
      }
      case SpvBuiltInInvocationId:
      *location = SYSTEM_VALUE_INVOCATION_ID;
   set_mode_system_value(b, mode);
      case SpvBuiltInLayer:
   case SpvBuiltInLayerPerViewNV:
      *location = VARYING_SLOT_LAYER;
   if (b->shader->info.stage == MESA_SHADER_FRAGMENT)
         else if (b->shader->info.stage == MESA_SHADER_GEOMETRY)
         else if (b->options && b->options->caps.shader_viewport_index_layer &&
            (b->shader->info.stage == MESA_SHADER_VERTEX ||
   b->shader->info.stage == MESA_SHADER_TESS_EVAL ||
      else
            case SpvBuiltInViewportIndex:
      *location = VARYING_SLOT_VIEWPORT;
   if (b->shader->info.stage == MESA_SHADER_GEOMETRY)
         else if (b->options && b->options->caps.shader_viewport_index_layer &&
            (b->shader->info.stage == MESA_SHADER_VERTEX ||
   b->shader->info.stage == MESA_SHADER_TESS_EVAL ||
      else if (b->shader->info.stage == MESA_SHADER_FRAGMENT)
         else
            case SpvBuiltInViewportMaskNV:
   case SpvBuiltInViewportMaskPerViewNV:
      *location = VARYING_SLOT_VIEWPORT_MASK;
   *mode = nir_var_shader_out;
      case SpvBuiltInTessLevelOuter:
      *location = VARYING_SLOT_TESS_LEVEL_OUTER;
      case SpvBuiltInTessLevelInner:
      *location = VARYING_SLOT_TESS_LEVEL_INNER;
      case SpvBuiltInTessCoord:
      *location = SYSTEM_VALUE_TESS_COORD;
   set_mode_system_value(b, mode);
      case SpvBuiltInPatchVertices:
      *location = SYSTEM_VALUE_VERTICES_IN;
   set_mode_system_value(b, mode);
      case SpvBuiltInFragCoord:
      vtn_assert(*mode == nir_var_shader_in);
   *mode = nir_var_system_value;
   *location = SYSTEM_VALUE_FRAG_COORD;
      case SpvBuiltInPointCoord:
      vtn_assert(*mode == nir_var_shader_in);
   set_mode_system_value(b, mode);
   *location = SYSTEM_VALUE_POINT_COORD;
      case SpvBuiltInFrontFacing:
      *location = SYSTEM_VALUE_FRONT_FACE;
   set_mode_system_value(b, mode);
      case SpvBuiltInSampleId:
      *location = SYSTEM_VALUE_SAMPLE_ID;
   set_mode_system_value(b, mode);
      case SpvBuiltInSamplePosition:
      *location = SYSTEM_VALUE_SAMPLE_POS;
   set_mode_system_value(b, mode);
      case SpvBuiltInSampleMask:
      if (*mode == nir_var_shader_out) {
         } else {
      *location = SYSTEM_VALUE_SAMPLE_MASK_IN;
      }
      case SpvBuiltInFragDepth:
      *location = FRAG_RESULT_DEPTH;
   vtn_assert(*mode == nir_var_shader_out);
      case SpvBuiltInHelperInvocation:
      *location = SYSTEM_VALUE_HELPER_INVOCATION;
   set_mode_system_value(b, mode);
      case SpvBuiltInNumWorkgroups:
      *location = SYSTEM_VALUE_NUM_WORKGROUPS;
   set_mode_system_value(b, mode);
      case SpvBuiltInWorkgroupSize:
   case SpvBuiltInEnqueuedWorkgroupSize:
      *location = SYSTEM_VALUE_WORKGROUP_SIZE;
   set_mode_system_value(b, mode);
      case SpvBuiltInWorkgroupId:
      *location = SYSTEM_VALUE_WORKGROUP_ID;
   set_mode_system_value(b, mode);
      case SpvBuiltInLocalInvocationId:
      *location = SYSTEM_VALUE_LOCAL_INVOCATION_ID;
   set_mode_system_value(b, mode);
      case SpvBuiltInLocalInvocationIndex:
      *location = SYSTEM_VALUE_LOCAL_INVOCATION_INDEX;
   set_mode_system_value(b, mode);
      case SpvBuiltInGlobalInvocationId:
      *location = SYSTEM_VALUE_GLOBAL_INVOCATION_ID;
   set_mode_system_value(b, mode);
      case SpvBuiltInGlobalLinearId:
      *location = SYSTEM_VALUE_GLOBAL_INVOCATION_INDEX;
   set_mode_system_value(b, mode);
      case SpvBuiltInGlobalOffset:
      *location = SYSTEM_VALUE_BASE_GLOBAL_INVOCATION_ID;
   set_mode_system_value(b, mode);
      case SpvBuiltInBaseVertex:
      /* OpenGL gl_BaseVertex (SYSTEM_VALUE_BASE_VERTEX) is not the same
   * semantic as Vulkan BaseVertex (SYSTEM_VALUE_FIRST_VERTEX).
   */
   if (b->options->environment == NIR_SPIRV_OPENGL)
         else
         set_mode_system_value(b, mode);
      case SpvBuiltInBaseInstance:
      *location = SYSTEM_VALUE_BASE_INSTANCE;
   set_mode_system_value(b, mode);
      case SpvBuiltInDrawIndex:
      *location = SYSTEM_VALUE_DRAW_ID;
   set_mode_system_value(b, mode);
      case SpvBuiltInSubgroupSize:
   /* TODO once we support non uniform work groups we have to fix this */
   case SpvBuiltInSubgroupMaxSize:
      *location = SYSTEM_VALUE_SUBGROUP_SIZE;
   set_mode_system_value(b, mode);
      case SpvBuiltInSubgroupId:
      *location = SYSTEM_VALUE_SUBGROUP_ID;
   set_mode_system_value(b, mode);
      case SpvBuiltInSubgroupLocalInvocationId:
      *location = SYSTEM_VALUE_SUBGROUP_INVOCATION;
   set_mode_system_value(b, mode);
      case SpvBuiltInNumSubgroups:
   /* TODO once we support non uniform work groups we have to fix this */
   case SpvBuiltInNumEnqueuedSubgroups:
      *location = SYSTEM_VALUE_NUM_SUBGROUPS;
   set_mode_system_value(b, mode);
      case SpvBuiltInDeviceIndex:
      *location = SYSTEM_VALUE_DEVICE_INDEX;
   set_mode_system_value(b, mode);
      case SpvBuiltInViewIndex:
      if (b->options && b->options->view_index_is_input) {
      *location = VARYING_SLOT_VIEW_INDEX;
      } else {
      *location = SYSTEM_VALUE_VIEW_INDEX;
      }
      case SpvBuiltInSubgroupEqMask:
      *location = SYSTEM_VALUE_SUBGROUP_EQ_MASK,
   set_mode_system_value(b, mode);
      case SpvBuiltInSubgroupGeMask:
      *location = SYSTEM_VALUE_SUBGROUP_GE_MASK,
   set_mode_system_value(b, mode);
      case SpvBuiltInSubgroupGtMask:
      *location = SYSTEM_VALUE_SUBGROUP_GT_MASK,
   set_mode_system_value(b, mode);
      case SpvBuiltInSubgroupLeMask:
      *location = SYSTEM_VALUE_SUBGROUP_LE_MASK,
   set_mode_system_value(b, mode);
      case SpvBuiltInSubgroupLtMask:
      *location = SYSTEM_VALUE_SUBGROUP_LT_MASK,
   set_mode_system_value(b, mode);
      case SpvBuiltInFragStencilRefEXT:
      *location = FRAG_RESULT_STENCIL;
   vtn_assert(*mode == nir_var_shader_out);
      case SpvBuiltInWorkDim:
      *location = SYSTEM_VALUE_WORK_DIM;
   set_mode_system_value(b, mode);
      case SpvBuiltInGlobalSize:
      *location = SYSTEM_VALUE_GLOBAL_GROUP_SIZE;
   set_mode_system_value(b, mode);
      case SpvBuiltInBaryCoordNoPerspAMD:
      *location = SYSTEM_VALUE_BARYCENTRIC_LINEAR_PIXEL;
   set_mode_system_value(b, mode);
      case SpvBuiltInBaryCoordNoPerspCentroidAMD:
      *location = SYSTEM_VALUE_BARYCENTRIC_LINEAR_CENTROID;
   set_mode_system_value(b, mode);
      case SpvBuiltInBaryCoordNoPerspSampleAMD:
      *location = SYSTEM_VALUE_BARYCENTRIC_LINEAR_SAMPLE;
   set_mode_system_value(b, mode);
      case SpvBuiltInBaryCoordSmoothAMD:
      *location = SYSTEM_VALUE_BARYCENTRIC_PERSP_PIXEL;
   set_mode_system_value(b, mode);
      case SpvBuiltInBaryCoordSmoothCentroidAMD:
      *location = SYSTEM_VALUE_BARYCENTRIC_PERSP_CENTROID;
   set_mode_system_value(b, mode);
      case SpvBuiltInBaryCoordSmoothSampleAMD:
      *location = SYSTEM_VALUE_BARYCENTRIC_PERSP_SAMPLE;
   set_mode_system_value(b, mode);
      case SpvBuiltInBaryCoordPullModelAMD:
      *location = SYSTEM_VALUE_BARYCENTRIC_PULL_MODEL;
   set_mode_system_value(b, mode);
      case SpvBuiltInLaunchIdKHR:
      *location = SYSTEM_VALUE_RAY_LAUNCH_ID;
   set_mode_system_value(b, mode);
      case SpvBuiltInLaunchSizeKHR:
      *location = SYSTEM_VALUE_RAY_LAUNCH_SIZE;
   set_mode_system_value(b, mode);
      case SpvBuiltInWorldRayOriginKHR:
      *location = SYSTEM_VALUE_RAY_WORLD_ORIGIN;
   set_mode_system_value(b, mode);
      case SpvBuiltInWorldRayDirectionKHR:
      *location = SYSTEM_VALUE_RAY_WORLD_DIRECTION;
   set_mode_system_value(b, mode);
      case SpvBuiltInObjectRayOriginKHR:
      *location = SYSTEM_VALUE_RAY_OBJECT_ORIGIN;
   set_mode_system_value(b, mode);
      case SpvBuiltInObjectRayDirectionKHR:
      *location = SYSTEM_VALUE_RAY_OBJECT_DIRECTION;
   set_mode_system_value(b, mode);
      case SpvBuiltInObjectToWorldKHR:
      *location = SYSTEM_VALUE_RAY_OBJECT_TO_WORLD;
   set_mode_system_value(b, mode);
      case SpvBuiltInWorldToObjectKHR:
      *location = SYSTEM_VALUE_RAY_WORLD_TO_OBJECT;
   set_mode_system_value(b, mode);
      case SpvBuiltInRayTminKHR:
      *location = SYSTEM_VALUE_RAY_T_MIN;
   set_mode_system_value(b, mode);
      case SpvBuiltInRayTmaxKHR:
   case SpvBuiltInHitTNV:
      *location = SYSTEM_VALUE_RAY_T_MAX;
   set_mode_system_value(b, mode);
      case SpvBuiltInInstanceCustomIndexKHR:
      *location = SYSTEM_VALUE_RAY_INSTANCE_CUSTOM_INDEX;
   set_mode_system_value(b, mode);
      case SpvBuiltInHitKindKHR:
      *location = SYSTEM_VALUE_RAY_HIT_KIND;
   set_mode_system_value(b, mode);
      case SpvBuiltInIncomingRayFlagsKHR:
      *location = SYSTEM_VALUE_RAY_FLAGS;
   set_mode_system_value(b, mode);
      case SpvBuiltInRayGeometryIndexKHR:
      *location = SYSTEM_VALUE_RAY_GEOMETRY_INDEX;
   set_mode_system_value(b, mode);
      case SpvBuiltInCullMaskKHR:
      *location = SYSTEM_VALUE_CULL_MASK;
   set_mode_system_value(b, mode);
      case SpvBuiltInShadingRateKHR:
      *location = SYSTEM_VALUE_FRAG_SHADING_RATE;
   set_mode_system_value(b, mode);
      case SpvBuiltInPrimitiveShadingRateKHR:
      if (b->shader->info.stage == MESA_SHADER_VERTEX ||
      b->shader->info.stage == MESA_SHADER_GEOMETRY ||
   b->shader->info.stage == MESA_SHADER_MESH) {
   *location = VARYING_SLOT_PRIMITIVE_SHADING_RATE;
      } else {
         }
      case SpvBuiltInPrimitiveCountNV:
      *location = VARYING_SLOT_PRIMITIVE_COUNT;
      case SpvBuiltInPrimitivePointIndicesEXT:
   case SpvBuiltInPrimitiveLineIndicesEXT:
   case SpvBuiltInPrimitiveTriangleIndicesEXT:
   case SpvBuiltInPrimitiveIndicesNV:
      *location = VARYING_SLOT_PRIMITIVE_INDICES;
      case SpvBuiltInTaskCountNV:
      /* NV_mesh_shader only. */
   *location = VARYING_SLOT_TASK_COUNT;
   *mode = nir_var_shader_out;
      case SpvBuiltInMeshViewCountNV:
      *location = SYSTEM_VALUE_MESH_VIEW_COUNT;
   set_mode_system_value(b, mode);
      case SpvBuiltInMeshViewIndicesNV:
      *location = SYSTEM_VALUE_MESH_VIEW_INDICES;
   set_mode_system_value(b, mode);
      case SpvBuiltInCullPrimitiveEXT:
      *location = VARYING_SLOT_CULL_PRIMITIVE;
      case SpvBuiltInFullyCoveredEXT:
      *location = SYSTEM_VALUE_FULLY_COVERED;
   set_mode_system_value(b, mode);
      case SpvBuiltInFragSizeEXT:
      *location = SYSTEM_VALUE_FRAG_SIZE;
   set_mode_system_value(b, mode);
      case SpvBuiltInFragInvocationCountEXT:
      *location = SYSTEM_VALUE_FRAG_INVOCATION_COUNT;
   set_mode_system_value(b, mode);
      case SpvBuiltInHitTriangleVertexPositionsKHR:
      *location = SYSTEM_VALUE_RAY_TRIANGLE_VERTEX_POSITIONS;
   set_mode_system_value(b, mode);
      case SpvBuiltInBaryCoordKHR:
      *location = SYSTEM_VALUE_BARYCENTRIC_PERSP_COORD;
   set_mode_system_value(b, mode);
      case SpvBuiltInBaryCoordNoPerspKHR:
      *location = SYSTEM_VALUE_BARYCENTRIC_LINEAR_COORD;
   set_mode_system_value(b, mode);
      case SpvBuiltInShaderIndexAMDX:
      *location = SYSTEM_VALUE_SHADER_INDEX;
   set_mode_system_value(b, mode);
      case SpvBuiltInCoalescedInputCountAMDX:
      *location = SYSTEM_VALUE_COALESCED_INPUT_COUNT;
   set_mode_system_value(b, mode);
         default:
      vtn_fail("Unsupported builtin: %s (%u)",
         }
      static void
   apply_var_decoration(struct vtn_builder *b,
               {
      switch (dec->decoration) {
   case SpvDecorationRelaxedPrecision:
      var_data->precision = GLSL_PRECISION_MEDIUM;
      case SpvDecorationNoPerspective:
      var_data->interpolation = INTERP_MODE_NOPERSPECTIVE;
      case SpvDecorationFlat:
      var_data->interpolation = INTERP_MODE_FLAT;
      case SpvDecorationExplicitInterpAMD:
      var_data->interpolation = INTERP_MODE_EXPLICIT;
      case SpvDecorationCentroid:
      var_data->centroid = true;
      case SpvDecorationSample:
      var_data->sample = true;
      case SpvDecorationInvariant:
      var_data->invariant = true;
      case SpvDecorationConstant:
      var_data->read_only = true;
      case SpvDecorationNonReadable:
      var_data->access |= ACCESS_NON_READABLE;
      case SpvDecorationNonWritable:
      var_data->read_only = true;
   var_data->access |= ACCESS_NON_WRITEABLE;
      case SpvDecorationRestrict:
      var_data->access |= ACCESS_RESTRICT;
      case SpvDecorationAliased:
      var_data->access &= ~ACCESS_RESTRICT;
      case SpvDecorationVolatile:
      var_data->access |= ACCESS_VOLATILE;
      case SpvDecorationCoherent:
      var_data->access |= ACCESS_COHERENT;
      case SpvDecorationComponent:
      var_data->location_frac = dec->operands[0];
      case SpvDecorationIndex:
      var_data->index = dec->operands[0];
      case SpvDecorationBuiltIn: {
               nir_variable_mode mode = var_data->mode;
   vtn_get_builtin_location(b, builtin, &var_data->location, &mode);
            switch (builtin) {
   case SpvBuiltInTessLevelOuter:
   case SpvBuiltInTessLevelInner:
   case SpvBuiltInClipDistance:
   case SpvBuiltInClipDistancePerViewNV:
   case SpvBuiltInCullDistance:
   case SpvBuiltInCullDistancePerViewNV:
      var_data->compact = true;
      case SpvBuiltInPrimitivePointIndicesEXT:
   case SpvBuiltInPrimitiveLineIndicesEXT:
   case SpvBuiltInPrimitiveTriangleIndicesEXT:
      /* Not defined as per-primitive in the EXT, but they behave
   * like per-primitive outputs so it's easier to treat them like that.
   * They may still require special treatment in the backend in order to
   * control where and how they are stored.
   *
   * EXT_mesh_shader: write-only array of vectors indexed by the primitive index
   * NV_mesh_shader: read/write flat array
   */
   var_data->per_primitive = true;
      default:
                              case SpvDecorationSpecId:
   case SpvDecorationRowMajor:
   case SpvDecorationColMajor:
   case SpvDecorationMatrixStride:
   case SpvDecorationUniform:
   case SpvDecorationUniformId:
   case SpvDecorationLinkageAttributes:
            case SpvDecorationPatch:
      var_data->patch = true;
         case SpvDecorationLocation:
            case SpvDecorationBlock:
   case SpvDecorationBufferBlock:
   case SpvDecorationArrayStride:
   case SpvDecorationGLSLShared:
   case SpvDecorationGLSLPacked:
            case SpvDecorationBinding:
   case SpvDecorationDescriptorSet:
   case SpvDecorationNoContraction:
   case SpvDecorationInputAttachmentIndex:
      vtn_warn("Decoration not allowed for variable or structure member: %s",
               case SpvDecorationXfbBuffer:
      var_data->explicit_xfb_buffer = true;
   var_data->xfb.buffer = dec->operands[0];
   var_data->always_active_io = true;
      case SpvDecorationXfbStride:
      var_data->explicit_xfb_stride = true;
   var_data->xfb.stride = dec->operands[0];
      case SpvDecorationOffset:
      var_data->explicit_offset = true;
   var_data->offset = dec->operands[0];
         case SpvDecorationStream:
      var_data->stream = dec->operands[0];
         case SpvDecorationCPacked:
   case SpvDecorationSaturatedConversion:
   case SpvDecorationFuncParamAttr:
   case SpvDecorationFPRoundingMode:
   case SpvDecorationFPFastMathMode:
   case SpvDecorationAlignment:
      if (b->shader->info.stage != MESA_SHADER_KERNEL) {
      vtn_warn("Decoration only allowed for CL-style kernels: %s",
      }
         case SpvDecorationUserSemantic:
   case SpvDecorationUserTypeGOOGLE:
      /* User semantic decorations can safely be ignored by the driver. */
         case SpvDecorationRestrictPointerEXT:
   case SpvDecorationAliasedPointerEXT:
      /* TODO: We should actually plumb alias information through NIR. */
         case SpvDecorationPerPrimitiveNV:
      vtn_fail_if(
      !(b->shader->info.stage == MESA_SHADER_MESH && var_data->mode == nir_var_shader_out) &&
   !(b->shader->info.stage == MESA_SHADER_FRAGMENT && var_data->mode == nir_var_shader_in),
      var_data->per_primitive = true;
         case SpvDecorationPerTaskNV:
      vtn_fail_if(
      (b->shader->info.stage != MESA_SHADER_MESH &&
   b->shader->info.stage != MESA_SHADER_TASK) ||
   var_data->mode != nir_var_mem_task_payload,
            case SpvDecorationPerViewNV:
      vtn_fail_if(b->shader->info.stage != MESA_SHADER_MESH,
         var_data->per_view = true;
         case SpvDecorationPerVertexKHR:
      vtn_fail_if(b->shader->info.stage != MESA_SHADER_FRAGMENT,
         var_data->per_vertex = true;
         case SpvDecorationNodeMaxPayloadsAMDX:
      vtn_fail_if(b->shader->info.stage != MESA_SHADER_COMPUTE,
               case SpvDecorationNodeSharesPayloadLimitsWithAMDX:
      vtn_fail_if(b->shader->info.stage != MESA_SHADER_COMPUTE,
               case SpvDecorationPayloadNodeNameAMDX:
      vtn_fail_if(b->shader->info.stage != MESA_SHADER_COMPUTE,
         var_data->node_name = vtn_string_literal(b, dec->operands, dec->num_operands, NULL);
         case SpvDecorationTrackFinishWritingAMDX:
      vtn_fail_if(b->shader->info.stage != MESA_SHADER_COMPUTE,
               default:
            }
      static void
   gather_var_kind_cb(struct vtn_builder *b, struct vtn_value *val, int member,
         {
      struct vtn_variable *vtn_var = void_var;
   switch (dec->decoration) {
   case SpvDecorationPatch:
      vtn_var->var->data.patch = true;
      case SpvDecorationPerPrimitiveNV:
      vtn_var->var->data.per_primitive = true;
      case SpvDecorationPerViewNV:
      vtn_var->var->data.per_view = true;
      default:
      /* Nothing to do. */
         }
      static void
   var_decoration_cb(struct vtn_builder *b, struct vtn_value *val, int member,
         {
               /* Handle decorations that apply to a vtn_variable as a whole */
   switch (dec->decoration) {
   case SpvDecorationBinding:
      vtn_var->binding = dec->operands[0];
   vtn_var->explicit_binding = true;
      case SpvDecorationDescriptorSet:
      vtn_var->descriptor_set = dec->operands[0];
      case SpvDecorationInputAttachmentIndex:
      vtn_var->input_attachment_index = dec->operands[0];
   vtn_var->access |= ACCESS_NON_WRITEABLE;
      case SpvDecorationPatch:
      vtn_var->var->data.patch = true;
      case SpvDecorationOffset:
      vtn_var->offset = dec->operands[0];
      case SpvDecorationNonWritable:
      vtn_var->access |= ACCESS_NON_WRITEABLE;
      case SpvDecorationNonReadable:
      vtn_var->access |= ACCESS_NON_READABLE;
      case SpvDecorationVolatile:
      vtn_var->access |= ACCESS_VOLATILE;
      case SpvDecorationCoherent:
      vtn_var->access |= ACCESS_COHERENT;
      case SpvDecorationCounterBuffer:
      /* Counter buffer decorations can safely be ignored by the driver. */
      default:
                  if (val->value_type == vtn_value_type_pointer) {
      assert(val->pointer->var == void_var);
      } else {
                  /* Location is odd.  If applied to a split structure, we have to walk the
   * whole thing and accumulate the location.  It's easier to handle as a
   * special case.
   */
   if (dec->decoration == SpvDecorationLocation) {
      unsigned location = dec->operands[0];
   if (b->shader->info.stage == MESA_SHADER_FRAGMENT &&
      vtn_var->mode == vtn_variable_mode_output) {
      } else if (b->shader->info.stage == MESA_SHADER_VERTEX &&
               } else if (vtn_var->mode == vtn_variable_mode_input ||
               } else if (vtn_var->mode == vtn_variable_mode_call_data ||
               } else if (vtn_var->mode != vtn_variable_mode_uniform &&
            vtn_warn("Location must be on input, output, uniform, sampler or "
                     if (vtn_var->var->num_members == 0) {
      /* This handles the member and lone variable cases */
      } else {
                     if (member == -1)
         else
                  } else {
      if (vtn_var->var) {
      if (vtn_var->var->num_members == 0) {
      /* We call this function on types as well as variables and not all
   * struct types get split so we can end up having stray member
   * decorations; just ignore them.
   */
   if (member == -1)
      } else if (member >= 0) {
      /* Member decorations must come from a type */
   assert(val->value_type == vtn_value_type_type);
      } else {
      unsigned length =
         for (unsigned i = 0; i < length; i++)
         } else {
      /* A few variables, those with external storage, have no actual
   * nir_variables associated with them.  Fortunately, all decorations
   * we care about for those variables are on the type only.
   */
   vtn_assert(vtn_var->mode == vtn_variable_mode_ubo ||
                  }
      enum vtn_variable_mode
   vtn_storage_class_to_mode(struct vtn_builder *b,
                     {
      enum vtn_variable_mode mode;
   nir_variable_mode nir_mode;
   switch (class) {
   case SpvStorageClassUniform:
      /* Assume it's an UBO if we lack the interface_type. */
   if (!interface_type || interface_type->block) {
      mode = vtn_variable_mode_ubo;
      } else if (interface_type->buffer_block) {
      mode = vtn_variable_mode_ssbo;
      } else {
      /* Default-block uniforms, coming from gl_spirv */
   mode = vtn_variable_mode_uniform;
      }
      case SpvStorageClassStorageBuffer:
      mode = vtn_variable_mode_ssbo;
   nir_mode = nir_var_mem_ssbo;
      case SpvStorageClassPhysicalStorageBuffer:
      mode = vtn_variable_mode_phys_ssbo;
   nir_mode = nir_var_mem_global;
      case SpvStorageClassUniformConstant:
      /* interface_type is only NULL when OpTypeForwardPointer is used and
   * OpTypeForwardPointer can only be used for struct types, not images or
   * acceleration structures.
   */
   if (interface_type)
            if (interface_type &&
      interface_type->base_type == vtn_base_type_image &&
   glsl_type_is_image(interface_type->glsl_image)) {
   mode = vtn_variable_mode_image;
      } else if (b->shader->info.stage == MESA_SHADER_KERNEL) {
      mode = vtn_variable_mode_constant;
      } else {
      /* interface_type is only NULL when OpTypeForwardPointer is used and
   * OpTypeForwardPointer cannot be used with the UniformConstant
   * storage class.
   */
   assert(interface_type != NULL);
   if (interface_type->base_type == vtn_base_type_accel_struct) {
      mode = vtn_variable_mode_accel_struct;
      } else {
      mode = vtn_variable_mode_uniform;
         }
      case SpvStorageClassPushConstant:
      mode = vtn_variable_mode_push_constant;
   nir_mode = nir_var_mem_push_const;
      case SpvStorageClassInput:
      mode = vtn_variable_mode_input;
            /* NV_mesh_shader: fixup due to lack of dedicated storage class */
   if (b->shader->info.stage == MESA_SHADER_MESH) {
      mode = vtn_variable_mode_task_payload;
      }
      case SpvStorageClassOutput:
      mode = vtn_variable_mode_output;
            /* NV_mesh_shader: fixup due to lack of dedicated storage class */
   if (b->shader->info.stage == MESA_SHADER_TASK) {
      mode = vtn_variable_mode_task_payload;
      }
      case SpvStorageClassPrivate:
      mode = vtn_variable_mode_private;
   nir_mode = nir_var_shader_temp;
      case SpvStorageClassFunction:
      mode = vtn_variable_mode_function;
   nir_mode = nir_var_function_temp;
      case SpvStorageClassWorkgroup:
      mode = vtn_variable_mode_workgroup;
   nir_mode = nir_var_mem_shared;
      case SpvStorageClassTaskPayloadWorkgroupEXT:
      mode = vtn_variable_mode_task_payload;
   nir_mode = nir_var_mem_task_payload;
      case SpvStorageClassAtomicCounter:
      mode = vtn_variable_mode_atomic_counter;
   nir_mode = nir_var_uniform;
      case SpvStorageClassCrossWorkgroup:
      mode = vtn_variable_mode_cross_workgroup;
   nir_mode = nir_var_mem_global;
      case SpvStorageClassImage:
      mode = vtn_variable_mode_image;
   nir_mode = nir_var_image;
      case SpvStorageClassCallableDataKHR:
      mode = vtn_variable_mode_call_data;
   nir_mode = nir_var_shader_temp;
      case SpvStorageClassIncomingCallableDataKHR:
      mode = vtn_variable_mode_call_data_in;
   nir_mode = nir_var_shader_call_data;
      case SpvStorageClassRayPayloadKHR:
      mode = vtn_variable_mode_ray_payload;
   nir_mode = nir_var_shader_temp;
      case SpvStorageClassIncomingRayPayloadKHR:
      mode = vtn_variable_mode_ray_payload_in;
   nir_mode = nir_var_shader_call_data;
      case SpvStorageClassHitAttributeKHR:
      mode = vtn_variable_mode_hit_attrib;
   nir_mode = nir_var_ray_hit_attrib;
      case SpvStorageClassShaderRecordBufferKHR:
      mode = vtn_variable_mode_shader_record;
   nir_mode = nir_var_mem_constant;
      case SpvStorageClassNodePayloadAMDX:
      mode = vtn_variable_mode_node_payload;
   nir_mode = nir_var_mem_node_payload_in;
      case SpvStorageClassNodeOutputPayloadAMDX:
      mode = vtn_variable_mode_node_payload;
   nir_mode = nir_var_mem_node_payload;
         case SpvStorageClassGeneric:
      mode = vtn_variable_mode_generic;
   nir_mode = nir_var_mem_generic;
      default:
      vtn_fail("Unhandled variable storage class: %s (%u)",
               if (nir_mode_out)
               }
      nir_address_format
   vtn_mode_to_address_format(struct vtn_builder *b, enum vtn_variable_mode mode)
   {
      switch (mode) {
   case vtn_variable_mode_ubo:
            case vtn_variable_mode_ssbo:
            case vtn_variable_mode_phys_ssbo:
            case vtn_variable_mode_push_constant:
            case vtn_variable_mode_workgroup:
            case vtn_variable_mode_generic:
   case vtn_variable_mode_cross_workgroup:
            case vtn_variable_mode_shader_record:
   case vtn_variable_mode_constant:
            case vtn_variable_mode_accel_struct:
   case vtn_variable_mode_node_payload:
            case vtn_variable_mode_task_payload:
            case vtn_variable_mode_function:
      if (b->physical_ptrs)
               case vtn_variable_mode_private:
   case vtn_variable_mode_uniform:
   case vtn_variable_mode_atomic_counter:
   case vtn_variable_mode_input:
   case vtn_variable_mode_output:
   case vtn_variable_mode_image:
   case vtn_variable_mode_call_data:
   case vtn_variable_mode_call_data_in:
   case vtn_variable_mode_ray_payload:
   case vtn_variable_mode_ray_payload_in:
   case vtn_variable_mode_hit_attrib:
                     }
      nir_def *
   vtn_pointer_to_ssa(struct vtn_builder *b, struct vtn_pointer *ptr)
   {
      if ((vtn_pointer_is_external_block(b, ptr) &&
      vtn_type_contains_block(b, ptr->type) &&
   ptr->mode != vtn_variable_mode_phys_ssbo) ||
   ptr->mode == vtn_variable_mode_accel_struct) {
   /* In this case, we're looking for a block index and not an actual
   * deref.
   *
   * For PhysicalStorageBuffer pointers, we don't have a block index
   * at all because we get the pointer directly from the client.  This
   * assumes that there will never be a SSBO binding variable using the
   * PhysicalStorageBuffer storage class.  This assumption appears
   * to be correct according to the Vulkan spec because the table,
   * "Shader Resource and Storage Class Correspondence," the only the
   * Uniform storage class with BufferBlock or the StorageBuffer
   * storage class with Block can be used.
   */
   if (!ptr->block_index) {
      /* If we don't have a block_index then we must be a pointer to the
   * variable itself.
                  struct vtn_access_chain chain = {
         };
                  } else {
            }
      struct vtn_pointer *
   vtn_pointer_from_ssa(struct vtn_builder *b, nir_def *ssa,
         {
               struct vtn_pointer *ptr = rzalloc(b, struct vtn_pointer);
   struct vtn_type *without_array =
            nir_variable_mode nir_mode;
   ptr->mode = vtn_storage_class_to_mode(b, ptr_type->storage_class,
         ptr->type = ptr_type->deref;
            const struct glsl_type *deref_type =
         if (!vtn_pointer_is_external_block(b, ptr) &&
      ptr->mode != vtn_variable_mode_accel_struct) {
   ptr->deref = nir_build_deref_cast(&b->nb, ssa, nir_mode,
      } else if ((vtn_type_contains_block(b, ptr->type) &&
                  /* This is a pointer to somewhere in an array of blocks, not a
   * pointer to somewhere inside the block.  Set the block index
   * instead of making a cast.
   */
      } else {
      /* This is a pointer to something internal or a pointer inside a
   * block.  It's just a regular cast.
   *
   * For PhysicalStorageBuffer pointers, we don't have a block index
   * at all because we get the pointer directly from the client.  This
   * assumes that there will never be a SSBO binding variable using the
   * PhysicalStorageBuffer storage class.  This assumption appears
   * to be correct according to the Vulkan spec because the table,
   * "Shader Resource and Storage Class Correspondence," the only the
   * Uniform storage class with BufferBlock or the StorageBuffer
   * storage class with Block can be used.
   */
   ptr->deref = nir_build_deref_cast(&b->nb, ssa, nir_mode,
         ptr->deref->def.num_components =
                        }
      static void
   assign_missing_member_locations(struct vtn_variable *var)
   {
      unsigned length =
                  for (unsigned i = 0; i < length; i++) {
      /* From the Vulkan spec:
   *
   * “If the structure type is a Block but without a Location, then each
   *  of its members must have a Location decoration.”
   *
   */
   if (var->type->block) {
      assert(var->base_location != -1 ||
               /* From the Vulkan spec:
   *
   * “Any member with its own Location decoration is assigned that
   *  location. Each remaining member is assigned the location after the
   *  immediately preceding member in declaration order.”
   */
   if (var->var->members[i].location != -1)
         else
            /* Below we use type instead of interface_type, because interface_type
   * is only available when it is a Block. This code also supports
   * input/outputs that are just structs
   */
   const struct glsl_type *member_type =
            location +=
      glsl_count_attribute_slots(member_type,
      }
      nir_deref_instr *
   vtn_get_call_payload_for_location(struct vtn_builder *b, uint32_t location_id)
   {
      uint32_t location = vtn_constant_uint(b, location_id);
   nir_foreach_variable_with_modes(var, b->nb.shader, nir_var_shader_temp) {
      if (var->data.explicit_location &&
      var->data.location == location)
   }
   vtn_fail("Couldn't find variable with a storage class of CallableDataKHR "
      }
      static bool
   vtn_type_is_ray_query(struct vtn_type *type)
   {
         }
      static void
   vtn_create_variable(struct vtn_builder *b, struct vtn_value *val,
               {
      vtn_assert(ptr_type->base_type == vtn_base_type_pointer);
                     enum vtn_variable_mode mode;
   nir_variable_mode nir_mode;
            switch (mode) {
   case vtn_variable_mode_ubo:
      /* There's no other way to get vtn_variable_mode_ubo */
   vtn_assert(without_array->block);
      case vtn_variable_mode_ssbo:
      if (storage_class == SpvStorageClassStorageBuffer &&
      !without_array->block) {
   if (b->variable_pointers) {
      vtn_fail("Variables in the StorageBuffer storage class must "
      } else {
      /* If variable pointers are not present, it's still malformed
   * SPIR-V but we can parse it and do the right thing anyway.
   * Since some of the 8-bit storage tests have bugs in this are,
   * just make it a warning for now.
   */
   vtn_warn("Variables in the StorageBuffer storage class must "
         }
         case vtn_variable_mode_generic:
      vtn_fail("Cannot create a variable with the Generic storage class");
         case vtn_variable_mode_image:
      if (storage_class == SpvStorageClassImage)
         else
               case vtn_variable_mode_phys_ssbo:
      vtn_fail("Cannot create a variable with the "
               default:
      /* No tallying is needed */
               struct vtn_variable *var = rzalloc(b, struct vtn_variable);
   var->type = type;
   var->mode = mode;
            val->pointer = rzalloc(b, struct vtn_pointer);
   val->pointer->mode = var->mode;
   val->pointer->type = var->type;
   val->pointer->ptr_type = ptr_type;
   val->pointer->var = var;
            switch (var->mode) {
   case vtn_variable_mode_function:
   case vtn_variable_mode_private:
   case vtn_variable_mode_uniform:
   case vtn_variable_mode_atomic_counter:
   case vtn_variable_mode_constant:
   case vtn_variable_mode_call_data:
   case vtn_variable_mode_call_data_in:
   case vtn_variable_mode_image:
   case vtn_variable_mode_ray_payload:
   case vtn_variable_mode_ray_payload_in:
   case vtn_variable_mode_hit_attrib:
   case vtn_variable_mode_node_payload:
      /* For these, we create the variable normally */
   var->var = rzalloc(b->shader, nir_variable);
   var->var->name = ralloc_strdup(var->var, val->name);
            /* This is a total hack but we need some way to flag variables which are
   * going to be call payloads.  See get_call_payload_deref.
   */
   if (storage_class == SpvStorageClassCallableDataKHR ||
                  var->var->data.mode = nir_mode;
   var->var->data.location = -1;
   var->var->data.ray_query = vtn_type_is_ray_query(var->type);
   var->var->interface_type = NULL;
         case vtn_variable_mode_ubo:
   case vtn_variable_mode_ssbo:
   case vtn_variable_mode_push_constant:
   case vtn_variable_mode_accel_struct:
   case vtn_variable_mode_shader_record:
      var->var = rzalloc(b->shader, nir_variable);
            var->var->type = vtn_type_get_nir_type(b, var->type, var->mode);
            var->var->data.mode = nir_mode;
   var->var->data.location = -1;
   var->var->data.driver_location = 0;
   var->var->data.access = var->type->access;
         case vtn_variable_mode_workgroup:
   case vtn_variable_mode_cross_workgroup:
   case vtn_variable_mode_task_payload:
      /* Create the variable normally */
   var->var = rzalloc(b->shader, nir_variable);
   var->var->name = ralloc_strdup(var->var, val->name);
   var->var->type = vtn_type_get_nir_type(b, var->type, var->mode);
   var->var->data.mode = nir_mode;
         case vtn_variable_mode_input:
   case vtn_variable_mode_output: {
      var->var = rzalloc(b->shader, nir_variable);
   var->var->name = ralloc_strdup(var->var, val->name);
   var->var->type = vtn_type_get_nir_type(b, var->type, var->mode);
            /* In order to know whether or not we're a per-vertex inout, we need
   * the patch qualifier.  This means walking the variable decorations
   * early before we actually create any variables.  Not a big deal.
   *
   * GLSLang really likes to place decorations in the most interior
   * thing it possibly can.  In particular, if you have a struct, it
   * will place the patch decorations on the struct members.  This
   * should be handled by the variable splitting below just fine.
   *
   * If you have an array-of-struct, things get even more weird as it
   * will place the patch decorations on the struct even though it's
   * inside an array and some of the members being patch and others not
   * makes no sense whatsoever.  Since the only sensible thing is for
   * it to be all or nothing, we'll call it patch if any of the members
   * are declared patch.
   */
   vtn_foreach_decoration(b, val, gather_var_kind_cb, var);
   if (glsl_type_is_array(var->type->type) &&
      glsl_type_is_struct_or_ifc(without_array->type)) {
   vtn_foreach_decoration(b, vtn_value(b, without_array->id,
                     struct vtn_type *per_vertex_type = var->type;
   if (nir_is_arrayed_io(var->var, b->shader->info.stage))
            /* Figure out the interface block type. */
   struct vtn_type *iface_type = per_vertex_type;
   if (var->mode == vtn_variable_mode_output &&
      (b->shader->info.stage == MESA_SHADER_VERTEX ||
   b->shader->info.stage == MESA_SHADER_TESS_EVAL ||
   b->shader->info.stage == MESA_SHADER_GEOMETRY)) {
   /* For vertex data outputs, we can end up with arrays of blocks for
   * transform feedback where each array element corresponds to a
   * different XFB output buffer.
   */
   while (iface_type->base_type == vtn_base_type_array)
      }
   if (iface_type->base_type == vtn_base_type_struct && iface_type->block)
                  /* If it's a block, set it up as per-member so can be splitted later by
   * nir_split_per_member_structs.
   *
   * This is for a couple of reasons.  For one, builtins may all come in a
   * block and we really want those split out into separate variables.
   * For another, interpolation qualifiers can be applied to members of
   * the top-level struct and we need to be able to preserve that
   * information.
   */
   if (per_vertex_type->base_type == vtn_base_type_struct &&
      per_vertex_type->block) {
   var->var->num_members = glsl_get_length(per_vertex_type->type);
                  for (unsigned i = 0; i < var->var->num_members; i++) {
      var->var->members[i].mode = nir_mode;
   var->var->members[i].patch = var->var->data.patch;
                  /* For inputs and outputs, we need to grab locations and builtin
   * information from the per-vertex type.
   */
   vtn_foreach_decoration(b, vtn_value(b, per_vertex_type->id,
                              case vtn_variable_mode_phys_ssbo:
   case vtn_variable_mode_generic:
                  /* Ignore incorrectly generated Undef initializers. */
   if (b->wa_llvm_spirv_ignore_workgroup_initializer &&
      initializer &&
   storage_class == SpvStorageClassWorkgroup)
         /* Only initialize variable when there is an initializer and it's not
   * undef.
   */
   if (initializer && !initializer->is_undef_constant) {
      switch (storage_class) {
   case SpvStorageClassWorkgroup:
      /* VK_KHR_zero_initialize_workgroup_memory. */
   vtn_fail_if(b->options->environment != NIR_SPIRV_VULKAN,
               "Only Vulkan supports variable initializer "
   vtn_fail_if(initializer->value_type != vtn_value_type_constant ||
               !initializer->is_null_constant,
   "Workgroup variable %u can only have OpConstantNull "
   "as initializer, but have %u instead",
               case SpvStorageClassUniformConstant:
      vtn_fail_if(b->options->environment != NIR_SPIRV_OPENGL &&
               b->options->environment != NIR_SPIRV_OPENCL,
   "Only OpenGL and OpenCL support variable initializer "
   vtn_fail_if(initializer->value_type != vtn_value_type_constant,
               "UniformConstant variable %u can only have a constant "
               case SpvStorageClassOutput:
   case SpvStorageClassPrivate:
      vtn_assert(b->options->environment != NIR_SPIRV_OPENCL);
               case SpvStorageClassFunction:
                  case SpvStorageClassCrossWorkgroup:
      vtn_assert(b->options->environment == NIR_SPIRV_OPENCL);
   vtn_fail("Initializer for CrossWorkgroup variable %u "
                     default: {
      const enum nir_spirv_execution_environment env =
         const char *env_name =
      env == NIR_SPIRV_VULKAN ? "Vulkan" :
   env == NIR_SPIRV_OPENCL ? "OpenCL" :
   env == NIR_SPIRV_OPENGL ? "OpenGL" :
      vtn_assert(env_name);
   vtn_fail("In %s, any OpVariable with an Initializer operand "
            "must have %s%s%s, or Function as "
   "its Storage Class operand.  Variable %u has an "
   "Initializer but its Storage Class is %s.",
   env_name,
   env == NIR_SPIRV_VULKAN ? "Private, Output, Workgroup" : "",
   env == NIR_SPIRV_OPENCL ? "CrossWorkgroup, UniformConstant" : "",
   env == NIR_SPIRV_OPENGL ? "Private, Output, UniformConstant" : "",
                  switch (initializer->value_type) {
   case vtn_value_type_constant:
      var->var->constant_initializer =
            case vtn_value_type_pointer:
      var->var->pointer_initializer = initializer->pointer->var->var;
      default:
      vtn_fail("SPIR-V variable initializer %u must be constant or pointer",
                  if (var->mode == vtn_variable_mode_uniform ||
      var->mode == vtn_variable_mode_image ||
   var->mode == vtn_variable_mode_ssbo) {
   /* SSBOs and images are assumed to not alias in the Simple, GLSL and Vulkan memory models */
               vtn_foreach_decoration(b, val, var_decoration_cb, var);
            /* Propagate access flags from the OpVariable decorations. */
            if ((var->mode == vtn_variable_mode_input ||
      var->mode == vtn_variable_mode_output) &&
   var->var->members) {
               if (var->mode == vtn_variable_mode_uniform ||
      var->mode == vtn_variable_mode_image ||
   var->mode == vtn_variable_mode_ubo ||
   var->mode == vtn_variable_mode_ssbo ||
   var->mode == vtn_variable_mode_atomic_counter) {
   /* XXX: We still need the binding information in the nir_variable
   * for these. We should fix that.
   */
   var->var->data.binding = var->binding;
   var->var->data.explicit_binding = var->explicit_binding;
   var->var->data.descriptor_set = var->descriptor_set;
   var->var->data.index = var->input_attachment_index;
            if (glsl_type_is_image(glsl_without_array(var->var->type)))
               if (var->mode == vtn_variable_mode_function) {
      vtn_assert(var->var != NULL && var->var->members == NULL);
      } else if (var->var) {
         } else {
      vtn_assert(vtn_pointer_is_external_block(b, val->pointer) ||
               }
      static void
   vtn_assert_types_equal(struct vtn_builder *b, SpvOp opcode,
               {
      if (dst_type->id == src_type->id)
            if (vtn_types_compatible(b, dst_type, src_type)) {
      /* Early versions of GLSLang would re-emit types unnecessarily and you
   * would end up with OpLoad, OpStore, or OpCopyMemory opcodes which have
   * mismatched source and destination types.
   *
   * https://github.com/KhronosGroup/glslang/issues/304
   * https://github.com/KhronosGroup/glslang/issues/307
   * https://bugs.freedesktop.org/show_bug.cgi?id=104338
   * https://bugs.freedesktop.org/show_bug.cgi?id=104424
   */
   vtn_warn("Source and destination types of %s do not have the same "
                           vtn_fail("Source and destination types of %s do not match: %s (%%%u) vs. %s (%%%u)",
            spirv_op_to_string(opcode),
   }
      static nir_def *
   nir_shrink_zero_pad_vec(nir_builder *b, nir_def *val,
         {
      if (val->num_components == num_components)
            nir_def *comps[NIR_MAX_VEC_COMPONENTS];
   for (unsigned i = 0; i < num_components; i++) {
      if (i < val->num_components)
         else
      }
      }
      static nir_def *
   nir_sloppy_bitcast(nir_builder *b, nir_def *val,
         {
      const unsigned num_components = glsl_get_vector_elements(type);
            /* First, zero-pad to ensure that the value is big enough that when we
   * bit-cast it, we don't loose anything.
   */
   if (val->bit_size < bit_size) {
      const unsigned src_num_components_needed =
                                 }
      bool
   vtn_get_mem_operands(struct vtn_builder *b, const uint32_t *w, unsigned count,
               {
      *access = 0;
   *alignment = 0;
   if (*idx >= count)
            *access = w[(*idx)++];
   if (*access & SpvMemoryAccessAlignedMask) {
      vtn_assert(*idx < count);
               if (*access & SpvMemoryAccessMakePointerAvailableMask) {
      vtn_assert(*idx < count);
   vtn_assert(dest_scope);
               if (*access & SpvMemoryAccessMakePointerVisibleMask) {
      vtn_assert(*idx < count);
   vtn_assert(src_scope);
                  }
      static enum gl_access_qualifier
   spv_access_to_gl_access(SpvMemoryAccessMask access)
   {
               if (access & SpvMemoryAccessVolatileMask)
         if (access & SpvMemoryAccessNontemporalMask)
               }
         SpvMemorySemanticsMask
   vtn_mode_to_memory_semantics(enum vtn_variable_mode mode)
   {
      switch (mode) {
   case vtn_variable_mode_ssbo:
   case vtn_variable_mode_phys_ssbo:
         case vtn_variable_mode_workgroup:
         case vtn_variable_mode_cross_workgroup:
         case vtn_variable_mode_atomic_counter:
         case vtn_variable_mode_image:
         case vtn_variable_mode_output:
         default:
            }
      void
   vtn_emit_make_visible_barrier(struct vtn_builder *b, SpvMemoryAccessMask access,
         {
      if (!(access & SpvMemoryAccessMakePointerVisibleMask))
            vtn_emit_memory_barrier(b, scope, SpvMemorySemanticsMakeVisibleMask |
            }
      void
   vtn_emit_make_available_barrier(struct vtn_builder *b, SpvMemoryAccessMask access,
         {
      if (!(access & SpvMemoryAccessMakePointerAvailableMask))
            vtn_emit_memory_barrier(b, scope, SpvMemorySemanticsMakeAvailableMask |
            }
      static void
   ptr_nonuniform_workaround_cb(struct vtn_builder *b, struct vtn_value *val,
         {
               switch (dec->decoration) {
   case SpvDecorationNonUniformEXT:
      *access |= ACCESS_NON_UNIFORM;
         default:
            }
      void
   vtn_handle_variables(struct vtn_builder *b, SpvOp opcode,
         {
      switch (opcode) {
   case SpvOpUndef: {
      struct vtn_value *val = vtn_push_value(b, w[2], vtn_value_type_undef);
   val->type = vtn_get_type(b, w[1]);
   val->is_undef_constant = true;
               case SpvOpVariable: {
                        const bool is_global = storage_class != SpvStorageClassFunction;
   const bool is_io = storage_class == SpvStorageClassInput ||
            /* Skip global variables that are not used by the entrypoint.  Before
   * SPIR-V 1.4 the interface is only used for I/O variables, so extra
   * variables will still need to be removed later.
   */
   if (!b->options->create_library &&
      (is_io || (b->version >= 0x10400 && is_global))) {
   if (!bsearch(&w[2], b->interface_ids, b->interface_ids_count, 4, cmp_uint32_t))
               struct vtn_value *val = vtn_push_value(b, w[2], vtn_value_type_pointer);
                                 case SpvOpConstantSampler: {
      /* Synthesize a pointer-to-sampler type, create a variable of that type,
   * and give the variable a constant initializer with the sampler params */
   struct vtn_type *sampler_type = vtn_value(b, w[1], vtn_value_type_type)->type;
            struct vtn_type *ptr_type = rzalloc(b, struct vtn_type);
   ptr_type->base_type = vtn_base_type_pointer;
   ptr_type->deref = sampler_type;
            ptr_type->type = nir_address_format_to_glsl_type(
                     nir_variable *nir_var = val->pointer->var->var;
   nir_var->data.sampler.is_inline_sampler = true;
   nir_var->data.sampler.addressing_mode = w[3];
   nir_var->data.sampler.normalized_coordinates = w[4];
                        case SpvOpAccessChain:
   case SpvOpPtrAccessChain:
   case SpvOpInBoundsAccessChain:
   case SpvOpInBoundsPtrAccessChain: {
      struct vtn_access_chain *chain = vtn_access_chain_create(b, count - 4);
   enum gl_access_qualifier access = 0;
            unsigned idx = 0;
   for (int i = 4; i < count; i++) {
      struct vtn_value *link_val = vtn_untyped_value(b, w[i]);
   if (link_val->value_type == vtn_value_type_constant) {
      chain->link[idx].mode = vtn_access_mode_literal;
      } else {
      chain->link[idx].mode = vtn_access_mode_id;
                                                                     /* Workaround for https://gitlab.freedesktop.org/mesa/mesa/-/issues/3406 */
            if (base->mode == vtn_variable_mode_ssbo && b->options->force_ssbo_non_uniform)
            struct vtn_pointer *ptr = vtn_pointer_dereference(b, base, chain);
   ptr->ptr_type = ptr_type;
   ptr->access |= access;
   vtn_push_pointer(b, w[2], ptr);
               case SpvOpCopyMemory: {
      struct vtn_value *dest_val = vtn_pointer_value(b, w[1]);
   struct vtn_value *src_val = vtn_pointer_value(b, w[2]);
   struct vtn_pointer *dest = vtn_value_to_pointer(b, dest_val);
            vtn_assert_types_equal(b, opcode, dest_val->type->deref,
            unsigned idx = 3, dest_alignment, src_alignment;
   SpvMemoryAccessMask dest_access, src_access;
   SpvScope dest_scope, src_scope;
   vtn_get_mem_operands(b, w, count, &idx, &dest_access, &dest_alignment,
         if (!vtn_get_mem_operands(b, w, count, &idx, &src_access, &src_alignment,
            src_alignment = dest_alignment;
      }
   src = vtn_align_pointer(b, src, src_alignment);
                     vtn_variable_copy(b, dest, src,
                  vtn_emit_make_available_barrier(b, dest_access, dest_scope, dest->mode);
               case SpvOpCopyMemorySized: {
      struct vtn_value *dest_val = vtn_pointer_value(b, w[1]);
   struct vtn_value *src_val = vtn_pointer_value(b, w[2]);
   nir_def *size = vtn_get_nir_ssa(b, w[3]);
   struct vtn_pointer *dest = vtn_value_to_pointer(b, dest_val);
            unsigned idx = 4, dest_alignment, src_alignment;
   SpvMemoryAccessMask dest_access, src_access;
   SpvScope dest_scope, src_scope;
   vtn_get_mem_operands(b, w, count, &idx, &dest_access, &dest_alignment,
         if (!vtn_get_mem_operands(b, w, count, &idx, &src_access, &src_alignment,
            src_alignment = dest_alignment;
      }
   src = vtn_align_pointer(b, src, src_alignment);
                     nir_memcpy_deref_with_access(&b->nb,
                                    vtn_emit_make_available_barrier(b, dest_access, dest_scope, dest->mode);
               case SpvOpLoad: {
      struct vtn_type *res_type = vtn_get_type(b, w[1]);
   struct vtn_value *src_val = vtn_value(b, w[3], vtn_value_type_pointer);
                     unsigned idx = 4, alignment;
   SpvMemoryAccessMask access;
   SpvScope scope;
   vtn_get_mem_operands(b, w, count, &idx, &access, &alignment, NULL, &scope);
                     vtn_push_ssa_value(b, w[2], vtn_variable_load(b, src, spv_access_to_gl_access(access)));
               case SpvOpStore: {
      struct vtn_value *dest_val = vtn_pointer_value(b, w[1]);
   struct vtn_pointer *dest = vtn_value_to_pointer(b, dest_val);
            /* OpStore requires us to actually have a storage type */
   vtn_fail_if(dest->type->type == NULL,
            if (glsl_get_base_type(dest->type->type) == GLSL_TYPE_BOOL &&
      glsl_get_base_type(src_val->type->type) == GLSL_TYPE_UINT) {
   /* Early versions of GLSLang would use uint types for UBOs/SSBOs but
   * would then store them to a local variable as bool.  Work around
   * the issue by doing an implicit conversion.
   *
   * https://github.com/KhronosGroup/glslang/issues/170
   * https://bugs.freedesktop.org/show_bug.cgi?id=104424
   */
   vtn_warn("OpStore of value of type OpTypeInt to a pointer to type "
               struct vtn_ssa_value *bool_ssa =
         bool_ssa->def = nir_i2b(&b->nb, vtn_ssa_value(b, w[2])->def);
   vtn_variable_store(b, bool_ssa, dest, 0);
                        unsigned idx = 3, alignment;
   SpvMemoryAccessMask access;
   SpvScope scope;
   vtn_get_mem_operands(b, w, count, &idx, &access, &alignment, &scope, NULL);
            struct vtn_ssa_value *src = vtn_ssa_value(b, w[2]);
            vtn_emit_make_available_barrier(b, access, scope, dest->mode);
               case SpvOpArrayLength: {
      struct vtn_pointer *ptr = vtn_pointer(b, w[3]);
            vtn_fail_if(ptr->type->base_type != vtn_base_type_struct,
         vtn_fail_if(field != ptr->type->length - 1 ||
                        struct vtn_access_chain chain = {
      .length = 1,
   .link = {
            };
            nir_def *array_length =
      nir_deref_buffer_array_length(&b->nb, 32,
               vtn_push_nir_ssa(b, w[2], array_length);
               case SpvOpConvertPtrToU: {
      struct vtn_type *u_type = vtn_get_type(b, w[1]);
            vtn_fail_if(ptr_type->base_type != vtn_base_type_pointer ||
                  vtn_fail_if(u_type->base_type != vtn_base_type_vector &&
                        /* The pointer will be converted to an SSA value automatically */
   nir_def *ptr = vtn_get_nir_ssa(b, w[3]);
   nir_def *u = nir_sloppy_bitcast(&b->nb, ptr, u_type->type);
   vtn_push_nir_ssa(b, w[2], u);
               case SpvOpConvertUToPtr: {
      struct vtn_type *ptr_type = vtn_get_type(b, w[1]);
            vtn_fail_if(ptr_type->base_type != vtn_base_type_pointer ||
                  vtn_fail_if(u_type->base_type != vtn_base_type_vector &&
                        nir_def *u = vtn_get_nir_ssa(b, w[3]);
   nir_def *ptr = nir_sloppy_bitcast(&b->nb, u, ptr_type->type);
   vtn_push_pointer(b, w[2], vtn_pointer_from_ssa(b, ptr, ptr_type));
               case SpvOpGenericCastToPtrExplicit: {
      struct vtn_type *dst_type = vtn_get_type(b, w[1]);
   struct vtn_type *src_type = vtn_get_value_type(b, w[3]);
            vtn_fail_if(dst_type->base_type != vtn_base_type_pointer ||
               dst_type->storage_class != storage_class,
            vtn_fail_if(src_type->base_type != vtn_base_type_pointer ||
               src_type->deref->id != dst_type->deref->id,
            vtn_fail_if(src_type->storage_class != SpvStorageClassGeneric,
                  vtn_fail_if(storage_class != SpvStorageClassWorkgroup &&
               storage_class != SpvStorageClassCrossWorkgroup &&
                     nir_variable_mode nir_mode;
   enum vtn_variable_mode mode =
                  nir_def *null_value =
      nir_build_imm(&b->nb, nir_address_format_num_components(addr_format),
               nir_def *valid = nir_build_deref_mode_is(&b->nb, 1, &src_deref->def, nir_mode);
   vtn_push_nir_ssa(b, w[2], nir_bcsel(&b->nb, valid,
                           case SpvOpGenericPtrMemSemantics: {
      struct vtn_type *dst_type = vtn_get_type(b, w[1]);
            vtn_fail_if(dst_type->base_type != vtn_base_type_scalar ||
                        vtn_fail_if(src_type->base_type != vtn_base_type_pointer ||
                                 nir_def *global_bit =
      nir_bcsel(&b->nb, nir_build_deref_mode_is(&b->nb, 1, &src_deref->def,
                     nir_def *shared_bit =
      nir_bcsel(&b->nb, nir_build_deref_mode_is(&b->nb, 1, &src_deref->def,
                     vtn_push_nir_ssa(b, w[2], nir_iand(&b->nb, global_bit, shared_bit));
               case SpvOpSubgroupBlockReadINTEL: {
      struct vtn_type *res_type = vtn_get_type(b, w[1]);
            nir_intrinsic_instr *load =
      nir_intrinsic_instr_create(b->nb.shader,
      load->src[0] = nir_src_for_ssa(&src->def);
   nir_def_init_for_type(&load->instr, &load->def, res_type->type);
   load->num_components = load->def.num_components;
            vtn_push_nir_ssa(b, w[2], &load->def);
               case SpvOpSubgroupBlockWriteINTEL: {
      nir_deref_instr *dest = vtn_nir_deref(b, w[1]);
            nir_intrinsic_instr *store =
      nir_intrinsic_instr_create(b->nb.shader,
      store->src[0] = nir_src_for_ssa(&dest->def);
   store->src[1] = nir_src_for_ssa(data);
   store->num_components = data->num_components;
   nir_builder_instr_insert(&b->nb, &store->instr);
               case SpvOpConvertUToAccelerationStructureKHR: {
      struct vtn_type *as_type = vtn_get_type(b, w[1]);
   struct vtn_type *u_type = vtn_get_value_type(b, w[3]);
   vtn_fail_if(!((u_type->base_type == vtn_base_type_vector &&
                     (u_type->base_type == vtn_base_type_scalar &&
         "OpConvertUToAccelerationStructure may only be used to "
   vtn_fail_if(as_type->base_type != vtn_base_type_accel_struct,
                  nir_def *u = vtn_get_nir_ssa(b, w[3]);
   vtn_push_nir_ssa(b, w[2], nir_sloppy_bitcast(&b->nb, u, as_type->type));
               default:
            }
