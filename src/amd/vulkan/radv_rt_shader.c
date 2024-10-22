   /*
   * Copyright © 2021 Google
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
      #include "nir/nir.h"
   #include "nir/nir_builder.h"
      #include "bvh/bvh.h"
   #include "meta/radv_meta.h"
   #include "ac_nir.h"
   #include "radv_private.h"
   #include "radv_rt_common.h"
   #include "radv_shader.h"
      /* Traversal stack size. This stack is put in LDS and experimentally 16 entries results in best
   * performance. */
   #define MAX_STACK_ENTRY_COUNT 16
      static bool
   lower_rt_derefs(nir_shader *shader)
   {
                                          nir_foreach_block (block, impl) {
      nir_foreach_instr_safe (instr, block) {
                     nir_deref_instr *deref = nir_instr_as_deref(instr);
                                 if (deref->deref_type == nir_deref_type_var) {
      b.cursor = nir_before_instr(&deref->instr);
   nir_deref_instr *replacement =
         nir_def_rewrite_uses(&deref->def, &replacement->def);
                     if (progress)
         else
               }
      /*
   * Global variables for an RT pipeline
   */
   struct rt_variables {
               /* idx of the next shader to run in the next iteration of the main loop.
   * During traversal, idx is used to store the SBT index and will contain
   * the correct resume index upon returning.
   */
   nir_variable *idx;
   nir_variable *shader_addr;
            /* scratch offset of the argument area relative to stack_ptr */
                     /* global address of the SBT entry used for the shader */
            /* trace_ray arguments */
   nir_variable *accel_struct;
   nir_variable *cull_mask_and_flags;
   nir_variable *sbt_offset;
   nir_variable *sbt_stride;
   nir_variable *miss_index;
   nir_variable *origin;
   nir_variable *tmin;
   nir_variable *direction;
            /* Properties of the primitive currently being visited. */
   nir_variable *primitive_id;
   nir_variable *geometry_id_and_flags;
   nir_variable *instance_addr;
   nir_variable *hit_kind;
            /* Output variables for intersection & anyhit shaders. */
   nir_variable *ahit_accept;
               };
      static struct rt_variables
   create_rt_variables(nir_shader *shader, const VkPipelineCreateFlags2KHR flags)
   {
      struct rt_variables vars = {
         };
   vars.idx = nir_variable_create(shader, nir_var_shader_temp, glsl_uint_type(), "idx");
   vars.shader_addr = nir_variable_create(shader, nir_var_shader_temp, glsl_uint64_t_type(), "shader_addr");
   vars.traversal_addr = nir_variable_create(shader, nir_var_shader_temp, glsl_uint64_t_type(), "traversal_addr");
   vars.arg = nir_variable_create(shader, nir_var_shader_temp, glsl_uint_type(), "arg");
   vars.stack_ptr = nir_variable_create(shader, nir_var_shader_temp, glsl_uint_type(), "stack_ptr");
            const struct glsl_type *vec3_type = glsl_vector_type(GLSL_TYPE_FLOAT, 3);
   vars.accel_struct = nir_variable_create(shader, nir_var_shader_temp, glsl_uint64_t_type(), "accel_struct");
   vars.cull_mask_and_flags = nir_variable_create(shader, nir_var_shader_temp, glsl_uint_type(), "cull_mask_and_flags");
   vars.sbt_offset = nir_variable_create(shader, nir_var_shader_temp, glsl_uint_type(), "sbt_offset");
   vars.sbt_stride = nir_variable_create(shader, nir_var_shader_temp, glsl_uint_type(), "sbt_stride");
   vars.miss_index = nir_variable_create(shader, nir_var_shader_temp, glsl_uint_type(), "miss_index");
   vars.origin = nir_variable_create(shader, nir_var_shader_temp, vec3_type, "ray_origin");
   vars.tmin = nir_variable_create(shader, nir_var_shader_temp, glsl_float_type(), "ray_tmin");
   vars.direction = nir_variable_create(shader, nir_var_shader_temp, vec3_type, "ray_direction");
            vars.primitive_id = nir_variable_create(shader, nir_var_shader_temp, glsl_uint_type(), "primitive_id");
   vars.geometry_id_and_flags =
         vars.instance_addr = nir_variable_create(shader, nir_var_shader_temp, glsl_uint64_t_type(), "instance_addr");
   vars.hit_kind = nir_variable_create(shader, nir_var_shader_temp, glsl_uint_type(), "hit_kind");
            vars.ahit_accept = nir_variable_create(shader, nir_var_shader_temp, glsl_bool_type(), "ahit_accept");
               }
      /*
   * Remap all the variables between the two rt_variables struct for inlining.
   */
   static void
   map_rt_variables(struct hash_table *var_remap, struct rt_variables *src, const struct rt_variables *dst)
   {
      _mesa_hash_table_insert(var_remap, src->idx, dst->idx);
   _mesa_hash_table_insert(var_remap, src->shader_addr, dst->shader_addr);
   _mesa_hash_table_insert(var_remap, src->traversal_addr, dst->traversal_addr);
   _mesa_hash_table_insert(var_remap, src->arg, dst->arg);
   _mesa_hash_table_insert(var_remap, src->stack_ptr, dst->stack_ptr);
            _mesa_hash_table_insert(var_remap, src->accel_struct, dst->accel_struct);
   _mesa_hash_table_insert(var_remap, src->cull_mask_and_flags, dst->cull_mask_and_flags);
   _mesa_hash_table_insert(var_remap, src->sbt_offset, dst->sbt_offset);
   _mesa_hash_table_insert(var_remap, src->sbt_stride, dst->sbt_stride);
   _mesa_hash_table_insert(var_remap, src->miss_index, dst->miss_index);
   _mesa_hash_table_insert(var_remap, src->origin, dst->origin);
   _mesa_hash_table_insert(var_remap, src->tmin, dst->tmin);
   _mesa_hash_table_insert(var_remap, src->direction, dst->direction);
            _mesa_hash_table_insert(var_remap, src->primitive_id, dst->primitive_id);
   _mesa_hash_table_insert(var_remap, src->geometry_id_and_flags, dst->geometry_id_and_flags);
   _mesa_hash_table_insert(var_remap, src->instance_addr, dst->instance_addr);
   _mesa_hash_table_insert(var_remap, src->hit_kind, dst->hit_kind);
   _mesa_hash_table_insert(var_remap, src->opaque, dst->opaque);
   _mesa_hash_table_insert(var_remap, src->ahit_accept, dst->ahit_accept);
      }
      /*
   * Create a copy of the global rt variables where the primitive/instance related variables are
   * independent.This is needed as we need to keep the old values of the global variables around
   * in case e.g. an anyhit shader reject the collision. So there are inner variables that get copied
   * to the outer variables once we commit to a better hit.
   */
   static struct rt_variables
   create_inner_vars(nir_builder *b, const struct rt_variables *vars)
   {
      struct rt_variables inner_vars = *vars;
   inner_vars.idx = nir_variable_create(b->shader, nir_var_shader_temp, glsl_uint_type(), "inner_idx");
   inner_vars.shader_record_ptr =
         inner_vars.primitive_id =
         inner_vars.geometry_id_and_flags =
         inner_vars.tmax = nir_variable_create(b->shader, nir_var_shader_temp, glsl_float_type(), "inner_tmax");
   inner_vars.instance_addr =
                     }
      static void
   insert_rt_return(nir_builder *b, const struct rt_variables *vars)
   {
      nir_store_var(b, vars->stack_ptr, nir_iadd_imm(b, nir_load_var(b, vars->stack_ptr), -16), 1);
   nir_store_var(b, vars->shader_addr, nir_load_scratch(b, 1, 64, nir_load_var(b, vars->stack_ptr), .align_mul = 16),
      }
      enum sbt_type {
      SBT_RAYGEN = offsetof(VkTraceRaysIndirectCommand2KHR, raygenShaderRecordAddress),
   SBT_MISS = offsetof(VkTraceRaysIndirectCommand2KHR, missShaderBindingTableAddress),
   SBT_HIT = offsetof(VkTraceRaysIndirectCommand2KHR, hitShaderBindingTableAddress),
      };
      enum sbt_entry {
      SBT_RECURSIVE_PTR = offsetof(struct radv_pipeline_group_handle, recursive_shader_ptr),
   SBT_GENERAL_IDX = offsetof(struct radv_pipeline_group_handle, general_index),
   SBT_CLOSEST_HIT_IDX = offsetof(struct radv_pipeline_group_handle, closest_hit_index),
   SBT_INTERSECTION_IDX = offsetof(struct radv_pipeline_group_handle, intersection_index),
      };
      static nir_def *
   get_sbt_ptr(nir_builder *b, nir_def *idx, enum sbt_type binding)
   {
                        nir_def *stride_offset = nir_imm_int(b, binding + (binding == SBT_RAYGEN ? 8 : 16));
               }
      static void
   load_sbt_entry(nir_builder *b, const struct rt_variables *vars, nir_def *idx, enum sbt_type binding,
         {
      nir_def *addr = get_sbt_ptr(b, idx, binding);
            if (offset == SBT_RECURSIVE_PTR) {
         } else {
                  nir_def *record_addr = nir_iadd_imm(b, addr, RADV_RT_HANDLE_SIZE);
      }
      struct radv_lower_rt_instruction_data {
      struct rt_variables *vars;
      };
      static bool
   radv_lower_rt_instruction(nir_builder *b, nir_instr *instr, void *_data)
   {
      if (instr->type == nir_instr_type_jump) {
      nir_jump_instr *jump = nir_instr_as_jump(instr);
   if (jump->type == nir_jump_halt) {
      jump->type = nir_jump_return;
      }
      } else if (instr->type != nir_instr_type_intrinsic) {
                           struct radv_lower_rt_instruction_data *data = _data;
   struct rt_variables *vars = data->vars;
                     nir_def *ret = NULL;
   switch (intr->intrinsic) {
   case nir_intrinsic_rt_execute_callable: {
      uint32_t size = align(nir_intrinsic_stack_size(intr), 16);
   nir_def *ret_ptr = nir_load_resume_shader_address_amd(b, nir_intrinsic_call_idx(intr));
            nir_store_var(b, vars->stack_ptr, nir_iadd_imm_nuw(b, nir_load_var(b, vars->stack_ptr), size), 1);
            nir_store_var(b, vars->stack_ptr, nir_iadd_imm_nuw(b, nir_load_var(b, vars->stack_ptr), 16), 1);
                     vars->stack_size = MAX2(vars->stack_size, size + 16);
      }
   case nir_intrinsic_rt_trace_ray: {
      uint32_t size = align(nir_intrinsic_stack_size(intr), 16);
   nir_def *ret_ptr = nir_load_resume_shader_address_amd(b, nir_intrinsic_call_idx(intr));
            nir_store_var(b, vars->stack_ptr, nir_iadd_imm_nuw(b, nir_load_var(b, vars->stack_ptr), size), 1);
                     nir_store_var(b, vars->shader_addr, nir_load_var(b, vars->traversal_addr), 1);
                     /* Per the SPIR-V extension spec we have to ignore some bits for some arguments. */
   nir_store_var(b, vars->accel_struct, intr->src[0].ssa, 0x1);
   nir_store_var(b, vars->cull_mask_and_flags, nir_ior(b, nir_ishl_imm(b, intr->src[2].ssa, 24), intr->src[1].ssa),
         nir_store_var(b, vars->sbt_offset, nir_iand_imm(b, intr->src[3].ssa, 0xf), 0x1);
   nir_store_var(b, vars->sbt_stride, nir_iand_imm(b, intr->src[4].ssa, 0xf), 0x1);
   nir_store_var(b, vars->miss_index, nir_iand_imm(b, intr->src[5].ssa, 0xffff), 0x1);
   nir_store_var(b, vars->origin, intr->src[6].ssa, 0x7);
   nir_store_var(b, vars->tmin, intr->src[7].ssa, 0x1);
   nir_store_var(b, vars->direction, intr->src[8].ssa, 0x7);
   nir_store_var(b, vars->tmax, intr->src[9].ssa, 0x1);
      }
   case nir_intrinsic_rt_resume: {
               nir_store_var(b, vars->stack_ptr, nir_iadd_imm(b, nir_load_var(b, vars->stack_ptr), -size), 1);
      }
   case nir_intrinsic_rt_return_amd: {
      if (b->shader->info.stage == MESA_SHADER_RAYGEN) {
      nir_terminate(b);
      }
   insert_rt_return(b, vars);
      }
   case nir_intrinsic_load_scratch: {
      if (apply_stack_ptr)
            }
   case nir_intrinsic_store_scratch: {
      if (apply_stack_ptr)
            }
   case nir_intrinsic_load_rt_arg_scratch_offset_amd: {
      ret = nir_load_var(b, vars->arg);
      }
   case nir_intrinsic_load_shader_record_ptr: {
      ret = nir_load_var(b, vars->shader_record_ptr);
      }
   case nir_intrinsic_load_ray_t_min: {
      ret = nir_load_var(b, vars->tmin);
      }
   case nir_intrinsic_load_ray_t_max: {
      ret = nir_load_var(b, vars->tmax);
      }
   case nir_intrinsic_load_ray_world_origin: {
      ret = nir_load_var(b, vars->origin);
      }
   case nir_intrinsic_load_ray_world_direction: {
      ret = nir_load_var(b, vars->direction);
      }
   case nir_intrinsic_load_ray_instance_custom_index: {
      nir_def *instance_node_addr = nir_load_var(b, vars->instance_addr);
   nir_def *custom_instance_and_mask = nir_build_load_global(
      b, 1, 32,
      ret = nir_iand_imm(b, custom_instance_and_mask, 0xFFFFFF);
      }
   case nir_intrinsic_load_primitive_id: {
      ret = nir_load_var(b, vars->primitive_id);
      }
   case nir_intrinsic_load_ray_geometry_index: {
      ret = nir_load_var(b, vars->geometry_id_and_flags);
   ret = nir_iand_imm(b, ret, 0xFFFFFFF);
      }
   case nir_intrinsic_load_instance_id: {
      nir_def *instance_node_addr = nir_load_var(b, vars->instance_addr);
   ret = nir_build_load_global(
            }
   case nir_intrinsic_load_ray_flags: {
      ret = nir_iand_imm(b, nir_load_var(b, vars->cull_mask_and_flags), 0xFFFFFF);
      }
   case nir_intrinsic_load_ray_hit_kind: {
      ret = nir_load_var(b, vars->hit_kind);
      }
   case nir_intrinsic_load_ray_world_to_object: {
      unsigned c = nir_intrinsic_column(intr);
   nir_def *instance_node_addr = nir_load_var(b, vars->instance_addr);
   nir_def *wto_matrix[3];
            nir_def *vals[3];
   for (unsigned i = 0; i < 3; ++i)
            ret = nir_vec(b, vals, 3);
      }
   case nir_intrinsic_load_ray_object_to_world: {
      unsigned c = nir_intrinsic_column(intr);
   nir_def *instance_node_addr = nir_load_var(b, vars->instance_addr);
   nir_def *rows[3];
   for (unsigned r = 0; r < 3; ++r)
      rows[r] = nir_build_load_global(
      b, 4, 32,
   ret = nir_vec3(b, nir_channel(b, rows[0], c), nir_channel(b, rows[1], c), nir_channel(b, rows[2], c));
      }
   case nir_intrinsic_load_ray_object_origin: {
      nir_def *instance_node_addr = nir_load_var(b, vars->instance_addr);
   nir_def *wto_matrix[3];
   nir_build_wto_matrix_load(b, instance_node_addr, wto_matrix);
   ret = nir_build_vec3_mat_mult(b, nir_load_var(b, vars->origin), wto_matrix, true);
      }
   case nir_intrinsic_load_ray_object_direction: {
      nir_def *instance_node_addr = nir_load_var(b, vars->instance_addr);
   nir_def *wto_matrix[3];
   nir_build_wto_matrix_load(b, instance_node_addr, wto_matrix);
   ret = nir_build_vec3_mat_mult(b, nir_load_var(b, vars->direction), wto_matrix, false);
      }
   case nir_intrinsic_load_intersection_opaque_amd: {
      ret = nir_load_var(b, vars->opaque);
      }
   case nir_intrinsic_load_cull_mask: {
      ret = nir_ushr_imm(b, nir_load_var(b, vars->cull_mask_and_flags), 24);
      }
   case nir_intrinsic_ignore_ray_intersection: {
               /* The if is a workaround to avoid having to fix up control flow manually */
   nir_push_if(b, nir_imm_true(b));
   nir_jump(b, nir_jump_return);
   nir_pop_if(b, NULL);
      }
   case nir_intrinsic_terminate_ray: {
      nir_store_var(b, vars->ahit_accept, nir_imm_true(b), 0x1);
            /* The if is a workaround to avoid having to fix up control flow manually */
   nir_push_if(b, nir_imm_true(b));
   nir_jump(b, nir_jump_return);
   nir_pop_if(b, NULL);
      }
   case nir_intrinsic_report_ray_intersection: {
      nir_push_if(b, nir_iand(b, nir_fge(b, nir_load_var(b, vars->tmax), intr->src[0].ssa),
         {
      nir_store_var(b, vars->ahit_accept, nir_imm_true(b), 0x1);
   nir_store_var(b, vars->tmax, intr->src[0].ssa, 1);
      }
   nir_pop_if(b, NULL);
      }
   case nir_intrinsic_load_sbt_offset_amd: {
      ret = nir_load_var(b, vars->sbt_offset);
      }
   case nir_intrinsic_load_sbt_stride_amd: {
      ret = nir_load_var(b, vars->sbt_stride);
      }
   case nir_intrinsic_load_accel_struct_amd: {
      ret = nir_load_var(b, vars->accel_struct);
      }
   case nir_intrinsic_load_cull_mask_and_flags_amd: {
      ret = nir_load_var(b, vars->cull_mask_and_flags);
      }
   case nir_intrinsic_execute_closest_hit_amd: {
      nir_store_var(b, vars->tmax, intr->src[1].ssa, 0x1);
   nir_store_var(b, vars->primitive_id, intr->src[2].ssa, 0x1);
   nir_store_var(b, vars->instance_addr, intr->src[3].ssa, 0x1);
   nir_store_var(b, vars->geometry_id_and_flags, intr->src[4].ssa, 0x1);
   nir_store_var(b, vars->hit_kind, intr->src[5].ssa, 0x1);
            nir_def *should_return =
            if (!(vars->flags & VK_PIPELINE_CREATE_2_RAY_TRACING_NO_NULL_CLOSEST_HIT_SHADERS_BIT_KHR)) {
                  /* should_return is set if we had a hit but we won't be calling the closest hit
   * shader and hence need to return immediately to the calling shader. */
   nir_push_if(b, should_return);
   insert_rt_return(b, vars);
   nir_pop_if(b, NULL);
      }
   case nir_intrinsic_execute_miss_amd: {
      nir_store_var(b, vars->tmax, intr->src[0].ssa, 0x1);
   nir_def *undef = nir_undef(b, 1, 32);
   nir_store_var(b, vars->primitive_id, undef, 0x1);
   nir_store_var(b, vars->instance_addr, nir_undef(b, 1, 64), 0x1);
   nir_store_var(b, vars->geometry_id_and_flags, undef, 0x1);
   nir_store_var(b, vars->hit_kind, undef, 0x1);
   nir_def *miss_index = nir_load_var(b, vars->miss_index);
            if (!(vars->flags & VK_PIPELINE_CREATE_2_RAY_TRACING_NO_NULL_MISS_SHADERS_BIT_KHR)) {
      /* In case of a NULL miss shader, do nothing and just return. */
   nir_push_if(b, nir_ieq_imm(b, nir_load_var(b, vars->shader_addr), 0));
   insert_rt_return(b, vars);
                  }
   default:
                  if (ret)
                     }
      /* This lowers all the RT instructions that we do not want to pass on to the combined shader and
   * that we can implement using the variables from the shader we are going to inline into. */
   static void
   lower_rt_instructions(nir_shader *shader, struct rt_variables *vars, bool apply_stack_ptr)
   {
      struct radv_lower_rt_instruction_data data = {
      .vars = vars,
      };
      }
      static bool
   lower_hit_attrib_deref(nir_builder *b, nir_instr *instr, void *data)
   {
      if (instr->type != nir_instr_type_intrinsic)
            nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(instr);
   if (intrin->intrinsic != nir_intrinsic_load_deref && intrin->intrinsic != nir_intrinsic_store_deref)
            nir_deref_instr *deref = nir_src_as_deref(intrin->src[0]);
   if (!nir_deref_mode_is(deref, nir_var_ray_hit_attrib))
                              if (intrin->intrinsic == nir_intrinsic_load_deref) {
      uint32_t num_components = intrin->def.num_components;
                     for (uint32_t comp = 0; comp < num_components; comp++) {
      uint32_t offset = deref->var->data.driver_location + comp * bit_size / 8;
                  if (bit_size == 64) {
      components[comp] = nir_pack_64_2x32_split(b, nir_load_hit_attrib_amd(b, .base = base),
      } else if (bit_size == 32) {
         } else if (bit_size == 16) {
      components[comp] =
      } else if (bit_size == 8) {
      components[comp] =
      } else {
                        } else {
      nir_def *value = intrin->src[1].ssa;
   uint32_t num_components = value->num_components;
            for (uint32_t comp = 0; comp < num_components; comp++) {
      uint32_t offset = deref->var->data.driver_location + comp * bit_size / 8;
                           if (bit_size == 64) {
      nir_store_hit_attrib_amd(b, nir_unpack_64_2x32_split_x(b, component), .base = base);
      } else if (bit_size == 32) {
         } else if (bit_size == 16) {
      nir_def *prev = nir_unpack_32_2x16(b, nir_load_hit_attrib_amd(b, .base = base));
   nir_def *components[2];
   for (uint32_t word = 0; word < 2; word++)
            } else if (bit_size == 8) {
      nir_def *prev = nir_unpack_bits(b, nir_load_hit_attrib_amd(b, .base = base), 8);
   nir_def *components[4];
   for (uint32_t byte = 0; byte < 4; byte++)
            } else {
                        nir_instr_remove(instr);
      }
      static bool
   lower_hit_attrib_derefs(nir_shader *shader)
   {
      bool progress = nir_shader_instructions_pass(shader, lower_hit_attrib_deref,
         if (progress) {
      nir_remove_dead_derefs(shader);
                  }
      /* Lowers hit attributes to registers or shared memory. If hit_attribs is NULL, attributes are
   * lowered to shared memory. */
   static void
   lower_hit_attribs(nir_shader *shader, nir_variable **hit_attribs, uint32_t workgroup_size)
   {
               nir_foreach_variable_with_modes (attrib, shader, nir_var_ray_hit_attrib)
                     nir_foreach_block (block, impl) {
      nir_foreach_instr_safe (instr, block) {
                     nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(instr);
   if (intrin->intrinsic != nir_intrinsic_load_hit_attrib_amd &&
                           nir_def *offset;
   if (!hit_attribs)
      offset = nir_imul_imm(
               if (intrin->intrinsic == nir_intrinsic_load_hit_attrib_amd) {
      nir_def *ret;
   if (hit_attribs)
         else
            } else {
      if (hit_attribs)
         else
      }
                  if (!hit_attribs)
      }
      static void
   inline_constants(nir_shader *dst, nir_shader *src)
   {
      if (!src->constant_data_size)
            uint32_t align_mul = 1;
   if (dst->constant_data_size) {
      nir_foreach_block (block, nir_shader_get_entrypoint(src)) {
      nir_foreach_instr (instr, block) {
                     nir_intrinsic_instr *intrinsic = nir_instr_as_intrinsic(instr);
   if (intrinsic->intrinsic == nir_intrinsic_load_constant)
                     uint32_t old_constant_data_size = dst->constant_data_size;
   uint32_t base_offset = align(dst->constant_data_size, align_mul);
   dst->constant_data_size = base_offset + src->constant_data_size;
   dst->constant_data = rerzalloc_size(dst, dst->constant_data, old_constant_data_size, dst->constant_data_size);
            if (!base_offset)
            nir_foreach_block (block, nir_shader_get_entrypoint(src)) {
      nir_foreach_instr (instr, block) {
                     nir_intrinsic_instr *intrinsic = nir_instr_as_intrinsic(instr);
   if (intrinsic->intrinsic == nir_intrinsic_load_constant)
            }
      static void
   insert_rt_case(nir_builder *b, nir_shader *shader, struct rt_variables *vars, nir_def *idx, uint32_t call_idx)
   {
                        struct rt_variables src_vars = create_rt_variables(shader, vars->flags);
                     NIR_PASS(_, shader, nir_lower_returns);
                     nir_push_if(b, nir_ieq_imm(b, idx, call_idx));
   nir_inline_function_impl(b, nir_shader_get_entrypoint(shader), NULL, var_remap);
               }
      nir_shader *
   radv_parse_rt_stage(struct radv_device *device, const VkPipelineShaderStageCreateInfo *sinfo,
         {
                                 NIR_PASS(_, shader, nir_split_struct_vars, nir_var_ray_hit_attrib);
   NIR_PASS(_, shader, nir_lower_indirect_derefs, nir_var_ray_hit_attrib, UINT32_MAX);
            NIR_PASS(_, shader, nir_lower_vars_to_explicit_types,
                  NIR_PASS(_, shader, lower_rt_derefs);
                        }
      static nir_function_impl *
   lower_any_hit_for_intersection(nir_shader *any_hit)
   {
               /* Any-hit shaders need three parameters */
   assert(impl->function->num_params == 0);
   nir_parameter params[] = {
      {
      /* A pointer to a boolean value for whether or not the hit was
   * accepted.
   */
   .num_components = 1,
      },
   {
      /* The hit T value */
   .num_components = 1,
      },
   {
      /* The hit kind */
   .num_components = 1,
      },
   {
      /* Scratch offset */
   .num_components = 1,
         };
   impl->function->num_params = ARRAY_SIZE(params);
   impl->function->params = ralloc_array(any_hit, nir_parameter, ARRAY_SIZE(params));
            nir_builder build = nir_builder_at(nir_before_impl(impl));
            nir_def *commit_ptr = nir_load_param(b, 0);
   nir_def *hit_t = nir_load_param(b, 1);
   nir_def *hit_kind = nir_load_param(b, 2);
                     nir_foreach_block_safe (block, impl) {
      nir_foreach_instr_safe (instr, block) {
      switch (instr->type) {
   case nir_instr_type_intrinsic: {
      nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(instr);
   switch (intrin->intrinsic) {
   case nir_intrinsic_ignore_ray_intersection:
      b->cursor = nir_instr_remove(&intrin->instr);
   /* We put the newly emitted code inside a dummy if because it's
   * going to contain a jump instruction and we don't want to
   * deal with that mess here.  It'll get dealt with by our
   * control-flow optimization passes.
   */
   nir_store_deref(b, commit, nir_imm_false(b), 0x1);
   nir_push_if(b, nir_imm_true(b));
                     case nir_intrinsic_terminate_ray:
      /* The "normal" handling of terminateRay works fine in
                     case nir_intrinsic_load_ray_t_max:
                        case nir_intrinsic_load_ray_hit_kind:
                        /* We place all any_hit scratch variables after intersection scratch variables.
   * For that reason, we increment the scratch offset by the intersection scratch
   * size. For call_data, we have to subtract the offset again.
   *
   * Note that we don't increase the scratch size as it is already reflected via
   * the any_hit stack_size.
   */
   case nir_intrinsic_load_scratch:
      b->cursor = nir_before_instr(instr);
   nir_src_rewrite(&intrin->src[0], nir_iadd_nuw(b, scratch_offset, intrin->src[0].ssa));
      case nir_intrinsic_store_scratch:
      b->cursor = nir_before_instr(instr);
   nir_src_rewrite(&intrin->src[1], nir_iadd_nuw(b, scratch_offset, intrin->src[1].ssa));
      case nir_intrinsic_load_rt_arg_scratch_offset_amd:
      b->cursor = nir_after_instr(instr);
                     default:
         }
      }
   case nir_instr_type_jump: {
      nir_jump_instr *jump = nir_instr_as_jump(instr);
   if (jump->type == nir_jump_halt) {
      b->cursor = nir_instr_remove(instr);
      }
               default:
                                                      }
      /* Inline the any_hit shader into the intersection shader so we don't have
   * to implement yet another shader call interface here. Neither do any recursion.
   */
   static void
   nir_lower_intersection_shader(nir_shader *intersection, nir_shader *any_hit)
   {
               nir_function_impl *any_hit_impl = NULL;
   struct hash_table *any_hit_var_remap = NULL;
   if (any_hit) {
      any_hit = nir_shader_clone(dead_ctx, any_hit);
                     any_hit_impl = lower_any_hit_for_intersection(any_hit);
                        nir_builder build = nir_builder_create(impl);
                     nir_variable *commit = nir_local_variable_create(impl, glsl_bool_type(), "ray_commit");
            nir_foreach_block_safe (block, impl) {
      nir_foreach_instr_safe (instr, block) {
                     nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(instr);
                  b->cursor = nir_instr_remove(&intrin->instr);
   nir_def *hit_t = intrin->src[0].ssa;
   nir_def *hit_kind = intrin->src[1].ssa;
                  /* bool commit_tmp = false; */
                  nir_push_if(b, nir_iand(b, nir_fge(b, hit_t, min_t), nir_fge(b, max_t, hit_t)));
   {
                     if (any_hit_impl != NULL) {
      nir_push_if(b, nir_inot(b, nir_load_intersection_opaque_amd(b)));
   {
      nir_def *params[] = {
      &nir_build_deref_var(b, commit_tmp)->def,
   hit_t,
   hit_kind,
      };
                        nir_push_if(b, nir_load_var(b, commit_tmp));
   {
         }
                     nir_def *accepted = nir_load_var(b, commit_tmp);
         }
            /* We did some inlining; have to re-index SSA defs */
            /* Eliminate the casts introduced for the commit return of the any-hit shader. */
               }
      /* Variables only used internally to ray traversal. This is data that describes
   * the current state of the traversal vs. what we'd give to a shader.  e.g. what
   * is the instance we're currently visiting vs. what is the instance of the
   * closest hit. */
   struct rt_traversal_vars {
      nir_variable *origin;
   nir_variable *dir;
   nir_variable *inv_dir;
   nir_variable *sbt_offset_and_flags;
   nir_variable *instance_addr;
   nir_variable *hit;
   nir_variable *bvh_base;
   nir_variable *stack;
   nir_variable *top_stack;
   nir_variable *stack_low_watermark;
   nir_variable *current_node;
   nir_variable *previous_node;
   nir_variable *instance_top_node;
      };
      static struct rt_traversal_vars
   init_traversal_vars(nir_builder *b)
   {
      const struct glsl_type *vec3_type = glsl_vector_type(GLSL_TYPE_FLOAT, 3);
            ret.origin = nir_variable_create(b->shader, nir_var_shader_temp, vec3_type, "traversal_origin");
   ret.dir = nir_variable_create(b->shader, nir_var_shader_temp, vec3_type, "traversal_dir");
   ret.inv_dir = nir_variable_create(b->shader, nir_var_shader_temp, vec3_type, "traversal_inv_dir");
   ret.sbt_offset_and_flags =
         ret.instance_addr = nir_variable_create(b->shader, nir_var_shader_temp, glsl_uint64_t_type(), "instance_addr");
   ret.hit = nir_variable_create(b->shader, nir_var_shader_temp, glsl_bool_type(), "traversal_hit");
   ret.bvh_base = nir_variable_create(b->shader, nir_var_shader_temp, glsl_uint64_t_type(), "traversal_bvh_base");
   ret.stack = nir_variable_create(b->shader, nir_var_shader_temp, glsl_uint_type(), "traversal_stack_ptr");
   ret.top_stack = nir_variable_create(b->shader, nir_var_shader_temp, glsl_uint_type(), "traversal_top_stack_ptr");
   ret.stack_low_watermark =
         ret.current_node = nir_variable_create(b->shader, nir_var_shader_temp, glsl_uint_type(), "current_node;");
   ret.previous_node = nir_variable_create(b->shader, nir_var_shader_temp, glsl_uint_type(), "previous_node");
   ret.instance_top_node = nir_variable_create(b->shader, nir_var_shader_temp, glsl_uint_type(), "instance_top_node");
   ret.instance_bottom_node =
            }
      struct traversal_data {
      struct radv_device *device;
   struct rt_variables *vars;
   struct rt_traversal_vars *trav_vars;
               };
      static void
   visit_any_hit_shaders(struct radv_device *device, nir_builder *b, struct traversal_data *data,
         {
               if (!(vars->flags & VK_PIPELINE_CREATE_2_RAY_TRACING_NO_NULL_ANY_HIT_SHADERS_BIT_KHR))
            for (unsigned i = 0; i < data->pipeline->group_count; ++i) {
      struct radv_ray_tracing_group *group = &data->pipeline->groups[i];
            switch (group->type) {
   case VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR:
      shader_id = group->any_hit_shader;
      default:
         }
   if (shader_id == VK_SHADER_UNUSED_KHR)
            /* Avoid emitting stages with the same shaders/handles multiple times. */
   bool is_dup = false;
   for (unsigned j = 0; j < i; ++j)
                  if (is_dup)
            nir_shader *nir_stage = radv_pipeline_cache_handle_to_nir(device, data->pipeline->stages[shader_id].nir);
            insert_rt_case(b, nir_stage, vars, sbt_idx, data->pipeline->groups[i].handle.any_hit_index);
               if (!(vars->flags & VK_PIPELINE_CREATE_2_RAY_TRACING_NO_NULL_ANY_HIT_SHADERS_BIT_KHR))
      }
      static void
   handle_candidate_triangle(nir_builder *b, struct radv_triangle_intersection *intersection,
         {
               nir_def *geometry_id = nir_iand_imm(b, intersection->base.geometry_id_and_flags, 0xfffffff);
   nir_def *sbt_idx =
      nir_iadd(b,
                              nir_def *prev_barycentrics = nir_load_var(b, data->barycentrics);
            nir_store_var(b, data->vars->ahit_accept, nir_imm_true(b), 0x1);
            nir_push_if(b, nir_inot(b, intersection->base.opaque));
   {
               nir_store_var(b, inner_vars.primitive_id, intersection->base.primitive_id, 1);
   nir_store_var(b, inner_vars.geometry_id_and_flags, intersection->base.geometry_id_and_flags, 1);
   nir_store_var(b, inner_vars.tmax, intersection->t, 0x1);
   nir_store_var(b, inner_vars.instance_addr, nir_load_var(b, data->trav_vars->instance_addr), 0x1);
                              nir_push_if(b, nir_inot(b, nir_load_var(b, data->vars->ahit_accept)));
   {
      nir_store_var(b, data->barycentrics, prev_barycentrics, 0x3);
      }
      }
            nir_store_var(b, data->vars->primitive_id, intersection->base.primitive_id, 1);
   nir_store_var(b, data->vars->geometry_id_and_flags, intersection->base.geometry_id_and_flags, 1);
   nir_store_var(b, data->vars->tmax, intersection->t, 0x1);
   nir_store_var(b, data->vars->instance_addr, nir_load_var(b, data->trav_vars->instance_addr), 0x1);
            nir_store_var(b, data->vars->idx, sbt_idx, 1);
            nir_def *ray_terminated = nir_load_var(b, data->vars->ahit_terminate);
   nir_push_if(b, nir_ior(b, ray_flags->terminate_on_first_hit, ray_terminated));
   {
         }
      }
      static void
   handle_candidate_aabb(nir_builder *b, struct radv_leaf_intersection *intersection,
         {
               nir_def *geometry_id = nir_iand_imm(b, intersection->geometry_id_and_flags, 0xfffffff);
   nir_def *sbt_idx =
      nir_iadd(b,
                              /* For AABBs the intersection shader writes the hit kind, and only does it if it is the
   * next closest hit candidate. */
            nir_store_var(b, inner_vars.primitive_id, intersection->primitive_id, 1);
   nir_store_var(b, inner_vars.geometry_id_and_flags, intersection->geometry_id_and_flags, 1);
   nir_store_var(b, inner_vars.tmax, nir_load_var(b, data->vars->tmax), 0x1);
   nir_store_var(b, inner_vars.instance_addr, nir_load_var(b, data->trav_vars->instance_addr), 0x1);
                     nir_store_var(b, data->vars->ahit_accept, nir_imm_false(b), 0x1);
            if (!(data->vars->flags & VK_PIPELINE_CREATE_2_RAY_TRACING_NO_NULL_INTERSECTION_SHADERS_BIT_KHR))
            for (unsigned i = 0; i < data->pipeline->group_count; ++i) {
      struct radv_ray_tracing_group *group = &data->pipeline->groups[i];
   uint32_t shader_id = VK_SHADER_UNUSED_KHR;
            switch (group->type) {
   case VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR:
      shader_id = group->intersection_shader;
   any_hit_shader_id = group->any_hit_shader;
      default:
         }
   if (shader_id == VK_SHADER_UNUSED_KHR)
            /* Avoid emitting stages with the same shaders/handles multiple times. */
   bool is_dup = false;
   for (unsigned j = 0; j < i; ++j)
                  if (is_dup)
            nir_shader *nir_stage = radv_pipeline_cache_handle_to_nir(data->device, data->pipeline->stages[shader_id].nir);
            nir_shader *any_hit_stage = NULL;
   if (any_hit_shader_id != VK_SHADER_UNUSED_KHR) {
                                    nir_lower_intersection_shader(nir_stage, any_hit_stage);
               insert_rt_case(b, nir_stage, &inner_vars, nir_load_var(b, inner_vars.idx),
                     if (!(data->vars->flags & VK_PIPELINE_CREATE_2_RAY_TRACING_NO_NULL_INTERSECTION_SHADERS_BIT_KHR))
            nir_push_if(b, nir_load_var(b, data->vars->ahit_accept));
   {
      nir_store_var(b, data->vars->primitive_id, intersection->primitive_id, 1);
   nir_store_var(b, data->vars->geometry_id_and_flags, intersection->geometry_id_and_flags, 1);
   nir_store_var(b, data->vars->tmax, nir_load_var(b, inner_vars.tmax), 0x1);
            nir_store_var(b, data->vars->idx, sbt_idx, 1);
            nir_def *terminate_on_first_hit = nir_test_mask(b, args->flags, SpvRayFlagsTerminateOnFirstHitKHRMask);
   nir_def *ray_terminated = nir_load_var(b, data->vars->ahit_terminate);
   nir_push_if(b, nir_ior(b, terminate_on_first_hit, ray_terminated));
   {
         }
      }
      }
      static void
   visit_closest_hit_shaders(struct radv_device *device, nir_builder *b, struct radv_ray_tracing_pipeline *pipeline,
         {
               if (!(vars->flags & VK_PIPELINE_CREATE_RAY_TRACING_NO_NULL_CLOSEST_HIT_SHADERS_BIT_KHR))
            for (unsigned i = 0; i < pipeline->group_count; ++i) {
               unsigned shader_id = VK_SHADER_UNUSED_KHR;
   if (group->type != VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR)
            if (shader_id == VK_SHADER_UNUSED_KHR)
            /* Avoid emitting stages with the same shaders/handles multiple times. */
   bool is_dup = false;
   for (unsigned j = 0; j < i; ++j)
                  if (is_dup)
            nir_shader *nir_stage = radv_pipeline_cache_handle_to_nir(device, pipeline->stages[shader_id].nir);
            insert_rt_case(b, nir_stage, vars, sbt_idx, pipeline->groups[i].handle.closest_hit_index);
               if (!(vars->flags & VK_PIPELINE_CREATE_RAY_TRACING_NO_NULL_CLOSEST_HIT_SHADERS_BIT_KHR))
      }
      static void
   visit_miss_shaders(struct radv_device *device, nir_builder *b, struct radv_ray_tracing_pipeline *pipeline,
         {
               if (!(vars->flags & VK_PIPELINE_CREATE_RAY_TRACING_NO_NULL_MISS_SHADERS_BIT_KHR))
            for (unsigned i = 0; i < pipeline->group_count; ++i) {
               unsigned shader_id = VK_SHADER_UNUSED_KHR;
   if (group->type == VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR)
            if (shader_id == VK_SHADER_UNUSED_KHR)
            if (pipeline->stages[shader_id].stage != MESA_SHADER_MISS)
            /* Avoid emitting stages with the same shaders/handles multiple times. */
   bool is_dup = false;
   for (unsigned j = 0; j < i; ++j)
                  if (is_dup)
            nir_shader *nir_stage = radv_pipeline_cache_handle_to_nir(device, pipeline->stages[shader_id].nir);
            insert_rt_case(b, nir_stage, vars, sbt_idx, pipeline->groups[i].handle.general_index);
               if (!(vars->flags & VK_PIPELINE_CREATE_RAY_TRACING_NO_NULL_MISS_SHADERS_BIT_KHR))
      }
      static void
   store_stack_entry(nir_builder *b, nir_def *index, nir_def *value, const struct radv_ray_traversal_args *args)
   {
         }
      static nir_def *
   load_stack_entry(nir_builder *b, nir_def *index, const struct radv_ray_traversal_args *args)
   {
         }
      static void
   radv_build_traversal(struct radv_device *device, struct radv_ray_tracing_pipeline *pipeline,
               {
      nir_variable *barycentrics =
                                    nir_def *accel_struct = nir_load_var(b, vars->accel_struct);
   nir_def *bvh_offset = nir_build_load_global(
      b, 1, 32, nir_iadd_imm(b, accel_struct, offsetof(struct radv_accel_struct_header, bvh_offset)),
      nir_def *root_bvh_base = nir_iadd(b, accel_struct, nir_u2u64(b, bvh_offset));
                              nir_store_var(b, trav_vars.origin, nir_load_var(b, vars->origin), 7);
   nir_store_var(b, trav_vars.dir, nir_load_var(b, vars->direction), 7);
   nir_store_var(b, trav_vars.inv_dir, nir_fdiv(b, vec3ones, nir_load_var(b, trav_vars.dir)), 7);
   nir_store_var(b, trav_vars.sbt_offset_and_flags, nir_imm_int(b, 0), 1);
            nir_store_var(b, trav_vars.stack, nir_imul_imm(b, nir_load_local_invocation_index(b), sizeof(uint32_t)), 1);
   nir_store_var(b, trav_vars.stack_low_watermark, nir_load_var(b, trav_vars.stack), 1);
   nir_store_var(b, trav_vars.current_node, nir_imm_int(b, RADV_BVH_ROOT_NODE), 0x1);
   nir_store_var(b, trav_vars.previous_node, nir_imm_int(b, RADV_BVH_INVALID_NODE), 0x1);
   nir_store_var(b, trav_vars.instance_top_node, nir_imm_int(b, RADV_BVH_INVALID_NODE), 0x1);
                     struct radv_ray_traversal_vars trav_vars_args = {
      .tmax = nir_build_deref_var(b, vars->tmax),
   .origin = nir_build_deref_var(b, trav_vars.origin),
   .dir = nir_build_deref_var(b, trav_vars.dir),
   .inv_dir = nir_build_deref_var(b, trav_vars.inv_dir),
   .bvh_base = nir_build_deref_var(b, trav_vars.bvh_base),
   .stack = nir_build_deref_var(b, trav_vars.stack),
   .top_stack = nir_build_deref_var(b, trav_vars.top_stack),
   .stack_low_watermark = nir_build_deref_var(b, trav_vars.stack_low_watermark),
   .current_node = nir_build_deref_var(b, trav_vars.current_node),
   .previous_node = nir_build_deref_var(b, trav_vars.previous_node),
   .instance_top_node = nir_build_deref_var(b, trav_vars.instance_top_node),
   .instance_bottom_node = nir_build_deref_var(b, trav_vars.instance_bottom_node),
   .instance_addr = nir_build_deref_var(b, trav_vars.instance_addr),
               struct traversal_data data = {
      .device = device,
   .vars = vars,
   .trav_vars = &trav_vars,
   .barycentrics = barycentrics,
               nir_def *cull_mask_and_flags = nir_load_var(b, vars->cull_mask_and_flags);
   struct radv_ray_traversal_args args = {
      .root_bvh_base = root_bvh_base,
   .flags = cull_mask_and_flags,
   .cull_mask = cull_mask_and_flags,
   .origin = nir_load_var(b, vars->origin),
   .tmin = nir_load_var(b, vars->tmin),
   .dir = nir_load_var(b, vars->direction),
   .vars = trav_vars_args,
   .stack_stride = device->physical_device->rt_wave_size * sizeof(uint32_t),
   .stack_entries = MAX_STACK_ENTRY_COUNT,
   .stack_base = 0,
   .ignore_cull_mask = ignore_cull_mask,
   .stack_store_cb = store_stack_entry,
   .stack_load_cb = load_stack_entry,
   .aabb_cb = (pipeline->base.base.create_flags & VK_PIPELINE_CREATE_2_RAY_TRACING_SKIP_AABBS_BIT_KHR)
               .triangle_cb = (pipeline->base.base.create_flags & VK_PIPELINE_CREATE_2_RAY_TRACING_SKIP_TRIANGLES_BIT_KHR)
                                    nir_metadata_preserve(nir_shader_get_entrypoint(b->shader), nir_metadata_none);
            /* Register storage for hit attributes */
            if (!monolithic) {
      for (uint32_t i = 0; i < ARRAY_SIZE(hit_attribs); i++)
                              /* Initialize follow-up shader. */
   nir_push_if(b, nir_load_var(b, trav_vars.hit));
   {
      if (monolithic) {
                              /* should_return is set if we had a hit but we won't be calling the closest hit
   * shader and hence need to return immediately to the calling shader. */
   nir_push_if(b, nir_inot(b, should_return));
   visit_closest_hit_shaders(device, b, pipeline, vars);
      } else {
      for (int i = 0; i < ARRAY_SIZE(hit_attribs); ++i)
         nir_execute_closest_hit_amd(b, nir_load_var(b, vars->idx), nir_load_var(b, vars->tmax),
               }
   nir_push_else(b, NULL);
   {
      if (monolithic) {
                  } else {
      /* Only load the miss shader if we actually miss. It is valid to not specify an SBT pointer
   * for miss shaders if none of the rays miss. */
         }
      }
      nir_shader *
   radv_build_traversal_shader(struct radv_device *device, struct radv_ray_tracing_pipeline *pipeline,
         {
               /* Create the traversal shader as an intersection shader to prevent validation failures due to
   * invalid variable modes.*/
   nir_builder b = radv_meta_init_shader(device, MESA_SHADER_INTERSECTION, "rt_traversal");
   b.shader->info.internal = false;
   b.shader->info.workgroup_size[0] = 8;
   b.shader->info.workgroup_size[1] = device->physical_device->rt_wave_size == 64 ? 8 : 4;
   b.shader->info.shared_size = device->physical_device->rt_wave_size * MAX_STACK_ENTRY_COUNT * sizeof(uint32_t);
            /* initialize trace_ray arguments */
   nir_store_var(&b, vars.accel_struct, nir_load_accel_struct_amd(&b), 1);
   nir_store_var(&b, vars.cull_mask_and_flags, nir_load_cull_mask_and_flags_amd(&b), 0x1);
   nir_store_var(&b, vars.sbt_offset, nir_load_sbt_offset_amd(&b), 0x1);
   nir_store_var(&b, vars.sbt_stride, nir_load_sbt_stride_amd(&b), 0x1);
   nir_store_var(&b, vars.origin, nir_load_ray_world_origin(&b), 0x7);
   nir_store_var(&b, vars.tmin, nir_load_ray_t_min(&b), 0x1);
   nir_store_var(&b, vars.direction, nir_load_ray_world_direction(&b), 0x7);
   nir_store_var(&b, vars.tmax, nir_load_ray_t_max(&b), 0x1);
   nir_store_var(&b, vars.arg, nir_load_rt_arg_scratch_offset_amd(&b), 0x1);
                     /* Deal with all the inline functions. */
   nir_index_ssa_defs(nir_shader_get_entrypoint(b.shader));
            /* Lower and cleanup variables */
   NIR_PASS_V(b.shader, nir_lower_global_vars_to_local);
               }
      struct lower_rt_instruction_monolithic_state {
      struct radv_device *device;
   struct radv_ray_tracing_pipeline *pipeline;
   const struct radv_pipeline_key *key;
               };
      static bool
   lower_rt_instruction_monolithic(nir_builder *b, nir_instr *instr, void *data)
   {
      if (instr->type != nir_instr_type_intrinsic)
                              struct lower_rt_instruction_monolithic_state *state = data;
            switch (intr->intrinsic) {
   case nir_intrinsic_execute_callable:
         case nir_intrinsic_trace_ray: {
               nir_src cull_mask = intr->src[2];
            /* Per the SPIR-V extension spec we have to ignore some bits for some arguments. */
   nir_store_var(b, vars->accel_struct, intr->src[0].ssa, 0x1);
   nir_store_var(b, vars->cull_mask_and_flags, nir_ior(b, nir_ishl_imm(b, cull_mask.ssa, 24), intr->src[1].ssa),
         nir_store_var(b, vars->sbt_offset, nir_iand_imm(b, intr->src[3].ssa, 0xf), 0x1);
   nir_store_var(b, vars->sbt_stride, nir_iand_imm(b, intr->src[4].ssa, 0xf), 0x1);
   nir_store_var(b, vars->miss_index, nir_iand_imm(b, intr->src[5].ssa, 0xffff), 0x1);
   nir_store_var(b, vars->origin, intr->src[6].ssa, 0x7);
   nir_store_var(b, vars->tmin, intr->src[7].ssa, 0x1);
   nir_store_var(b, vars->direction, intr->src[8].ssa, 0x7);
            nir_def *stack_ptr = nir_load_var(b, vars->stack_ptr);
            radv_build_traversal(state->device, state->pipeline, state->pCreateInfo, true, b, vars, ignore_cull_mask);
   b->shader->info.shared_size = MAX2(b->shader->info.shared_size, state->device->physical_device->rt_wave_size *
                     nir_instr_remove(instr);
      }
   case nir_intrinsic_rt_resume:
         case nir_intrinsic_rt_return_amd:
         case nir_intrinsic_execute_closest_hit_amd:
         case nir_intrinsic_execute_miss_amd:
         default:
            }
      static void
   lower_rt_instructions_monolithic(nir_shader *shader, struct radv_device *device,
               {
               struct lower_rt_instruction_monolithic_state state = {
      .device = device,
   .pipeline = pipeline,
   .pCreateInfo = pCreateInfo,
               nir_shader_instructions_pass(shader, lower_rt_instruction_monolithic, nir_metadata_none, &state);
            /* Register storage for hit attributes */
            for (uint32_t i = 0; i < ARRAY_SIZE(hit_attribs); i++)
               }
      /** Select the next shader based on priorities:
   *
   * Detect the priority of the shader stage by the lowest bits in the address (low to high):
   *  - Raygen              - idx 0
   *  - Traversal           - idx 1
   *  - Closest Hit / Miss  - idx 2
   *  - Callable            - idx 3
   *
   *
   * This gives us the following priorities:
   * Raygen       :  Callable  >               >  Traversal  >  Raygen
   * Traversal    :            >  Chit / Miss  >             >  Raygen
   * CHit / Miss  :  Callable  >  Chit / Miss  >  Traversal  >  Raygen
   * Callable     :  Callable  >  Chit / Miss  >             >  Raygen
   */
   static nir_def *
   select_next_shader(nir_builder *b, nir_def *shader_addr, unsigned wave_size)
   {
      gl_shader_stage stage = b->shader->info.stage;
   nir_def *prio = nir_iand_imm(b, shader_addr, radv_rt_priority_mask);
   nir_def *ballot = nir_ballot(b, 1, wave_size, nir_imm_bool(b, true));
   nir_def *ballot_traversal = nir_ballot(b, 1, wave_size, nir_ieq_imm(b, prio, radv_rt_priority_traversal));
   nir_def *ballot_hit_miss = nir_ballot(b, 1, wave_size, nir_ieq_imm(b, prio, radv_rt_priority_hit_miss));
            if (stage != MESA_SHADER_CALLABLE && stage != MESA_SHADER_INTERSECTION)
         if (stage != MESA_SHADER_RAYGEN)
         if (stage != MESA_SHADER_INTERSECTION)
            nir_def *lsb = nir_find_lsb(b, ballot);
   nir_def *next = nir_read_invocation(b, shader_addr, lsb);
      }
      void
   radv_nir_lower_rt_abi(nir_shader *shader, const VkRayTracingPipelineCreateInfoKHR *pCreateInfo,
                     {
                                 if (monolithic)
                     if (stack_size) {
      vars.stack_size = MAX2(vars.stack_size, shader->scratch_size);
      }
                     nir_cf_list list;
            /* initialize variables */
            nir_def *traversal_addr = ac_nir_load_arg(&b, &args->ac, args->ac.rt.traversal_shader_addr);
            nir_def *shader_addr = ac_nir_load_arg(&b, &args->ac, args->ac.rt.shader_addr);
   shader_addr = nir_pack_64_2x32(&b, shader_addr);
            nir_store_var(&b, vars.stack_ptr, ac_nir_load_arg(&b, &args->ac, args->ac.rt.dynamic_callable_stack_base), 1);
   nir_def *record_ptr = ac_nir_load_arg(&b, &args->ac, args->ac.rt.shader_record);
   nir_store_var(&b, vars.shader_record_ptr, nir_pack_64_2x32(&b, record_ptr), 1);
            nir_def *accel_struct = ac_nir_load_arg(&b, &args->ac, args->ac.rt.accel_struct);
   nir_store_var(&b, vars.accel_struct, nir_pack_64_2x32(&b, accel_struct), 1);
   nir_store_var(&b, vars.cull_mask_and_flags, ac_nir_load_arg(&b, &args->ac, args->ac.rt.cull_mask_and_flags), 1);
   nir_store_var(&b, vars.sbt_offset, ac_nir_load_arg(&b, &args->ac, args->ac.rt.sbt_offset), 1);
   nir_store_var(&b, vars.sbt_stride, ac_nir_load_arg(&b, &args->ac, args->ac.rt.sbt_stride), 1);
   nir_store_var(&b, vars.miss_index, ac_nir_load_arg(&b, &args->ac, args->ac.rt.miss_index), 1);
   nir_store_var(&b, vars.origin, ac_nir_load_arg(&b, &args->ac, args->ac.rt.ray_origin), 0x7);
   nir_store_var(&b, vars.tmin, ac_nir_load_arg(&b, &args->ac, args->ac.rt.ray_tmin), 1);
   nir_store_var(&b, vars.direction, ac_nir_load_arg(&b, &args->ac, args->ac.rt.ray_direction), 0x7);
            nir_store_var(&b, vars.primitive_id, ac_nir_load_arg(&b, &args->ac, args->ac.rt.primitive_id), 1);
   nir_def *instance_addr = ac_nir_load_arg(&b, &args->ac, args->ac.rt.instance_addr);
   nir_store_var(&b, vars.instance_addr, nir_pack_64_2x32(&b, instance_addr), 1);
   nir_store_var(&b, vars.geometry_id_and_flags, ac_nir_load_arg(&b, &args->ac, args->ac.rt.geometry_id_and_flags), 1);
            /* guard the shader, so that only the correct invocations execute it */
   nir_if *shader_guard = NULL;
   if (shader->info.stage != MESA_SHADER_RAYGEN || resume_shader) {
      nir_def *uniform_shader_addr = ac_nir_load_arg(&b, &args->ac, args->ac.rt.uniform_shader_addr);
   uniform_shader_addr = nir_pack_64_2x32(&b, uniform_shader_addr);
            shader_guard = nir_push_if(&b, nir_ieq(&b, uniform_shader_addr, shader_addr));
                        if (shader_guard)
                     if (monolithic) {
         } else {
      /* select next shader */
   shader_addr = nir_load_var(&b, vars.shader_addr);
   nir_def *next = select_next_shader(&b, shader_addr, info->wave_size);
            /* store back all variables to registers */
   ac_nir_store_arg(&b, &args->ac, args->ac.rt.dynamic_callable_stack_base, nir_load_var(&b, vars.stack_ptr));
   ac_nir_store_arg(&b, &args->ac, args->ac.rt.shader_addr, shader_addr);
   ac_nir_store_arg(&b, &args->ac, args->ac.rt.shader_record, nir_load_var(&b, vars.shader_record_ptr));
   ac_nir_store_arg(&b, &args->ac, args->ac.rt.payload_offset, nir_load_var(&b, vars.arg));
   ac_nir_store_arg(&b, &args->ac, args->ac.rt.accel_struct, nir_load_var(&b, vars.accel_struct));
   ac_nir_store_arg(&b, &args->ac, args->ac.rt.cull_mask_and_flags, nir_load_var(&b, vars.cull_mask_and_flags));
   ac_nir_store_arg(&b, &args->ac, args->ac.rt.sbt_offset, nir_load_var(&b, vars.sbt_offset));
   ac_nir_store_arg(&b, &args->ac, args->ac.rt.sbt_stride, nir_load_var(&b, vars.sbt_stride));
   ac_nir_store_arg(&b, &args->ac, args->ac.rt.miss_index, nir_load_var(&b, vars.miss_index));
   ac_nir_store_arg(&b, &args->ac, args->ac.rt.ray_origin, nir_load_var(&b, vars.origin));
   ac_nir_store_arg(&b, &args->ac, args->ac.rt.ray_tmin, nir_load_var(&b, vars.tmin));
   ac_nir_store_arg(&b, &args->ac, args->ac.rt.ray_direction, nir_load_var(&b, vars.direction));
            ac_nir_store_arg(&b, &args->ac, args->ac.rt.primitive_id, nir_load_var(&b, vars.primitive_id));
   ac_nir_store_arg(&b, &args->ac, args->ac.rt.instance_addr, nir_load_var(&b, vars.instance_addr));
   ac_nir_store_arg(&b, &args->ac, args->ac.rt.geometry_id_and_flags, nir_load_var(&b, vars.geometry_id_and_flags));
                        /* cleanup passes */
   NIR_PASS_V(shader, nir_lower_global_vars_to_local);
   NIR_PASS_V(shader, nir_lower_vars_to_ssa);
   if (shader->info.stage == MESA_SHADER_CLOSEST_HIT || shader->info.stage == MESA_SHADER_INTERSECTION)
      }
