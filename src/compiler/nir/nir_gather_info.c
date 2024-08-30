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
      #include "main/menums.h"
   #include "nir.h"
   #include "nir_deref.h"
      #include "util/set.h"
      static bool
   src_is_invocation_id(const nir_src *src)
   {
      nir_scalar s = nir_scalar_resolved(src->ssa, 0);
   return nir_scalar_is_intrinsic(s) &&
      }
      static bool
   src_is_local_invocation_index(nir_shader *shader, const nir_src *src)
   {
               nir_scalar s = nir_scalar_resolved(src->ssa, 0);
   if (!nir_scalar_is_intrinsic(s))
            const nir_intrinsic_op op = nir_scalar_intrinsic_op(s);
   if (op == nir_intrinsic_load_local_invocation_index)
         if (op != nir_intrinsic_load_local_invocation_id)
            unsigned nz_ids = 0;
   for (unsigned i = 0; i < 3; i++)
               }
      static void
   get_deref_info(nir_shader *shader, nir_variable *var, nir_deref_instr *deref,
         {
      *cross_invocation = false;
                     nir_deref_path path;
   nir_deref_path_init(&path, deref, NULL);
   assert(path.path[0]->deref_type == nir_deref_type_var);
            /* Vertex index is the outermost array index. */
   if (is_arrayed) {
      assert((*p)->deref_type == nir_deref_type_array);
   if (shader->info.stage == MESA_SHADER_TESS_CTRL)
         else if (shader->info.stage == MESA_SHADER_MESH)
                     /* We always lower indirect dereferences for "compact" array vars. */
   if (!path.path[0]->var->data.compact) {
      /* Non-compact array vars: find out if they are indirect. */
   for (; *p; p++) {
      if ((*p)->deref_type == nir_deref_type_array) {
         } else if ((*p)->deref_type == nir_deref_type_struct) {
         } else {
                           }
      static void
   set_io_mask(nir_shader *shader, nir_variable *var, int offset, int len,
         {
      for (int i = 0; i < len; i++) {
      /* Varyings might not have been assigned values yet so abort. */
   if (var->data.location == -1)
            int idx = var->data.location + offset + i;
   bool is_patch_generic = var->data.patch &&
                                    if (is_patch_generic) {
      /* Varyings might still have temp locations so abort */
                     } else {
      /* Varyings might still have temp locations so abort */
                              bool cross_invocation;
   bool indirect;
            if (var->data.mode == nir_var_shader_in) {
      if (is_patch_generic) {
      shader->info.patch_inputs_read |= bitfield;
   if (indirect)
      } else {
      shader->info.inputs_read |= bitfield;
   if (indirect)
                              if (shader->info.stage == MESA_SHADER_FRAGMENT) {
            } else {
      assert(var->data.mode == nir_var_shader_out);
   if (is_output_read) {
      if (is_patch_generic) {
      shader->info.patch_outputs_read |= bitfield;
   if (indirect)
      } else {
      shader->info.outputs_read |= bitfield;
                     if (cross_invocation && shader->info.stage == MESA_SHADER_TESS_CTRL)
      } else {
      if (is_patch_generic) {
      shader->info.patch_outputs_written |= bitfield;
   if (indirect)
      } else if (!var->data.read_only) {
      shader->info.outputs_written |= bitfield;
   if (indirect)
                                 if (var->data.fb_fetch_output) {
      shader->info.outputs_read |= bitfield;
   if (shader->info.stage == MESA_SHADER_FRAGMENT) {
      shader->info.fs.uses_fbfetch_output = true;
                  if (shader->info.stage == MESA_SHADER_FRAGMENT &&
      !is_output_read && var->data.index == 1)
         }
      /**
   * Mark an entire variable as used.  Caller must ensure that the variable
   * represents a shader input or output.
   */
   static void
   mark_whole_variable(nir_shader *shader, nir_variable *var,
         {
               if (nir_is_arrayed_io(var, shader->info.stage) ||
      /* For NV_mesh_shader. */
   (shader->info.stage == MESA_SHADER_MESH &&
   var->data.location == VARYING_SLOT_PRIMITIVE_INDICES &&
   !var->data.per_primitive)) {
   assert(glsl_type_is_array(type));
               if (var->data.per_view) {
      assert(glsl_type_is_array(type));
               const unsigned slots = nir_variable_count_slots(var, type);
      }
      static unsigned
   get_io_offset(nir_deref_instr *deref, nir_variable *var, bool is_arrayed,
         {
      if (var->data.compact) {
      if (deref->deref_type == nir_deref_type_var) {
      assert(glsl_type_is_array(var->type));
      }
   assert(deref->deref_type == nir_deref_type_array);
                        for (nir_deref_instr *d = deref; d; d = nir_deref_instr_parent(d)) {
      if (d->deref_type == nir_deref_type_array) {
                                                   offset += glsl_count_attribute_slots(d->type, false) *
      } else if (d->deref_type == nir_deref_type_struct) {
      const struct glsl_type *parent_type = nir_deref_instr_parent(d)->type;
   for (unsigned i = 0; i < d->strct.index; i++) {
      const struct glsl_type *field_type = glsl_get_struct_field(parent_type, i);
                        }
      /**
   * Try to mark a portion of the given varying as used.  Caller must ensure
   * that the variable represents a shader input or output.
   *
   * If the index can't be interpreted as a constant, or some other problem
   * occurs, then nothing will be marked and false will be returned.
   */
   static bool
   try_mask_partial_io(nir_shader *shader, nir_variable *var,
         {
      const struct glsl_type *type = var->type;
   bool is_arrayed = nir_is_arrayed_io(var, shader->info.stage);
            if (is_arrayed) {
      assert(glsl_type_is_array(type));
               /* Per view variables will be considered as a whole. */
   if (var->data.per_view)
            unsigned offset = get_io_offset(deref, var, is_arrayed, skip_non_arrayed);
   if (offset == -1)
            const unsigned slots = nir_variable_count_slots(var, type);
   if (offset >= slots) {
      /* Constant index outside the bounds of the matrix/array.  This could
   * arise as a result of constant folding of a legal GLSL program.
   *
   * Even though the spec says that indexing outside the bounds of a
   * matrix/array results in undefined behaviour, we don't want to pass
   * out-of-range values to set_io_mask() (since this could result in
   * slots that don't exist being marked as used), so just let the caller
   * mark the whole variable as used.
   */
               unsigned len = nir_deref_count_slots(deref, var);
   set_io_mask(shader, var, offset, len, deref, is_output_read);
      }
      /** Returns true if the given intrinsic writes external memory
   *
   * Only returns true for writes to globally visible memory, not scratch and
   * not shared.
   */
   bool
   nir_intrinsic_writes_external_memory(const nir_intrinsic_instr *instr)
   {
      switch (instr->intrinsic) {
   case nir_intrinsic_atomic_counter_inc:
   case nir_intrinsic_atomic_counter_inc_deref:
   case nir_intrinsic_atomic_counter_add:
   case nir_intrinsic_atomic_counter_add_deref:
   case nir_intrinsic_atomic_counter_pre_dec:
   case nir_intrinsic_atomic_counter_pre_dec_deref:
   case nir_intrinsic_atomic_counter_post_dec:
   case nir_intrinsic_atomic_counter_post_dec_deref:
   case nir_intrinsic_atomic_counter_min:
   case nir_intrinsic_atomic_counter_min_deref:
   case nir_intrinsic_atomic_counter_max:
   case nir_intrinsic_atomic_counter_max_deref:
   case nir_intrinsic_atomic_counter_and:
   case nir_intrinsic_atomic_counter_and_deref:
   case nir_intrinsic_atomic_counter_or:
   case nir_intrinsic_atomic_counter_or_deref:
   case nir_intrinsic_atomic_counter_xor:
   case nir_intrinsic_atomic_counter_xor_deref:
   case nir_intrinsic_atomic_counter_exchange:
   case nir_intrinsic_atomic_counter_exchange_deref:
   case nir_intrinsic_atomic_counter_comp_swap:
   case nir_intrinsic_atomic_counter_comp_swap_deref:
   case nir_intrinsic_bindless_image_atomic:
   case nir_intrinsic_bindless_image_atomic_swap:
   case nir_intrinsic_bindless_image_store:
   case nir_intrinsic_bindless_image_store_raw_intel:
   case nir_intrinsic_global_atomic:
   case nir_intrinsic_global_atomic_swap:
   case nir_intrinsic_global_atomic_ir3:
   case nir_intrinsic_global_atomic_swap_ir3:
   case nir_intrinsic_image_atomic:
   case nir_intrinsic_image_atomic_swap:
   case nir_intrinsic_image_deref_atomic:
   case nir_intrinsic_image_deref_atomic_swap:
   case nir_intrinsic_image_deref_store:
   case nir_intrinsic_image_deref_store_raw_intel:
   case nir_intrinsic_image_store:
   case nir_intrinsic_image_store_raw_intel:
   case nir_intrinsic_ssbo_atomic:
   case nir_intrinsic_ssbo_atomic_swap:
   case nir_intrinsic_ssbo_atomic_ir3:
   case nir_intrinsic_ssbo_atomic_swap_ir3:
   case nir_intrinsic_store_global:
   case nir_intrinsic_store_global_ir3:
   case nir_intrinsic_store_global_amd:
   case nir_intrinsic_store_ssbo:
   case nir_intrinsic_store_ssbo_ir3:
            case nir_intrinsic_store_deref:
   case nir_intrinsic_deref_atomic:
   case nir_intrinsic_deref_atomic_swap:
      return nir_deref_mode_may_be(nir_src_as_deref(instr->src[0]),
         default:
            }
      static bool
   intrinsic_is_bindless(nir_intrinsic_instr *instr)
   {
      switch (instr->intrinsic) {
   case nir_intrinsic_bindless_image_atomic:
   case nir_intrinsic_bindless_image_atomic_swap:
   case nir_intrinsic_bindless_image_descriptor_amd:
   case nir_intrinsic_bindless_image_format:
   case nir_intrinsic_bindless_image_load:
   case nir_intrinsic_bindless_image_load_raw_intel:
   case nir_intrinsic_bindless_image_order:
   case nir_intrinsic_bindless_image_samples:
   case nir_intrinsic_bindless_image_samples_identical:
   case nir_intrinsic_bindless_image_size:
   case nir_intrinsic_bindless_image_sparse_load:
   case nir_intrinsic_bindless_image_store:
   case nir_intrinsic_bindless_image_store_raw_intel:
   case nir_intrinsic_bindless_resource_ir3:
         default:
         }
      }
      static void
   gather_intrinsic_info(nir_intrinsic_instr *instr, nir_shader *shader,
         {
      uint64_t slot_mask = 0;
   uint16_t slot_mask_16bit = 0;
            if (nir_intrinsic_infos[instr->intrinsic].index_map[NIR_INTRINSIC_IO_SEMANTICS] > 0) {
               is_patch_special = semantics.location == VARYING_SLOT_TESS_LEVEL_INNER ||
                        if (semantics.location >= VARYING_SLOT_PATCH0 &&
      semantics.location <= VARYING_SLOT_PATCH31) {
   /* Generic per-patch I/O. */
   assert((shader->info.stage == MESA_SHADER_TESS_EVAL &&
         instr->intrinsic == nir_intrinsic_load_input) ||
                              if (semantics.location >= VARYING_SLOT_VAR0_16BIT &&
      semantics.location <= VARYING_SLOT_VAR15_16BIT) {
   /* Convert num_slots from the units of half vectors to full vectors. */
   unsigned num_slots = (semantics.num_slots + semantics.high_16bits + 1) / 2;
   slot_mask_16bit =
      } else {
      slot_mask = BITFIELD64_RANGE(semantics.location, semantics.num_slots);
                  switch (instr->intrinsic) {
   case nir_intrinsic_demote:
   case nir_intrinsic_demote_if:
      shader->info.fs.uses_demote = true;
      case nir_intrinsic_discard:
   case nir_intrinsic_discard_if:
   case nir_intrinsic_terminate:
   case nir_intrinsic_terminate_if:
      /* Freedreno uses discard_if() to end GS invocations that don't produce
   * a vertex and RADV uses terminate() to end ray-tracing shaders,
   * so only set uses_discard for fragment shaders.
   */
   if (shader->info.stage == MESA_SHADER_FRAGMENT)
               case nir_intrinsic_interp_deref_at_centroid:
   case nir_intrinsic_interp_deref_at_sample:
   case nir_intrinsic_interp_deref_at_offset:
   case nir_intrinsic_interp_deref_at_vertex:
   case nir_intrinsic_load_deref:
   case nir_intrinsic_store_deref:
   case nir_intrinsic_copy_deref: {
      nir_deref_instr *deref = nir_src_as_deref(instr->src[0]);
   if (nir_deref_mode_is_one_of(deref, nir_var_shader_in |
            nir_variable *var = nir_deref_instr_get_variable(deref);
   bool is_output_read = false;
   if (var->data.mode == nir_var_shader_out &&
                                 /* We need to track which input_reads bits correspond to a
   * dvec3/dvec4 input attribute */
   if (shader->info.stage == MESA_SHADER_VERTEX &&
      var->data.mode == nir_var_shader_in &&
   glsl_type_is_dual_slot(glsl_without_array(var->type))) {
   for (unsigned i = 0; i < glsl_count_attribute_slots(var->type, false); i++) {
      int idx = var->data.location + i;
            }
   if (nir_intrinsic_writes_external_memory(instr))
            }
   case nir_intrinsic_image_deref_load: {
      nir_deref_instr *deref = nir_src_as_deref(instr->src[0]);
   nir_variable *var = nir_deref_instr_get_variable(deref);
   enum glsl_sampler_dim dim = glsl_get_sampler_dim(glsl_without_array(var->type));
   if (dim != GLSL_SAMPLER_DIM_SUBPASS &&
                  var->data.fb_fetch_output = true;
   shader->info.fs.uses_fbfetch_output = true;
               case nir_intrinsic_bindless_image_load: {
      enum glsl_sampler_dim dim = nir_intrinsic_image_dim(instr);
   if (dim != GLSL_SAMPLER_DIM_SUBPASS &&
      dim != GLSL_SAMPLER_DIM_SUBPASS_MS)
      shader->info.fs.uses_fbfetch_output = true;
               case nir_intrinsic_load_input:
   case nir_intrinsic_load_per_vertex_input:
   case nir_intrinsic_load_input_vertex:
   case nir_intrinsic_load_interpolated_input:
      if (shader->info.stage == MESA_SHADER_TESS_EVAL &&
      instr->intrinsic == nir_intrinsic_load_input &&
   !is_patch_special) {
   shader->info.patch_inputs_read |= slot_mask;
   if (!nir_src_is_const(*nir_get_io_offset_src(instr)))
      } else {
      shader->info.inputs_read |= slot_mask;
   if (nir_intrinsic_io_semantics(instr).high_dvec2)
         shader->info.inputs_read_16bit |= slot_mask_16bit;
   if (!nir_src_is_const(*nir_get_io_offset_src(instr))) {
      shader->info.inputs_read_indirectly |= slot_mask;
                  if (shader->info.stage == MESA_SHADER_TESS_CTRL &&
      instr->intrinsic == nir_intrinsic_load_per_vertex_input &&
   !src_is_invocation_id(nir_get_io_arrayed_index_src(instr)))
            case nir_intrinsic_load_output:
   case nir_intrinsic_load_per_vertex_output:
   case nir_intrinsic_load_per_primitive_output:
      if (shader->info.stage == MESA_SHADER_TESS_CTRL &&
      instr->intrinsic == nir_intrinsic_load_output &&
   !is_patch_special) {
   shader->info.patch_outputs_read |= slot_mask;
   if (!nir_src_is_const(*nir_get_io_offset_src(instr)))
      } else {
      shader->info.outputs_read |= slot_mask;
   shader->info.outputs_read_16bit |= slot_mask_16bit;
   if (!nir_src_is_const(*nir_get_io_offset_src(instr))) {
      shader->info.outputs_accessed_indirectly |= slot_mask;
                  if (shader->info.stage == MESA_SHADER_TESS_CTRL &&
      instr->intrinsic == nir_intrinsic_load_per_vertex_output &&
               /* NV_mesh_shader: mesh shaders can load their outputs. */
   if (shader->info.stage == MESA_SHADER_MESH &&
      (instr->intrinsic == nir_intrinsic_load_per_vertex_output ||
   instr->intrinsic == nir_intrinsic_load_per_primitive_output) &&
               if (shader->info.stage == MESA_SHADER_FRAGMENT &&
      nir_intrinsic_io_semantics(instr).fb_fetch_output)
            case nir_intrinsic_store_output:
   case nir_intrinsic_store_per_vertex_output:
   case nir_intrinsic_store_per_primitive_output:
      if (shader->info.stage == MESA_SHADER_TESS_CTRL &&
      instr->intrinsic == nir_intrinsic_store_output &&
   !is_patch_special) {
   shader->info.patch_outputs_written |= slot_mask;
   if (!nir_src_is_const(*nir_get_io_offset_src(instr)))
      } else {
      shader->info.outputs_written |= slot_mask;
   shader->info.outputs_written_16bit |= slot_mask_16bit;
   if (!nir_src_is_const(*nir_get_io_offset_src(instr))) {
      shader->info.outputs_accessed_indirectly |= slot_mask;
                  if (shader->info.stage == MESA_SHADER_MESH &&
      (instr->intrinsic == nir_intrinsic_store_per_vertex_output ||
   instr->intrinsic == nir_intrinsic_store_per_primitive_output) &&
               if (shader->info.stage == MESA_SHADER_FRAGMENT &&
      nir_intrinsic_io_semantics(instr).dual_source_blend_index)
            case nir_intrinsic_load_color0:
   case nir_intrinsic_load_color1:
      shader->info.inputs_read |=
            case nir_intrinsic_load_subgroup_size:
   case nir_intrinsic_load_subgroup_invocation:
   case nir_intrinsic_load_subgroup_eq_mask:
   case nir_intrinsic_load_subgroup_ge_mask:
   case nir_intrinsic_load_subgroup_gt_mask:
   case nir_intrinsic_load_subgroup_le_mask:
   case nir_intrinsic_load_subgroup_lt_mask:
   case nir_intrinsic_load_num_subgroups:
   case nir_intrinsic_load_subgroup_id:
   case nir_intrinsic_load_vertex_id:
   case nir_intrinsic_load_instance_id:
   case nir_intrinsic_load_vertex_id_zero_base:
   case nir_intrinsic_load_base_vertex:
   case nir_intrinsic_load_first_vertex:
   case nir_intrinsic_load_is_indexed_draw:
   case nir_intrinsic_load_base_instance:
   case nir_intrinsic_load_draw_id:
   case nir_intrinsic_load_invocation_id:
   case nir_intrinsic_load_frag_coord:
   case nir_intrinsic_load_frag_shading_rate:
   case nir_intrinsic_load_fully_covered:
   case nir_intrinsic_load_point_coord:
   case nir_intrinsic_load_line_coord:
   case nir_intrinsic_load_front_face:
   case nir_intrinsic_load_sample_id:
   case nir_intrinsic_load_sample_pos:
   case nir_intrinsic_load_sample_pos_or_center:
   case nir_intrinsic_load_sample_mask_in:
   case nir_intrinsic_load_helper_invocation:
   case nir_intrinsic_load_tess_coord:
   case nir_intrinsic_load_tess_coord_xy:
   case nir_intrinsic_load_patch_vertices_in:
   case nir_intrinsic_load_primitive_id:
   case nir_intrinsic_load_tess_level_outer:
   case nir_intrinsic_load_tess_level_inner:
   case nir_intrinsic_load_tess_level_outer_default:
   case nir_intrinsic_load_tess_level_inner_default:
   case nir_intrinsic_load_local_invocation_id:
   case nir_intrinsic_load_local_invocation_index:
   case nir_intrinsic_load_global_invocation_id:
   case nir_intrinsic_load_base_global_invocation_id:
   case nir_intrinsic_load_global_invocation_index:
   case nir_intrinsic_load_workgroup_id:
   case nir_intrinsic_load_workgroup_index:
   case nir_intrinsic_load_num_workgroups:
   case nir_intrinsic_load_workgroup_size:
   case nir_intrinsic_load_work_dim:
   case nir_intrinsic_load_user_data_amd:
   case nir_intrinsic_load_view_index:
   case nir_intrinsic_load_barycentric_model:
   case nir_intrinsic_load_ray_launch_id:
   case nir_intrinsic_load_ray_launch_size:
   case nir_intrinsic_load_ray_launch_size_addr_amd:
   case nir_intrinsic_load_ray_world_origin:
   case nir_intrinsic_load_ray_world_direction:
   case nir_intrinsic_load_ray_object_origin:
   case nir_intrinsic_load_ray_object_direction:
   case nir_intrinsic_load_ray_t_min:
   case nir_intrinsic_load_ray_t_max:
   case nir_intrinsic_load_ray_object_to_world:
   case nir_intrinsic_load_ray_world_to_object:
   case nir_intrinsic_load_ray_hit_kind:
   case nir_intrinsic_load_ray_flags:
   case nir_intrinsic_load_ray_geometry_index:
   case nir_intrinsic_load_ray_instance_custom_index:
   case nir_intrinsic_load_mesh_view_count:
   case nir_intrinsic_load_gs_header_ir3:
   case nir_intrinsic_load_tcs_header_ir3:
   case nir_intrinsic_load_ray_triangle_vertex_positions:
      BITSET_SET(shader->info.system_values_read,
               case nir_intrinsic_load_barycentric_pixel:
      if (nir_intrinsic_interp_mode(instr) == INTERP_MODE_SMOOTH ||
      nir_intrinsic_interp_mode(instr) == INTERP_MODE_NONE) {
   BITSET_SET(shader->info.system_values_read,
      } else if (nir_intrinsic_interp_mode(instr) == INTERP_MODE_NOPERSPECTIVE) {
      BITSET_SET(shader->info.system_values_read,
      }
         case nir_intrinsic_load_barycentric_centroid:
      if (nir_intrinsic_interp_mode(instr) == INTERP_MODE_SMOOTH ||
      nir_intrinsic_interp_mode(instr) == INTERP_MODE_NONE) {
   BITSET_SET(shader->info.system_values_read,
      } else if (nir_intrinsic_interp_mode(instr) == INTERP_MODE_NOPERSPECTIVE) {
      BITSET_SET(shader->info.system_values_read,
      }
         case nir_intrinsic_load_barycentric_sample:
      if (nir_intrinsic_interp_mode(instr) == INTERP_MODE_SMOOTH ||
      nir_intrinsic_interp_mode(instr) == INTERP_MODE_NONE) {
   BITSET_SET(shader->info.system_values_read,
      } else if (nir_intrinsic_interp_mode(instr) == INTERP_MODE_NOPERSPECTIVE) {
      BITSET_SET(shader->info.system_values_read,
      }
   if (shader->info.stage == MESA_SHADER_FRAGMENT)
               case nir_intrinsic_load_barycentric_coord_pixel:
   case nir_intrinsic_load_barycentric_coord_centroid:
   case nir_intrinsic_load_barycentric_coord_sample:
   case nir_intrinsic_load_barycentric_coord_at_offset:
   case nir_intrinsic_load_barycentric_coord_at_sample:
      if (nir_intrinsic_interp_mode(instr) == INTERP_MODE_SMOOTH ||
      nir_intrinsic_interp_mode(instr) == INTERP_MODE_NONE) {
      } else if (nir_intrinsic_interp_mode(instr) == INTERP_MODE_NOPERSPECTIVE) {
         }
         case nir_intrinsic_quad_broadcast:
   case nir_intrinsic_quad_swap_horizontal:
   case nir_intrinsic_quad_swap_vertical:
   case nir_intrinsic_quad_swap_diagonal:
   case nir_intrinsic_quad_swizzle_amd:
      if (shader->info.stage == MESA_SHADER_FRAGMENT)
               case nir_intrinsic_vote_any:
   case nir_intrinsic_vote_all:
   case nir_intrinsic_vote_feq:
   case nir_intrinsic_vote_ieq:
   case nir_intrinsic_ballot:
   case nir_intrinsic_ballot_bit_count_exclusive:
   case nir_intrinsic_ballot_bit_count_inclusive:
   case nir_intrinsic_ballot_bitfield_extract:
   case nir_intrinsic_ballot_bit_count_reduce:
   case nir_intrinsic_ballot_find_lsb:
   case nir_intrinsic_ballot_find_msb:
   case nir_intrinsic_first_invocation:
   case nir_intrinsic_read_invocation:
   case nir_intrinsic_read_first_invocation:
   case nir_intrinsic_elect:
   case nir_intrinsic_reduce:
   case nir_intrinsic_inclusive_scan:
   case nir_intrinsic_exclusive_scan:
   case nir_intrinsic_shuffle:
   case nir_intrinsic_shuffle_xor:
   case nir_intrinsic_shuffle_up:
   case nir_intrinsic_shuffle_down:
   case nir_intrinsic_write_invocation_amd:
      if (shader->info.stage == MESA_SHADER_FRAGMENT)
            shader->info.uses_wide_subgroup_intrinsics = true;
         case nir_intrinsic_end_primitive:
   case nir_intrinsic_end_primitive_with_counter:
   case nir_intrinsic_end_primitive_nv:
      assert(shader->info.stage == MESA_SHADER_GEOMETRY);
   shader->info.gs.uses_end_primitive = 1;
         case nir_intrinsic_emit_vertex:
   case nir_intrinsic_emit_vertex_with_counter:
   case nir_intrinsic_emit_vertex_nv:
                     case nir_intrinsic_barrier:
      shader->info.uses_control_barrier |=
            shader->info.uses_memory_barrier |=
               case nir_intrinsic_store_zs_agx:
      shader->info.outputs_written |= BITFIELD64_BIT(FRAG_RESULT_DEPTH) |
               case nir_intrinsic_sample_mask_agx:
      shader->info.outputs_written |= BITFIELD64_BIT(FRAG_RESULT_SAMPLE_MASK);
         case nir_intrinsic_discard_agx:
      shader->info.fs.uses_discard = true;
         case nir_intrinsic_launch_mesh_workgroups:
   case nir_intrinsic_launch_mesh_workgroups_with_payload_deref: {
      for (unsigned i = 0; i < 3; ++i) {
      nir_scalar dim = nir_scalar_resolved(instr->src[0].ssa, i);
   if (nir_scalar_is_const(dim))
      shader->info.mesh.ts_mesh_dispatch_dimensions[i] =
   }
               default:
      shader->info.uses_bindless |= intrinsic_is_bindless(instr);
   if (nir_intrinsic_writes_external_memory(instr))
            if (instr->intrinsic == nir_intrinsic_image_size ||
      instr->intrinsic == nir_intrinsic_image_samples ||
   instr->intrinsic == nir_intrinsic_image_deref_size ||
   instr->intrinsic == nir_intrinsic_image_deref_samples ||
   instr->intrinsic == nir_intrinsic_bindless_image_size ||
   instr->intrinsic == nir_intrinsic_bindless_image_samples)
            }
      static void
   gather_tex_info(nir_tex_instr *instr, nir_shader *shader)
   {
      if (shader->info.stage == MESA_SHADER_FRAGMENT &&
      nir_tex_instr_has_implicit_derivative(instr))
         if (nir_tex_instr_src_index(instr, nir_tex_src_texture_handle) != -1 ||
      nir_tex_instr_src_index(instr, nir_tex_src_sampler_handle) != -1)
         switch (instr->op) {
   case nir_texop_tg4:
      shader->info.uses_texture_gather = true;
      case nir_texop_txs:
   case nir_texop_query_levels:
   case nir_texop_texture_samples:
      shader->info.uses_resource_info_query = true;
      default:
            }
      static void
   gather_alu_info(nir_alu_instr *instr, nir_shader *shader)
   {
      if (nir_op_is_derivative(instr->op) &&
                           if (instr->op == nir_op_fddx || instr->op == nir_op_fddy)
                     for (unsigned i = 0; i < info->num_inputs; i++) {
      if (nir_alu_type_get_base_type(info->input_types[i]) == nir_type_float)
         else
      }
   if (nir_alu_type_get_base_type(info->output_type) == nir_type_float)
         else
      }
      static void
   gather_func_info(nir_function_impl *func, nir_shader *shader,
         {
      if (_mesa_set_search(visited_funcs, func))
                     nir_foreach_block(block, func) {
      nir_foreach_instr(instr, block) {
      switch (instr->type) {
   case nir_instr_type_alu:
      gather_alu_info(nir_instr_as_alu(instr), shader);
      case nir_instr_type_intrinsic:
      gather_intrinsic_info(nir_instr_as_intrinsic(instr), shader, dead_ctx);
      case nir_instr_type_tex:
      gather_tex_info(nir_instr_as_tex(instr), shader);
      case nir_instr_type_call: {
                     assert(impl || !"nir_shader_gather_info only works with linked shaders");
   gather_func_info(impl, shader, visited_funcs, dead_ctx);
      }
   default:
                  }
      void
   nir_shader_gather_info(nir_shader *shader, nir_function_impl *entrypoint)
   {
      shader->info.num_textures = 0;
   shader->info.num_images = 0;
   shader->info.bit_sizes_float = 0;
   shader->info.bit_sizes_int = 0;
            nir_foreach_variable_with_modes(var, shader, nir_var_image | nir_var_uniform) {
      if (var->data.bindless)
         /* Bindless textures and images don't use non-bindless slots.
   * Interface blocks imply inputs, outputs, UBO, or SSBO, which can only
   * mean bindless.
   */
   if (var->data.bindless || var->interface_type)
            shader->info.num_textures += glsl_type_get_sampler_count(var->type) +
                     /* these types may not initially be marked bindless */
   nir_foreach_variable_with_modes(var, shader, nir_var_shader_in | nir_var_shader_out) {
      const struct glsl_type *type = glsl_without_array(var->type);
   if (glsl_type_is_sampler(type) || glsl_type_is_image(type))
               shader->info.inputs_read = 0;
   shader->info.dual_slot_inputs = 0;
   shader->info.outputs_written = 0;
   shader->info.outputs_read = 0;
   shader->info.inputs_read_16bit = 0;
   shader->info.outputs_written_16bit = 0;
   shader->info.outputs_read_16bit = 0;
   shader->info.inputs_read_indirectly_16bit = 0;
   shader->info.outputs_accessed_indirectly_16bit = 0;
   shader->info.patch_outputs_read = 0;
   shader->info.patch_inputs_read = 0;
   shader->info.patch_outputs_written = 0;
   BITSET_ZERO(shader->info.system_values_read);
   shader->info.inputs_read_indirectly = 0;
   shader->info.outputs_accessed_indirectly = 0;
   shader->info.patch_inputs_read_indirectly = 0;
                     if (shader->info.stage == MESA_SHADER_VERTEX) {
         }
   if (shader->info.stage == MESA_SHADER_FRAGMENT) {
      shader->info.fs.uses_sample_qualifier = false;
   shader->info.fs.uses_discard = false;
   shader->info.fs.uses_demote = false;
   shader->info.fs.color_is_dual_source = false;
   shader->info.fs.uses_fbfetch_output = false;
   shader->info.fs.needs_quad_helper_invocations = false;
      }
   if (shader->info.stage == MESA_SHADER_TESS_CTRL) {
      shader->info.tess.tcs_cross_invocation_inputs_read = 0;
      }
   if (shader->info.stage == MESA_SHADER_MESH) {
         }
   if (shader->info.stage == MESA_SHADER_TASK) {
      shader->info.mesh.ts_mesh_dispatch_dimensions[0] = 0;
   shader->info.mesh.ts_mesh_dispatch_dimensions[1] = 0;
               if (shader->info.stage != MESA_SHADER_FRAGMENT)
            void *dead_ctx = ralloc_context(NULL);
   struct set *visited_funcs = _mesa_pointer_set_create(dead_ctx);
   gather_func_info(entrypoint, shader, visited_funcs, dead_ctx);
            shader->info.per_primitive_outputs = 0;
   shader->info.per_view_outputs = 0;
   nir_foreach_shader_out_variable(var, shader) {
      if (var->data.per_primitive) {
      assert(shader->info.stage == MESA_SHADER_MESH);
   assert(nir_is_arrayed_io(var, shader->info.stage));
   const unsigned slots =
            }
   if (var->data.per_view) {
      const unsigned slots =
                        shader->info.per_primitive_inputs = 0;
   if (shader->info.stage == MESA_SHADER_FRAGMENT) {
      nir_foreach_shader_in_variable(var, shader) {
      if (var->data.per_primitive) {
      const unsigned slots =
                           shader->info.ray_queries = 0;
   nir_foreach_variable_in_shader(var, shader) {
      if (!var->data.ray_query)
               }
   nir_foreach_function_impl(impl, shader) {
      nir_foreach_function_temp_variable(var, impl) {
                              }
