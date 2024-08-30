   /*
   * Copyright © 2020 Intel Corporation
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
   #include "nir_phi_builder.h"
      UNUSED static bool
   no_load_scratch_base_ptr_intrinsic(nir_shader *shader)
   {
      nir_foreach_function_impl(impl, shader) {
      nir_foreach_block(block, impl) {
      nir_foreach_instr(instr, block) {
                     nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(instr);
   if (intrin->intrinsic == nir_intrinsic_load_scratch_base_ptr)
                        }
      /** Insert the appropriate return instruction at the end of the shader */
   void
   brw_nir_lower_shader_returns(nir_shader *shader)
   {
               /* Reserve scratch space at the start of the shader's per-thread scratch
   * space for the return BINDLESS_SHADER_RECORD address and data payload.
   * When a shader is called, the calling shader will write the return BSR
   * address in this region of the callee's scratch space.
   *
   * We could also put it at the end of the caller's scratch space.  However,
   * doing this way means that a shader never accesses its caller's scratch
   * space unless given an explicit pointer (such as for ray payloads).  It
   * also makes computing the address easier given that we want to apply an
   * alignment to the scratch offset to ensure we can make alignment
   * assumptions in the called shader.
   *
   * This isn't needed for ray-gen shaders because they end the thread and
   * never return to the calling trampoline shader.
   */
   assert(no_load_scratch_base_ptr_intrinsic(shader));
   if (shader->info.stage != MESA_SHADER_RAYGEN)
                     set_foreach(impl->end_block->predecessors, block_entry) {
      struct nir_block *block = (void *)block_entry->key;
            switch (shader->info.stage) {
   case MESA_SHADER_RAYGEN:
      /* A raygen shader is always the root of the shader call tree.  When
   * it ends, we retire the bindless stack ID and no further shaders
   * will be executed.
   */
   assert(impl->end_block->predecessors->entries == 1);
               case MESA_SHADER_ANY_HIT:
      /* The default action of an any-hit shader is to accept the ray
   * intersection.  Any-hit shaders may have more than one exit.  Only
   * the final "normal" exit will actually need to accept the
   * intersection as any others should come from nir_jump_halt
   * instructions inserted after ignore_ray_intersection or
   * terminate_ray or the like.  However, inserting an accept after
   * the ignore or terminate is safe because it'll get deleted later.
   */
               case MESA_SHADER_CALLABLE:
   case MESA_SHADER_MISS:
   case MESA_SHADER_CLOSEST_HIT:
      /* Callable, miss, and closest-hit shaders don't take any special
   * action at the end.  They simply return back to the previous shader
   * in the call stack.
   */
   assert(impl->end_block->predecessors->entries == 1);
               case MESA_SHADER_INTERSECTION:
                  default:
                     nir_metadata_preserve(impl, nir_metadata_block_index |
      }
      static void
   store_resume_addr(nir_builder *b, nir_intrinsic_instr *call)
   {
      uint32_t call_idx = nir_intrinsic_call_idx(call);
            /* First thing on the called shader's stack is the resume address
   * followed by a pointer to the payload.
   */
   nir_def *resume_record_addr =
      nir_iadd_imm(b, nir_load_btd_resume_sbt_addr_intel(b),
      /* By the time we get here, any remaining shader/function memory
   * pointers have been lowered to SSA values.
   */
   nir_def *payload_addr =
         brw_nir_rt_store_scratch(b, offset, BRW_BTD_STACK_ALIGN,
                     }
      static bool
   lower_shader_trace_ray_instr(struct nir_builder *b, nir_instr *instr, void *data)
   {
               if (instr->type != nir_instr_type_intrinsic)
            /* Leave nir_intrinsic_rt_resume to be lowered by
   * brw_nir_lower_rt_intrinsics()
   */
   nir_intrinsic_instr *call = nir_instr_as_intrinsic(instr);
   if (call->intrinsic != nir_intrinsic_rt_trace_ray)
                              nir_def *as_addr = call->src[0].ssa;
   nir_def *ray_flags = call->src[1].ssa;
   /* From the SPIR-V spec:
   *
   *    "Only the 8 least-significant bits of Cull Mask are used by this
   *    instruction - other bits are ignored.
   *
   *    Only the 4 least-significant bits of SBT Offset and SBT Stride are
   *    used by this instruction - other bits are ignored.
   *
   *    Only the 16 least-significant bits of Miss Index are used by this
   *    instruction - other bits are ignored."
   */
   nir_def *cull_mask = nir_iand_imm(b, call->src[2].ssa, 0xff);
   nir_def *sbt_offset = nir_iand_imm(b, call->src[3].ssa, 0xf);
   nir_def *sbt_stride = nir_iand_imm(b, call->src[4].ssa, 0xf);
   nir_def *miss_index = nir_iand_imm(b, call->src[5].ssa, 0xffff);
   nir_def *ray_orig = call->src[6].ssa;
   nir_def *ray_t_min = call->src[7].ssa;
   nir_def *ray_dir = call->src[8].ssa;
            nir_def *root_node_ptr =
            /* The hardware packet requires an address to the first element of the
   * hit SBT.
   *
   * In order to calculate this, we must multiply the "SBT Offset"
   * provided to OpTraceRay by the SBT stride provided for the hit SBT in
   * the call to vkCmdTraceRay() and add that to the base address of the
   * hit SBT. This stride is not to be confused with the "SBT Stride"
   * provided to OpTraceRay which is in units of this stride. It's a
   * rather terrible overload of the word "stride". The hardware docs
   * calls the SPIR-V stride value the "shader index multiplier" which is
   * a much more sane name.
   */
   nir_def *hit_sbt_stride_B =
         nir_def *hit_sbt_offset_B =
         nir_def *hit_sbt_addr =
      nir_iadd(b, nir_load_ray_hit_sbt_addr_intel(b),
         /* The hardware packet takes an address to the miss BSR. */
   nir_def *miss_sbt_stride_B =
         nir_def *miss_sbt_offset_B =
         nir_def *miss_sbt_addr =
      nir_iadd(b, nir_load_ray_miss_sbt_addr_intel(b),
         struct brw_nir_rt_mem_ray_defs ray_defs = {
      .root_node_ptr = root_node_ptr,
   /* Combine the shader value given to traceRayEXT() with the pipeline
   * creation value VkPipelineCreateFlags.
   */
   .ray_flags = nir_ior_imm(b, nir_u2u16(b, ray_flags), key->pipeline_ray_flags),
   .ray_mask = cull_mask,
   .hit_group_sr_base_ptr = hit_sbt_addr,
   .hit_group_sr_stride = nir_u2u16(b, hit_sbt_stride_B),
   .miss_sr_ptr = miss_sbt_addr,
   .orig = ray_orig,
   .t_near = ray_t_min,
   .dir = ray_dir,
   .t_far = ray_t_max,
   .shader_index_multiplier = sbt_stride,
   /* The instance leaf pointer is unused in the top level BVH traversal
   * since we always start from the root node. We can reuse that field to
   * store the ray_flags handed to traceRayEXT(). This will be reloaded
   * when the shader accesses gl_IncomingRayFlagsEXT (see
   * nir_intrinsic_load_ray_flags brw_nir_lower_rt_intrinsic.c)
   */
      };
            nir_trace_ray_intel(b,
                     nir_load_btd_global_arg_addr_intel(b),
      }
      static bool
   lower_shader_call_instr(struct nir_builder *b, nir_intrinsic_instr *call,
         {
      if (call->intrinsic != nir_intrinsic_rt_execute_callable)
                              nir_def *sbt_offset32 =
      nir_imul(b, call->src[0].ssa,
      nir_def *sbt_addr =
      nir_iadd(b, nir_load_callable_sbt_addr_intel(b),
      brw_nir_btd_spawn(b, sbt_addr);
      }
      bool
   brw_nir_lower_shader_calls(nir_shader *shader, struct brw_bs_prog_key *key)
   {
      bool a = nir_shader_instructions_pass(shader,
                     bool b = nir_shader_intrinsics_pass(shader, lower_shader_call_instr,
                        }
      /** Creates a trivial return shader
   *
   * In most cases this shader doesn't actually do anything. It just needs to
   * return to the caller.
   *
   * By default, our HW has the ability to handle the fact that a shader is not
   * available and will execute the next following shader in the tracing call.
   * For instance, a RAYGEN shader traces a ray, the tracing generates a hit,
   * but there is no ANYHIT shader available. The HW should follow up by
   * execution the CLOSESTHIT shader.
   *
   * This default behavior can be changed through the RT_CTRL register
   * (privileged access) and when NULL shader checks are disabled, the HW will
   * instead call the call stack handler (this shader). This is what i915 is
   * doing as part of Wa_14013202645.
   *
   * In order to ensure the call to the CLOSESTHIT shader, this shader needs to
   * commit the ray and will not proceed with the BTD return. Similarly when the
   * same thing happen with the INTERSECTION shader, we should just carry on the
   * ray traversal with the continue operation.
   *
   */
   nir_shader *
   brw_nir_create_trivial_return_shader(const struct brw_compiler *compiler,
         {
      const nir_shader_compiler_options *nir_options =
            nir_builder _b = nir_builder_init_simple_shader(MESA_SHADER_CALLABLE,
                        ralloc_steal(mem_ctx, b->shader);
            /* Workaround not needed on DG2-G10-C0+ & DG2-G11-B0+ */
   if ((compiler->devinfo->platform == INTEL_PLATFORM_DG2_G10 &&
      compiler->devinfo->revision < 8) ||
   (compiler->devinfo->platform == INTEL_PLATFORM_DG2_G11 &&
   compiler->devinfo->revision < 4)) {
   /* Reserve scratch space at the start of the shader's per-thread scratch
   * space for the return BINDLESS_SHADER_RECORD address and data payload.
   * When a shader is called, the calling shader will write the return BSR
   * address in this region of the callee's scratch space.
   */
                                       nir_def *is_intersection_shader =
         nir_def *is_anyhit_shader =
            nir_def *needs_commit_or_continue =
            nir_push_if(b, needs_commit_or_continue);
   {
                     nir_def *ray_op =
      nir_bcsel(b, is_intersection_shader,
                     nir_trace_ray_intel(b,
                  }
   nir_push_else(b, NULL);
   {
         }
      } else {
                     }
