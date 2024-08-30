   /*
   * Copyright (c) 2021 Intel Corporation
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
      #include "brw_nir_rt.h"
   #include "brw_nir_rt_builder.h"
      #include "nir_deref.h"
      #include "util/macros.h"
      struct lowering_state {
                        struct hash_table *queries;
            struct brw_nir_rt_globals_defs globals;
      };
      struct brw_ray_query {
      nir_variable *opaque_var;
   nir_variable *internal_var;
      };
      #define SIZEOF_QUERY_STATE (sizeof(uint32_t))
      static bool
   need_spill_fill(struct lowering_state *state)
   {
         }
      /**
   * This pass converts opaque RayQuery structures from SPIRV into a vec3 where
   * the first 2 elements store a global address for the query and the third
   * element is an incremented counter on the number of executed
   * nir_intrinsic_rq_proceed.
   */
      static void
   register_opaque_var(nir_variable *opaque_var, struct lowering_state *state)
   {
      struct hash_entry *entry = _mesa_hash_table_search(state->queries, opaque_var);
            struct brw_ray_query *rq = rzalloc(state->queries, struct brw_ray_query);
   rq->opaque_var = opaque_var;
            unsigned aoa_size = glsl_get_aoa_size(opaque_var->type);
               }
      static void
   create_internal_var(struct brw_ray_query *rq, struct lowering_state *state)
   {
      const struct glsl_type *opaque_type = rq->opaque_var->type;
            while (glsl_type_is_array(opaque_type)) {
      assert(!glsl_type_is_unsized_array(opaque_type));
   internal_type = glsl_array_type(internal_type,
                           rq->internal_var = nir_local_variable_create(state->impl,
            }
            static nir_def *
   get_ray_query_shadow_addr(nir_builder *b,
                     {
      nir_deref_path path;
   nir_deref_path_init(&path, deref, NULL);
            nir_variable *opaque_var = nir_deref_instr_get_variable(path.path[0]);
   struct hash_entry *entry = _mesa_hash_table_search(state->queries, opaque_var);
                     /* Base address in the shadow memory of the variable associated with this
   * ray query variable.
   */
   nir_def *base_addr =
      nir_iadd_imm(b, state->globals.resume_sbt_addr,
         bool spill_fill = need_spill_fill(state);
            if (!spill_fill)
            /* Just emit code and let constant-folding go to town */
   nir_deref_instr **p = &path.path[1];
   for (; *p; p++) {
      if ((*p)->deref_type == nir_deref_type_array) {
                              /**/
                              } else {
                              /* Add the lane offset to the shadow memory address */
   nir_def *lane_offset =
      nir_imul_imm(
      b,
   nir_iadd(
      b,
   nir_imul(
      b,
   brw_load_btd_dss_id(b),
               }
      static void
   update_trace_ctrl_level(nir_builder *b,
                           nir_deref_instr *state_deref,
   {
      nir_def *old_value = nir_load_deref(b, state_deref);
   nir_def *old_ctrl = nir_ishr_imm(b, old_value, 2);
            if (out_old_ctrl)
         if (out_old_level)
            if (new_ctrl)
         if (new_level)
            if (new_ctrl || new_level) {
      if (!new_ctrl)
         if (!new_level)
            nir_def *new_value = nir_ior(b, nir_ishl_imm(b, new_ctrl, 2), new_level);
         }
      static void
   fill_query(nir_builder *b,
            nir_def *hw_stack_addr,
      {
      brw_nir_memcpy_global(b, hw_stack_addr, 64, shadow_stack_addr, 64,
      }
      static void
   spill_query(nir_builder *b,
               {
      brw_nir_memcpy_global(b, shadow_stack_addr, 64, hw_stack_addr, 64,
      }
         static void
   lower_ray_query_intrinsic(nir_builder *b,
               {
                        nir_deref_instr *ctrl_level_deref;
   nir_def *shadow_stack_addr =
         nir_def *hw_stack_addr =
                  switch (intrin->intrinsic) {
   case nir_intrinsic_rq_initialize: {
      nir_def *as_addr = intrin->src[1].ssa;
   nir_def *ray_flags = intrin->src[2].ssa;
   /* From the SPIR-V spec:
   *
   *    "Only the 8 least-significant bits of Cull Mask are used by
   *    this instruction - other bits are ignored.
   *
   *    Only the 16 least-significant bits of Miss Index are used by
   *    this instruction - other bits are ignored."
   */
   nir_def *cull_mask = nir_iand_imm(b, intrin->src[3].ssa, 0xff);
   nir_def *ray_orig = intrin->src[4].ssa;
   nir_def *ray_t_min = intrin->src[5].ssa;
   nir_def *ray_dir = intrin->src[6].ssa;
            nir_def *root_node_ptr =
            struct brw_nir_rt_mem_ray_defs ray_defs = {
      .root_node_ptr = root_node_ptr,
   .ray_flags = nir_u2u16(b, ray_flags),
   .ray_mask = cull_mask,
   .orig = ray_orig,
   .t_near = ray_t_min,
   .dir = ray_dir,
               nir_def *ray_addr =
            brw_nir_rt_query_mark_init(b, stack_addr);
            update_trace_ctrl_level(b, ctrl_level_deref,
                                 case nir_intrinsic_rq_proceed: {
      nir_def *not_done =
                  nir_push_if(b, not_done);
   {
      nir_def *ctrl, *level;
   update_trace_ctrl_level(b, ctrl_level_deref,
                        /* Mark the query as done because handing it over to the HW for
   * processing. If the HW make any progress, it will write back some
   * data and as a side effect, clear the "done" bit. If no progress is
   * made, HW does not write anything back and we can use this bit to
   * detect that.
                                                                        update_trace_ctrl_level(b, ctrl_level_deref,
                           }
   nir_push_else(b, NULL);
   {
         }
   nir_pop_if(b, NULL);
   not_done = nir_if_phi(b, not_done_then, not_done_else);
   nir_def_rewrite_uses(&intrin->def, not_done);
               case nir_intrinsic_rq_confirm_intersection: {
      brw_nir_memcpy_global(b,
                     update_trace_ctrl_level(b, ctrl_level_deref,
                                 case nir_intrinsic_rq_generate_intersection: {
      brw_nir_rt_generate_hit_addr(b, stack_addr, intrin->src[1].ssa);
   update_trace_ctrl_level(b, ctrl_level_deref,
                                 case nir_intrinsic_rq_terminate: {
      brw_nir_rt_query_mark_done(b, stack_addr);
               case nir_intrinsic_rq_load: {
               struct brw_nir_rt_mem_ray_defs world_ray_in = {};
   struct brw_nir_rt_mem_ray_defs object_ray_in = {};
   struct brw_nir_rt_mem_hit_defs hit_in = {};
   brw_nir_rt_load_mem_ray_from_addr(b, &world_ray_in, stack_addr,
         brw_nir_rt_load_mem_ray_from_addr(b, &object_ray_in, stack_addr,
                  nir_def *sysval = NULL;
   switch (nir_intrinsic_ray_query_value(intrin)) {
   case nir_ray_query_value_intersection_type:
      if (committed) {
      /* Values we want to generate :
   *
   * RayQueryCommittedIntersectionNoneEXT = 0U        <= hit_in.valid == false
   * RayQueryCommittedIntersectionTriangleEXT = 1U    <= hit_in.leaf_type == BRW_RT_BVH_NODE_TYPE_QUAD (4)
   * RayQueryCommittedIntersectionGeneratedEXT = 2U   <= hit_in.leaf_type == BRW_RT_BVH_NODE_TYPE_PROCEDURAL (3)
   */
   sysval =
      nir_bcsel(b, nir_ieq_imm(b, hit_in.leaf_type, 4),
      sysval =
      nir_bcsel(b, hit_in.valid,
   } else {
      /* 0 -> triangle, 1 -> AABB */
   sysval =
      nir_b2i32(b,
                     case nir_ray_query_value_intersection_t:
                  case nir_ray_query_value_intersection_instance_custom_index: {
      struct brw_nir_rt_bvh_instance_leaf_defs leaf;
   brw_nir_rt_load_bvh_instance_leaf(b, &leaf, hit_in.inst_leaf_ptr);
   sysval = leaf.instance_id;
               case nir_ray_query_value_intersection_instance_id: {
      struct brw_nir_rt_bvh_instance_leaf_defs leaf;
   brw_nir_rt_load_bvh_instance_leaf(b, &leaf, hit_in.inst_leaf_ptr);
   sysval = leaf.instance_index;
               case nir_ray_query_value_intersection_instance_sbt_index: {
      struct brw_nir_rt_bvh_instance_leaf_defs leaf;
   brw_nir_rt_load_bvh_instance_leaf(b, &leaf, hit_in.inst_leaf_ptr);
   sysval = leaf.contribution_to_hit_group_index;
               case nir_ray_query_value_intersection_geometry_index: {
      nir_def *geometry_index_dw =
      nir_load_global(b, nir_iadd_imm(b, hit_in.prim_leaf_ptr, 4), 4,
      sysval = nir_iand_imm(b, geometry_index_dw, BITFIELD_MASK(29));
               case nir_ray_query_value_intersection_primitive_index:
                  case nir_ray_query_value_intersection_barycentrics:
                  case nir_ray_query_value_intersection_front_face:
                  case nir_ray_query_value_intersection_object_ray_direction:
                  case nir_ray_query_value_intersection_object_ray_origin:
                  case nir_ray_query_value_intersection_object_to_world: {
      struct brw_nir_rt_bvh_instance_leaf_defs leaf;
   brw_nir_rt_load_bvh_instance_leaf(b, &leaf, hit_in.inst_leaf_ptr);
   sysval = leaf.object_to_world[nir_intrinsic_column(intrin)];
               case nir_ray_query_value_intersection_world_to_object: {
      struct brw_nir_rt_bvh_instance_leaf_defs leaf;
   brw_nir_rt_load_bvh_instance_leaf(b, &leaf, hit_in.inst_leaf_ptr);
   sysval = leaf.world_to_object[nir_intrinsic_column(intrin)];
               case nir_ray_query_value_intersection_candidate_aabb_opaque:
                  case nir_ray_query_value_tmin:
                  case nir_ray_query_value_flags:
                  case nir_ray_query_value_world_ray_direction:
                  case nir_ray_query_value_world_ray_origin:
                  case nir_ray_query_value_intersection_triangle_vertex_positions: {
      struct brw_nir_rt_bvh_primitive_leaf_positions_defs pos;
   brw_nir_rt_load_bvh_primitive_leaf_positions(b, &pos, hit_in.prim_leaf_ptr);
   sysval = pos.positions[nir_intrinsic_column(intrin)];
               default:
                  assert(sysval);
   nir_def_rewrite_uses(&intrin->def, sysval);
               default:
            }
      static void
   lower_ray_query_impl(nir_function_impl *impl, struct lowering_state *state)
   {
      nir_builder _b, *b = &_b;
                              nir_foreach_block_safe(block, impl) {
      nir_foreach_instr_safe(instr, block) {
                     nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(instr);
   if (intrin->intrinsic != nir_intrinsic_rq_initialize &&
      intrin->intrinsic != nir_intrinsic_rq_terminate &&
   intrin->intrinsic != nir_intrinsic_rq_proceed &&
   intrin->intrinsic != nir_intrinsic_rq_generate_intersection &&
   intrin->intrinsic != nir_intrinsic_rq_confirm_intersection &&
                                 }
      bool
   brw_nir_lower_ray_queries(nir_shader *shader,
         {
               struct lowering_state state = {
      .devinfo = devinfo,
   .impl = nir_shader_get_entrypoint(shader),
               /* Map all query variable to internal type variables */
   nir_foreach_function_temp_variable(var, state.impl)
         hash_table_foreach(state.queries, entry)
                     if (progress) {
               nir_remove_dead_derefs(shader);
   nir_remove_dead_variables(shader,
                                          }
