   /*
   * Copyright © 2019 Intel Corporation
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
   #include "nir_builder.h"
   #include "compiler/brw_nir.h"
   #include "util/mesa-sha1.h"
      #define sizeof_field(type, field) sizeof(((type *)0)->field)
      void
   anv_nir_compute_push_layout(nir_shader *nir,
                                 {
      const struct brw_compiler *compiler = pdevice->compiler;
            bool has_const_ubo = false;
   unsigned push_start = UINT_MAX, push_end = 0;
   nir_foreach_function_impl(impl, nir) {
      nir_foreach_block(block, impl) {
      nir_foreach_instr(instr, block) {
                     nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(instr);
   switch (intrin->intrinsic) {
   case nir_intrinsic_load_ubo:
      if (nir_src_is_const(intrin->src[0]) &&
                     case nir_intrinsic_load_push_constant: {
      unsigned base = nir_intrinsic_base(intrin);
   unsigned range = nir_intrinsic_range(intrin);
   push_start = MIN2(push_start, base);
                     default:
                                    const bool push_ubo_ranges =
      pdevice->info.verx10 >= 75 &&
   has_const_ubo && nir->info.stage != MESA_SHADER_COMPUTE &&
         if (push_ubo_ranges && (robust_flags & BRW_ROBUSTNESS_UBO)) {
      /* We can't on-the-fly adjust our push ranges because doing so would
   * mess up the layout in the shader.  When robustBufferAccess is
   * enabled, we push a mask into the shader indicating which pushed
   * registers are valid and we zero out the invalid ones at the top of
   * the shader.
   */
   const uint32_t push_reg_mask_start =
         const uint32_t push_reg_mask_end = push_reg_mask_start + sizeof(uint64_t);
   push_start = MIN2(push_start, push_reg_mask_start);
               if (nir->info.stage == MESA_SHADER_COMPUTE) {
      /* For compute shaders, we always have to have the subgroup ID.  The
   * back-end compiler will "helpfully" add it for us in the last push
   * constant slot.  Yes, there is an off-by-one error here but that's
   * because the back-end will add it so we want to claim the number of
   * push constants one dword less than the full amount including
   * gl_SubgroupId.
   */
   assert(push_end <= offsetof(struct anv_push_constants, cs.subgroup_id));
               /* Align push_start down to a 32B boundary and make it no larger than
   * push_end (no push constants is indicated by push_start = UINT_MAX).
   */
   push_start = MIN2(push_start, push_end);
            /* For vec4 our push data size needs to be aligned to a vec4 and for
   * scalar, it needs to be aligned to a DWORD.
   */
   const unsigned align = compiler->scalar_stage[nir->info.stage] ? 4 : 16;
   nir->num_uniforms = ALIGN(push_end - push_start, align);
   prog_data->nr_params = nir->num_uniforms / 4;
            struct anv_push_range push_constant_range = {
      .set = ANV_DESCRIPTOR_SET_PUSH_CONSTANTS,
   .start = push_start / 32,
               if (has_push_intrinsic) {
      nir_foreach_function_impl(impl, nir) {
      nir_foreach_block(block, impl) {
      nir_foreach_instr_safe(instr, block) {
                     nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(instr);
   switch (intrin->intrinsic) {
   case nir_intrinsic_load_push_constant: {
      /* With bindless shaders we load uniforms with SEND
   * messages. All the push constants are located after the
   * RT_DISPATCH_GLOBALS. We just need to add the offset to
   * the address right after RT_DISPATCH_GLOBALS (see
   * brw_nir_lower_rt_intrinsics.c).
   */
   unsigned base_offset =
         intrin->intrinsic = nir_intrinsic_load_uniform;
   nir_intrinsic_set_base(intrin,
                           default:
                              if (push_ubo_ranges) {
               /* The vec4 back-end pushes at most 32 regs while the scalar back-end
   * pushes up to 64.  This is primarily because the scalar back-end has a
   * massively more competent register allocator and so the risk of
   * spilling due to UBO pushing isn't nearly as high.
   */
   const unsigned max_push_regs =
            unsigned total_push_regs = push_constant_range.length;
   for (unsigned i = 0; i < 4; i++) {
      if (total_push_regs + prog_data->ubo_ranges[i].length > max_push_regs)
            }
                     if (push_constant_range.length > 0)
            if (robust_flags & BRW_ROBUSTNESS_UBO) {
      const uint32_t push_reg_mask_offset =
         assert(push_reg_mask_offset >= push_start);
   prog_data->push_reg_mask_param =
                        for (int i = 0; i < 4; i++) {
      struct brw_ubo_range *ubo_range = &prog_data->ubo_ranges[i];
                  if (n >= 4 || (n == 3 && compiler->constant_buffer_0_is_relative)) {
      memset(ubo_range, 0, sizeof(*ubo_range));
                              map->push_ranges[n++] = (struct anv_push_range) {
      .set = binding->set,
   .index = binding->index,
   .dynamic_offset_index = binding->dynamic_offset_index,
   .start = ubo_range->start,
               /* We only bother to shader-zero pushed client UBOs */
   if (binding->set < MAX_SETS &&
      (robust_flags & BRW_ROBUSTNESS_UBO)) {
   prog_data->zero_push_reg |= BITFIELD64_RANGE(range_start_reg,
                     } else {
      /* For Ivy Bridge, the push constants packets have a different
   * rule that would require us to iterate in the other direction
   * and possibly mess around with dynamic state base address.
   * Don't bother; just emit regular push constants at n = 0.
   *
   * In the compute case, we don't have multiple push ranges so it's
   * better to just provide one in push_ranges[0].
   */
               /* Now that we're done computing the push constant portion of the
   * bind map, hash it.  This lets us quickly determine if the actual
   * mapping has changed and not just a no-op pipeline change.
   */
   _mesa_sha1_compute(map->push_ranges,
            }
      void
   anv_nir_validate_push_layout(struct brw_stage_prog_data *prog_data,
         {
   #ifndef NDEBUG
      unsigned prog_data_push_size = DIV_ROUND_UP(prog_data->nr_params, 8);
   for (unsigned i = 0; i < 4; i++)
            unsigned bind_map_push_size = 0;
   for (unsigned i = 0; i < 4; i++)
            /* We could go through everything again but it should be enough to assert
   * that they push the same number of registers.  This should alert us if
   * the back-end compiler decides to re-arrange stuff or shrink a range.
   */
      #endif
   }
