   /*
   * Copyright © 2022 Konstantin Seurer
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
      #include "util/hash_table.h"
      #include "bvh/bvh.h"
   #include "radv_debug.h"
   #include "radv_nir.h"
   #include "radv_private.h"
   #include "radv_rt_common.h"
   #include "radv_shader.h"
      /* Traversal stack size. Traversal supports backtracking so we can go deeper than this size if
   * needed. However, we keep a large stack size to avoid it being put into registers, which hurts
   * occupancy. */
   #define MAX_SCRATCH_STACK_ENTRY_COUNT 76
      typedef struct {
      nir_variable *variable;
      } rq_variable;
      static rq_variable *
   rq_variable_create(void *ctx, nir_shader *shader, unsigned array_length, const struct glsl_type *type, const char *name)
   {
      rq_variable *result = ralloc(ctx, rq_variable);
            const struct glsl_type *variable_type = type;
   if (array_length != 1)
                        }
      static nir_def *
   nir_load_array(nir_builder *b, nir_variable *array, nir_def *index)
   {
         }
      static void
   nir_store_array(nir_builder *b, nir_variable *array, nir_def *index, nir_def *value, unsigned writemask)
   {
         }
      static nir_deref_instr *
   rq_deref_var(nir_builder *b, nir_def *index, rq_variable *var)
   {
      if (var->array_length == 1)
               }
      static nir_def *
   rq_load_var(nir_builder *b, nir_def *index, rq_variable *var)
   {
      if (var->array_length == 1)
               }
      static void
   rq_store_var(nir_builder *b, nir_def *index, rq_variable *var, nir_def *value, unsigned writemask)
   {
      if (var->array_length == 1) {
         } else {
            }
      static void
   rq_copy_var(nir_builder *b, nir_def *index, rq_variable *dst, rq_variable *src, unsigned mask)
   {
         }
      static nir_def *
   rq_load_array(nir_builder *b, nir_def *index, rq_variable *var, nir_def *array_index)
   {
      if (var->array_length == 1)
            return nir_load_deref(
      }
      static void
   rq_store_array(nir_builder *b, nir_def *index, rq_variable *var, nir_def *array_index, nir_def *value,
         {
      if (var->array_length == 1) {
         } else {
      nir_store_deref(
      b,
   nir_build_deref_array(b, nir_build_deref_array(b, nir_build_deref_var(b, var->variable), index), array_index),
      }
      struct ray_query_traversal_vars {
      rq_variable *origin;
            rq_variable *bvh_base;
   rq_variable *stack;
   rq_variable *top_stack;
   rq_variable *stack_low_watermark;
   rq_variable *current_node;
   rq_variable *previous_node;
   rq_variable *instance_top_node;
      };
      struct ray_query_intersection_vars {
      rq_variable *primitive_id;
   rq_variable *geometry_id_and_flags;
   rq_variable *instance_addr;
   rq_variable *intersection_type;
   rq_variable *opaque;
   rq_variable *frontface;
   rq_variable *sbt_offset_and_flags;
   rq_variable *barycentrics;
      };
      struct ray_query_vars {
      rq_variable *root_bvh_base;
   rq_variable *flags;
   rq_variable *cull_mask;
   rq_variable *origin;
   rq_variable *tmin;
                     struct ray_query_intersection_vars closest;
                     rq_variable *stack;
   uint32_t shared_base;
               };
      #define VAR_NAME(name) strcat(strcpy(ralloc_size(ctx, strlen(base_name) + strlen(name) + 1), base_name), name)
      static struct ray_query_traversal_vars
   init_ray_query_traversal_vars(void *ctx, nir_shader *shader, unsigned array_length, const char *base_name)
   {
                        result.origin = rq_variable_create(ctx, shader, array_length, vec3_type, VAR_NAME("_origin"));
            result.bvh_base = rq_variable_create(ctx, shader, array_length, glsl_uint64_t_type(), VAR_NAME("_bvh_base"));
   result.stack = rq_variable_create(ctx, shader, array_length, glsl_uint_type(), VAR_NAME("_stack"));
   result.top_stack = rq_variable_create(ctx, shader, array_length, glsl_uint_type(), VAR_NAME("_top_stack"));
   result.stack_low_watermark =
         result.current_node = rq_variable_create(ctx, shader, array_length, glsl_uint_type(), VAR_NAME("_current_node"));
   result.previous_node = rq_variable_create(ctx, shader, array_length, glsl_uint_type(), VAR_NAME("_previous_node"));
   result.instance_top_node =
         result.instance_bottom_node =
            }
      static struct ray_query_intersection_vars
   init_ray_query_intersection_vars(void *ctx, nir_shader *shader, unsigned array_length, const char *base_name)
   {
                        result.primitive_id = rq_variable_create(ctx, shader, array_length, glsl_uint_type(), VAR_NAME("_primitive_id"));
   result.geometry_id_and_flags =
         result.instance_addr =
         result.intersection_type =
         result.opaque = rq_variable_create(ctx, shader, array_length, glsl_bool_type(), VAR_NAME("_opaque"));
   result.frontface = rq_variable_create(ctx, shader, array_length, glsl_bool_type(), VAR_NAME("_frontface"));
   result.sbt_offset_and_flags =
         result.barycentrics = rq_variable_create(ctx, shader, array_length, vec2_type, VAR_NAME("_barycentrics"));
               }
      static void
   init_ray_query_vars(nir_shader *shader, unsigned array_length, struct ray_query_vars *dst, const char *base_name,
         {
      void *ctx = dst;
            dst->root_bvh_base = rq_variable_create(dst, shader, array_length, glsl_uint64_t_type(), VAR_NAME("_root_bvh_base"));
   dst->flags = rq_variable_create(dst, shader, array_length, glsl_uint_type(), VAR_NAME("_flags"));
   dst->cull_mask = rq_variable_create(dst, shader, array_length, glsl_uint_type(), VAR_NAME("_cull_mask"));
   dst->origin = rq_variable_create(dst, shader, array_length, vec3_type, VAR_NAME("_origin"));
   dst->tmin = rq_variable_create(dst, shader, array_length, glsl_float_type(), VAR_NAME("_tmin"));
                     dst->closest = init_ray_query_intersection_vars(dst, shader, array_length, VAR_NAME("_closest"));
                     uint32_t workgroup_size =
         uint32_t shared_stack_entries = shader->info.ray_queries == 1 ? 16 : 8;
   uint32_t shared_stack_size = workgroup_size * shared_stack_entries * 4;
   uint32_t shared_offset = align(shader->info.shared_size, 4);
   if (shader->info.stage != MESA_SHADER_COMPUTE || array_length > 1 ||
      shared_offset + shared_stack_size > max_shared_size) {
   dst->stack =
      rq_variable_create(dst, shader, array_length,
         } else {
      dst->stack = NULL;
   dst->shared_base = shared_offset;
                  }
      #undef VAR_NAME
      static void
   lower_ray_query(nir_shader *shader, nir_variable *ray_query, struct hash_table *ht, uint32_t max_shared_size)
   {
               unsigned array_length = 1;
   if (glsl_type_is_array(ray_query->type))
                        }
      static void
   copy_candidate_to_closest(nir_builder *b, nir_def *index, struct ray_query_vars *vars)
   {
      rq_copy_var(b, index, vars->closest.barycentrics, vars->candidate.barycentrics, 0x3);
   rq_copy_var(b, index, vars->closest.geometry_id_and_flags, vars->candidate.geometry_id_and_flags, 0x1);
   rq_copy_var(b, index, vars->closest.instance_addr, vars->candidate.instance_addr, 0x1);
   rq_copy_var(b, index, vars->closest.intersection_type, vars->candidate.intersection_type, 0x1);
   rq_copy_var(b, index, vars->closest.opaque, vars->candidate.opaque, 0x1);
   rq_copy_var(b, index, vars->closest.frontface, vars->candidate.frontface, 0x1);
   rq_copy_var(b, index, vars->closest.sbt_offset_and_flags, vars->candidate.sbt_offset_and_flags, 0x1);
   rq_copy_var(b, index, vars->closest.primitive_id, vars->candidate.primitive_id, 0x1);
      }
      static void
   insert_terminate_on_first_hit(nir_builder *b, nir_def *index, struct ray_query_vars *vars,
         {
      nir_def *terminate_on_first_hit;
   if (ray_flags)
         else
      terminate_on_first_hit =
      nir_push_if(b, terminate_on_first_hit);
   {
      rq_store_var(b, index, vars->incomplete, nir_imm_false(b), 0x1);
   if (break_on_terminate)
      }
      }
      static void
   lower_rq_confirm_intersection(nir_builder *b, nir_def *index, nir_intrinsic_instr *instr, struct ray_query_vars *vars)
   {
      copy_candidate_to_closest(b, index, vars);
      }
      static void
   lower_rq_generate_intersection(nir_builder *b, nir_def *index, nir_intrinsic_instr *instr, struct ray_query_vars *vars)
   {
      nir_push_if(b, nir_iand(b, nir_fge(b, rq_load_var(b, index, vars->closest.t), instr->src[1].ssa),
         {
      copy_candidate_to_closest(b, index, vars);
   insert_terminate_on_first_hit(b, index, vars, NULL, false);
      }
      }
      enum rq_intersection_type { intersection_type_none, intersection_type_triangle, intersection_type_aabb };
      static void
   lower_rq_initialize(nir_builder *b, nir_def *index, nir_intrinsic_instr *instr, struct ray_query_vars *vars,
         {
      rq_store_var(b, index, vars->flags, instr->src[2].ssa, 0x1);
            rq_store_var(b, index, vars->origin, instr->src[4].ssa, 0x7);
                     rq_store_var(b, index, vars->direction, instr->src[6].ssa, 0x7);
            rq_store_var(b, index, vars->closest.t, instr->src[7].ssa, 0x1);
                     /* Make sure that instance data loads don't hang in case of a miss by setting a valid initial address. */
   rq_store_var(b, index, vars->closest.instance_addr, accel_struct, 1);
            nir_def *bvh_offset = nir_build_load_global(
      b, 1, 32, nir_iadd_imm(b, accel_struct, offsetof(struct radv_accel_struct_header, bvh_offset)),
      nir_def *bvh_base = nir_iadd(b, accel_struct, nir_u2u64(b, bvh_offset));
            rq_store_var(b, index, vars->root_bvh_base, bvh_base, 0x1);
            if (vars->stack) {
      rq_store_var(b, index, vars->trav.stack, nir_imm_int(b, 0), 0x1);
      } else {
      nir_def *base_offset = nir_imul_imm(b, nir_load_local_invocation_index(b), sizeof(uint32_t));
   base_offset = nir_iadd_imm(b, base_offset, vars->shared_base);
   rq_store_var(b, index, vars->trav.stack, base_offset, 0x1);
               rq_store_var(b, index, vars->trav.current_node, nir_imm_int(b, RADV_BVH_ROOT_NODE), 0x1);
   rq_store_var(b, index, vars->trav.previous_node, nir_imm_int(b, RADV_BVH_INVALID_NODE), 0x1);
   rq_store_var(b, index, vars->trav.instance_top_node, nir_imm_int(b, RADV_BVH_INVALID_NODE), 0x1);
                                 }
      static nir_def *
   lower_rq_load(nir_builder *b, nir_def *index, nir_intrinsic_instr *instr, struct ray_query_vars *vars)
   {
      bool committed = nir_intrinsic_committed(instr);
                     nir_ray_query_value value = nir_intrinsic_ray_query_value(instr);
   switch (value) {
   case nir_ray_query_value_flags:
         case nir_ray_query_value_intersection_barycentrics:
         case nir_ray_query_value_intersection_candidate_aabb_opaque:
      return nir_iand(b, rq_load_var(b, index, vars->candidate.opaque),
      case nir_ray_query_value_intersection_front_face:
         case nir_ray_query_value_intersection_geometry_index:
         case nir_ray_query_value_intersection_instance_custom_index: {
      nir_def *instance_node_addr = rq_load_var(b, index, intersection->instance_addr);
   return nir_iand_imm(
      b,
   nir_build_load_global(
      b, 1, 32,
      }
   case nir_ray_query_value_intersection_instance_id: {
      nir_def *instance_node_addr = rq_load_var(b, index, intersection->instance_addr);
   return nir_build_load_global(
      }
   case nir_ray_query_value_intersection_instance_sbt_index:
         case nir_ray_query_value_intersection_object_ray_direction: {
      nir_def *instance_node_addr = rq_load_var(b, index, intersection->instance_addr);
   nir_def *wto_matrix[3];
   nir_build_wto_matrix_load(b, instance_node_addr, wto_matrix);
      }
   case nir_ray_query_value_intersection_object_ray_origin: {
      nir_def *instance_node_addr = rq_load_var(b, index, intersection->instance_addr);
   nir_def *wto_matrix[3];
   nir_build_wto_matrix_load(b, instance_node_addr, wto_matrix);
      }
   case nir_ray_query_value_intersection_object_to_world: {
      nir_def *instance_node_addr = rq_load_var(b, index, intersection->instance_addr);
   nir_def *rows[3];
   for (unsigned r = 0; r < 3; ++r)
      rows[r] = nir_build_load_global(
               return nir_vec3(b, nir_channel(b, rows[0], column), nir_channel(b, rows[1], column),
      }
   case nir_ray_query_value_intersection_primitive_index:
         case nir_ray_query_value_intersection_t:
         case nir_ray_query_value_intersection_type: {
      nir_def *intersection_type = rq_load_var(b, index, intersection->intersection_type);
   if (!committed)
               }
   case nir_ray_query_value_intersection_world_to_object: {
               nir_def *wto_matrix[3];
            nir_def *vals[3];
   for (unsigned i = 0; i < 3; ++i)
               }
   case nir_ray_query_value_tmin:
         case nir_ray_query_value_world_ray_direction:
         case nir_ray_query_value_world_ray_origin:
         default:
                     }
      struct traversal_data {
      struct ray_query_vars *vars;
      };
      static void
   handle_candidate_aabb(nir_builder *b, struct radv_leaf_intersection *intersection,
         {
      struct traversal_data *data = args->data;
   struct ray_query_vars *vars = data->vars;
            rq_store_var(b, index, vars->candidate.primitive_id, intersection->primitive_id, 1);
   rq_store_var(b, index, vars->candidate.geometry_id_and_flags, intersection->geometry_id_and_flags, 1);
   rq_store_var(b, index, vars->candidate.opaque, intersection->opaque, 0x1);
               }
      static void
   handle_candidate_triangle(nir_builder *b, struct radv_triangle_intersection *intersection,
         {
      struct traversal_data *data = args->data;
   struct ray_query_vars *vars = data->vars;
            rq_store_var(b, index, vars->candidate.barycentrics, intersection->barycentrics, 3);
   rq_store_var(b, index, vars->candidate.primitive_id, intersection->base.primitive_id, 1);
   rq_store_var(b, index, vars->candidate.geometry_id_and_flags, intersection->base.geometry_id_and_flags, 1);
   rq_store_var(b, index, vars->candidate.t, intersection->t, 0x1);
   rq_store_var(b, index, vars->candidate.opaque, intersection->base.opaque, 0x1);
   rq_store_var(b, index, vars->candidate.frontface, intersection->frontface, 0x1);
            nir_push_if(b, intersection->base.opaque);
   {
      copy_candidate_to_closest(b, index, vars);
      }
   nir_push_else(b, NULL);
   {
         }
      }
      static void
   store_stack_entry(nir_builder *b, nir_def *index, nir_def *value, const struct radv_ray_traversal_args *args)
   {
      struct traversal_data *data = args->data;
   if (data->vars->stack)
         else
      }
      static nir_def *
   load_stack_entry(nir_builder *b, nir_def *index, const struct radv_ray_traversal_args *args)
   {
      struct traversal_data *data = args->data;
   if (data->vars->stack)
         else
      }
      static nir_def *
   lower_rq_proceed(nir_builder *b, nir_def *index, nir_intrinsic_instr *instr, struct ray_query_vars *vars,
         {
               bool ignore_cull_mask = false;
   if (nir_block_dominates(vars->initialize->instr.block, instr->instr.block)) {
      nir_src cull_mask = vars->initialize->src[3];
   if (nir_src_is_const(cull_mask) && nir_src_as_uint(cull_mask) == 0xFF)
               nir_variable *inv_dir = nir_local_variable_create(b->impl, glsl_vector_type(GLSL_TYPE_FLOAT, 3), "inv_dir");
            struct radv_ray_traversal_vars trav_vars = {
      .tmax = rq_deref_var(b, index, vars->closest.t),
   .origin = rq_deref_var(b, index, vars->trav.origin),
   .dir = rq_deref_var(b, index, vars->trav.direction),
   .inv_dir = nir_build_deref_var(b, inv_dir),
   .bvh_base = rq_deref_var(b, index, vars->trav.bvh_base),
   .stack = rq_deref_var(b, index, vars->trav.stack),
   .top_stack = rq_deref_var(b, index, vars->trav.top_stack),
   .stack_low_watermark = rq_deref_var(b, index, vars->trav.stack_low_watermark),
   .current_node = rq_deref_var(b, index, vars->trav.current_node),
   .previous_node = rq_deref_var(b, index, vars->trav.previous_node),
   .instance_top_node = rq_deref_var(b, index, vars->trav.instance_top_node),
   .instance_bottom_node = rq_deref_var(b, index, vars->trav.instance_bottom_node),
   .instance_addr = rq_deref_var(b, index, vars->candidate.instance_addr),
               struct traversal_data data = {
      .vars = vars,
               struct radv_ray_traversal_args args = {
      .root_bvh_base = rq_load_var(b, index, vars->root_bvh_base),
   .flags = rq_load_var(b, index, vars->flags),
   .cull_mask = rq_load_var(b, index, vars->cull_mask),
   .origin = rq_load_var(b, index, vars->origin),
   .tmin = rq_load_var(b, index, vars->tmin),
   .dir = rq_load_var(b, index, vars->direction),
   .vars = trav_vars,
   .stack_entries = vars->stack_entries,
   .ignore_cull_mask = ignore_cull_mask,
   .stack_store_cb = store_stack_entry,
   .stack_load_cb = load_stack_entry,
   .aabb_cb = handle_candidate_aabb,
   .triangle_cb = handle_candidate_triangle,
               if (vars->stack) {
      args.stack_stride = 1;
      } else {
      uint32_t workgroup_size =
         args.stack_stride = workgroup_size * 4;
               nir_push_if(b, rq_load_var(b, index, vars->incomplete));
   {
      nir_def *incomplete = radv_build_ray_traversal(device, b, &args);
      }
               }
      static void
   lower_rq_terminate(nir_builder *b, nir_def *index, nir_intrinsic_instr *instr, struct ray_query_vars *vars)
   {
         }
      bool
   radv_nir_lower_ray_queries(struct nir_shader *shader, struct radv_device *device)
   {
      bool progress = false;
            nir_foreach_variable_in_list (var, &shader->variables) {
      if (!var->data.ray_query)
                                 nir_foreach_function (function, shader) {
      if (!function->impl)
                     nir_foreach_variable_in_list (var, &function->impl->locals) {
                                          nir_foreach_block (block, function->impl) {
      nir_foreach_instr_safe (instr, block) {
                                                            if (ray_query_deref->deref_type == nir_deref_type_array) {
                                                                  switch (intrinsic->intrinsic) {
   case nir_intrinsic_rq_confirm_intersection:
      lower_rq_confirm_intersection(&builder, index, intrinsic, vars);
      case nir_intrinsic_rq_generate_intersection:
      lower_rq_generate_intersection(&builder, index, intrinsic, vars);
      case nir_intrinsic_rq_initialize:
      lower_rq_initialize(&builder, index, intrinsic, vars, device->instance);
      case nir_intrinsic_rq_load:
      new_dest = lower_rq_load(&builder, index, intrinsic, vars);
      case nir_intrinsic_rq_proceed:
      new_dest = lower_rq_proceed(&builder, index, intrinsic, vars, device);
      case nir_intrinsic_rq_terminate:
      lower_rq_terminate(&builder, index, intrinsic, vars);
      default:
                                                                                       }
