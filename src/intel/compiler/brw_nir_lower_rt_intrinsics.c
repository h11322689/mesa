   /*
   * Copyright (c) 2020 Intel Corporation
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
      static nir_def *
   build_leaf_is_procedural(nir_builder *b, struct brw_nir_rt_mem_hit_defs *hit)
   {
      switch (b->shader->info.stage) {
   case MESA_SHADER_ANY_HIT:
      /* Any-hit shaders are always compiled into intersection shaders for
   * procedural geometry.  If we got here in an any-hit shader, it's for
   * triangles.
   */
         case MESA_SHADER_INTERSECTION:
            default:
      return nir_ieq_imm(b, hit->leaf_type,
         }
      static void
   lower_rt_intrinsics_impl(nir_function_impl *impl,
         {
               nir_builder build = nir_builder_at(nir_before_impl(impl));
            struct brw_nir_rt_globals_defs globals;
            nir_def *hotzone_addr = brw_nir_rt_sw_hotzone_addr(b, devinfo);
            gl_shader_stage stage = b->shader->info.stage;
   struct brw_nir_rt_mem_ray_defs world_ray_in = {};
   struct brw_nir_rt_mem_ray_defs object_ray_in = {};
   struct brw_nir_rt_mem_hit_defs hit_in = {};
   switch (stage) {
   case MESA_SHADER_ANY_HIT:
   case MESA_SHADER_CLOSEST_HIT:
   case MESA_SHADER_INTERSECTION:
      brw_nir_rt_load_mem_hit(b, &hit_in,
         brw_nir_rt_load_mem_ray(b, &object_ray_in,
               case MESA_SHADER_MISS:
      brw_nir_rt_load_mem_ray(b, &world_ray_in,
               default:
                  nir_def *thread_stack_base_addr = brw_nir_rt_sw_stack_addr(b, devinfo);
   nir_def *stack_base_offset = nir_channel(b, hotzone, 0);
   nir_def *stack_base_addr =
         ASSERTED bool seen_scratch_base_ptr_load = false;
            nir_foreach_block(block, impl) {
      nir_foreach_instr_safe(instr, block) {
                                       nir_def *sysval = NULL;
   switch (intrin->intrinsic) {
   case nir_intrinsic_load_scratch_base_ptr:
      assert(nir_intrinsic_base(intrin) == 1);
   seen_scratch_base_ptr_load = true;
               case nir_intrinsic_btd_stack_push_intel: {
      int32_t stack_size = nir_intrinsic_stack_size(intrin);
   if (stack_size > 0) {
      nir_def *child_stack_offset =
            }
   nir_instr_remove(instr);
               case nir_intrinsic_rt_resume:
      /* This is the first "interesting" instruction */
   assert(block == nir_start_block(impl));
                  int32_t stack_size = nir_intrinsic_stack_size(intrin);
   if (stack_size > 0) {
      stack_base_offset =
         nir_store_global(b, hotzone_addr, 16, stack_base_offset, 0x1);
   stack_base_addr = nir_iadd(b, thread_stack_base_addr,
      }
               case nir_intrinsic_load_uniform: {
      /* We don't want to lower this in the launch trampoline. */
                  sysval = brw_nir_load_global_const(b, intrin,
                              case nir_intrinsic_load_ray_launch_id:
                  case nir_intrinsic_load_ray_launch_size:
                  case nir_intrinsic_load_ray_world_origin:
                  case nir_intrinsic_load_ray_world_direction:
                  case nir_intrinsic_load_ray_object_origin:
                  case nir_intrinsic_load_ray_object_direction:
                  case nir_intrinsic_load_ray_t_min:
      /* It shouldn't matter which we pull this from */
               case nir_intrinsic_load_ray_t_max:
      if (stage == MESA_SHADER_MISS)
         else
               case nir_intrinsic_load_primitive_id:
      sysval = brw_nir_rt_load_primitive_id_from_hit(b,
                     case nir_intrinsic_load_instance_id: {
      struct brw_nir_rt_bvh_instance_leaf_defs leaf;
   brw_nir_rt_load_bvh_instance_leaf(b, &leaf, hit_in.inst_leaf_ptr);
   sysval = leaf.instance_index;
               case nir_intrinsic_load_ray_object_to_world: {
      struct brw_nir_rt_bvh_instance_leaf_defs leaf;
   brw_nir_rt_load_bvh_instance_leaf(b, &leaf, hit_in.inst_leaf_ptr);
   sysval = leaf.object_to_world[nir_intrinsic_column(intrin)];
               case nir_intrinsic_load_ray_world_to_object: {
      struct brw_nir_rt_bvh_instance_leaf_defs leaf;
   brw_nir_rt_load_bvh_instance_leaf(b, &leaf, hit_in.inst_leaf_ptr);
   sysval = leaf.world_to_object[nir_intrinsic_column(intrin)];
               case nir_intrinsic_load_ray_hit_kind: {
      nir_def *tri_hit_kind =
      nir_bcsel(b, hit_in.front_face,
            sysval = nir_bcsel(b, build_leaf_is_procedural(b, &hit_in),
                     case nir_intrinsic_load_ray_flags:
      /* We need to fetch the original ray flags we stored in the
   * leaf pointer, because the actual ray flags we get here
   * will include any flags passed on the pipeline at creation
   * time, and the spec for IncomingRayFlagsKHR says:
   *   Setting pipeline flags on the raytracing pipeline must not
   *   cause any corresponding flags to be set in variables with
   *   this decoration.
   */
               case nir_intrinsic_load_cull_mask:
                  case nir_intrinsic_load_ray_geometry_index: {
      nir_def *geometry_index_dw =
      nir_load_global(b, nir_iadd_imm(b, hit_in.prim_leaf_ptr, 4), 4,
      sysval = nir_iand_imm(b, geometry_index_dw, BITFIELD_MASK(29));
               case nir_intrinsic_load_ray_instance_custom_index: {
      struct brw_nir_rt_bvh_instance_leaf_defs leaf;
   brw_nir_rt_load_bvh_instance_leaf(b, &leaf, hit_in.inst_leaf_ptr);
   sysval = leaf.instance_id;
               case nir_intrinsic_load_shader_record_ptr:
      /* We can't handle this intrinsic in resume shaders because the
   * handle we get there won't be from the original SBT.  The shader
   * call lowering/splitting pass should have ensured that this
   * value was spilled from the initial shader and unspilled in any
   * resume shaders that need it.
   */
   assert(!found_resume);
               case nir_intrinsic_load_ray_base_mem_addr_intel:
                  case nir_intrinsic_load_ray_hw_stack_size_intel:
                  case nir_intrinsic_load_ray_sw_stack_size_intel:
                  case nir_intrinsic_load_ray_num_dss_rt_stacks_intel:
                  case nir_intrinsic_load_ray_hit_sbt_addr_intel:
                  case nir_intrinsic_load_ray_hit_sbt_stride_intel:
                  case nir_intrinsic_load_ray_miss_sbt_addr_intel:
                  case nir_intrinsic_load_ray_miss_sbt_stride_intel:
                  case nir_intrinsic_load_callable_sbt_addr_intel:
                  case nir_intrinsic_load_callable_sbt_stride_intel:
                  case nir_intrinsic_load_btd_resume_sbt_addr_intel:
      sysval = nir_pack_64_2x32_split(b,
                     case nir_intrinsic_load_leaf_procedural_intel:
                  case nir_intrinsic_load_ray_triangle_vertex_positions: {
      struct brw_nir_rt_bvh_primitive_leaf_positions_defs pos;
   brw_nir_rt_load_bvh_primitive_leaf_positions(b, &pos, hit_in.prim_leaf_ptr);
   sysval = pos.positions[nir_intrinsic_column(intrin)];
               case nir_intrinsic_load_leaf_opaque_intel: {
      if (stage == MESA_SHADER_INTERSECTION) {
      /* In intersection shaders, the opaque bit is passed to us in
   * the front_face bit.
   */
      } else {
      nir_def *flags_dw =
      nir_load_global(b, nir_iadd_imm(b, hit_in.prim_leaf_ptr, 4), 4,
         }
               default:
                           if (sysval) {
      nir_def_rewrite_uses(&intrin->def,
                           nir_metadata_preserve(impl,
                        }
      /** Lower ray-tracing system values and intrinsics
   *
   * In most 3D shader stages, intrinsics are a fairly thin wrapper around
   * hardware functionality and system values represent magic bits that come
   * into the shader from FF hardware.  Ray-tracing, however, looks a bit more
   * like the OpenGL 1.0 world where the underlying hardware is simple and most
   * of the API implementation is software.
   *
   * In particular, most things that are treated as system values (or built-ins
   * in SPIR-V) don't get magically dropped into registers for us.  Instead, we
   * have to fetch them from the relevant data structures shared with the
   * ray-tracing hardware.  Most come from either the RT_DISPATCH_GLOBALS or
   * from one of the MemHit data structures.  Some, such as primitive_id require
   * us to fetch the leaf address from the MemHit struct and then manually read
   * the data out of the BVH.  Instead of trying to emit all this code deep in
   * the back-end where we can't effectively optimize it, we lower it all to
   * global memory access in NIR.
   *
   * Once this pass is complete, the only real system values left are the two
   * argument pointer system values for BTD dispatch: btd_local_arg_addr and
   * btd_global_arg_addr.
   */
   void
   brw_nir_lower_rt_intrinsics(nir_shader *nir,
         {
      nir_foreach_function_impl(impl, nir) {
            }
