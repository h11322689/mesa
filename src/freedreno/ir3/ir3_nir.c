   /*
   * Copyright (C) 2015 Rob Clark <robclark@freedesktop.org>
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
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   * SOFTWARE.
   *
   * Authors:
   *    Rob Clark <robclark@freedesktop.org>
   */
      #include "util/u_debug.h"
   #include "util/u_math.h"
      #include "ir3_compiler.h"
   #include "ir3_nir.h"
   #include "ir3_shader.h"
      static bool
   ir3_nir_should_vectorize_mem(unsigned align_mul, unsigned align_offset,
                     {
      struct ir3_compiler *compiler = data;
            /* Don't vectorize load_ssbo's that we could otherwise lower to isam,
   * as the tex cache benefit outweighs the benefit of vectorizing
   */
   if ((low->intrinsic == nir_intrinsic_load_ssbo) &&
      (nir_intrinsic_access(low) & ACCESS_CAN_REORDER) &&
   compiler->has_isam_ssbo) {
               if (low->intrinsic != nir_intrinsic_load_ubo) {
      return bit_size <= 32 && align_mul >= byte_size &&
      align_offset % byte_size == 0 &&
            assert(bit_size >= 8);
   if (bit_size != 32)
                     /* Don't care about alignment past vec4. */
   assert(util_is_power_of_two_nonzero(align_mul));
   align_mul = MIN2(align_mul, 16);
            /* Our offset alignment should aways be at least 4 bytes */
   if (align_mul < 4)
            unsigned worst_start_offset = 16 - align_mul + align_offset;
   if (worst_start_offset + size > 16)
               }
      #define OPT(nir, pass, ...)                                                    \
      ({                                                                          \
      bool this_progress = false;                                              \
   NIR_PASS(this_progress, nir, pass, ##__VA_ARGS__);                       \
            #define OPT_V(nir, pass, ...) NIR_PASS_V(nir, pass, ##__VA_ARGS__)
      void
   ir3_optimize_loop(struct ir3_compiler *compiler, nir_shader *s)
   {
               bool progress;
   unsigned lower_flrp = (s->options->lower_flrp16 ? 16 : 0) |
                  do {
               OPT_V(s, nir_lower_vars_to_ssa);
   progress |= OPT(s, nir_lower_alu_to_scalar, NULL, NULL);
            progress |= OPT(s, nir_copy_prop);
   progress |= OPT(s, nir_opt_deref);
   progress |= OPT(s, nir_opt_dce);
            progress |= OPT(s, nir_opt_find_array_copies);
   progress |= OPT(s, nir_opt_copy_prop_vars);
            static int gcm = -1;
   if (gcm == -1)
         if (gcm == 1)
         else if (gcm == 2)
         progress |= OPT(s, nir_opt_peephole_select, 16, true, true);
   progress |= OPT(s, nir_opt_intrinsics);
   /* NOTE: GS lowering inserts an output var with varying slot that
   * is larger than VARYING_SLOT_MAX (ie. GS_VERTEX_FLAGS_IR3),
   * which triggers asserts in nir_shader_gather_info().  To work
   * around that skip lowering phi precision for GS.
   *
   * Calling nir_shader_gather_info() late also seems to cause
   * problems for tess lowering, for now since we only enable
   * fp16/int16 for frag and compute, skip phi precision lowering
   * for other stages.
   */
   if ((s->info.stage == MESA_SHADER_FRAGMENT) ||
      (s->info.stage == MESA_SHADER_COMPUTE) ||
   (s->info.stage == MESA_SHADER_KERNEL)) {
      }
   progress |= OPT(s, nir_opt_algebraic);
   progress |= OPT(s, nir_lower_alu);
   progress |= OPT(s, nir_lower_pack);
            static const nir_opt_offsets_options offset_options = {
      /* How large an offset we can encode in the instr's immediate field.
                  /* STL/LDL have 13b for offset with MSB being a sign bit, but this opt
   * doesn't deal with negative offsets.
                     };
            nir_load_store_vectorize_options vectorize_opts = {
      .modes = nir_var_mem_ubo | nir_var_mem_ssbo,
   .callback = ir3_nir_should_vectorize_mem,
   .robust_modes = compiler->options.robust_buffer_access2 ?
            };
            if (lower_flrp != 0) {
      if (OPT(s, nir_lower_flrp, lower_flrp, false /* always_precise */)) {
      OPT(s, nir_opt_constant_folding);
               /* Nothing should rematerialize any flrps, so we only
   * need to do this lowering once.
   */
               progress |= OPT(s, nir_opt_dead_cf);
   if (OPT(s, nir_opt_trivial_continues)) {
      progress |= true;
   /* If nir_opt_trivial_continues makes progress, then we need to clean
   * things up if we want any hope of nir_opt_if or nir_opt_loop_unroll
   * to make progress.
   */
   OPT(s, nir_copy_prop);
      }
   progress |= OPT(s, nir_opt_if, nir_opt_if_optimize_phi_true_false);
   progress |= OPT(s, nir_opt_loop_unroll);
   progress |= OPT(s, nir_lower_64bit_phis);
   progress |= OPT(s, nir_opt_remove_phis);
                  }
      static bool
   should_split_wrmask(const nir_instr *instr, const void *data)
   {
               switch (intr->intrinsic) {
   case nir_intrinsic_store_ssbo:
   case nir_intrinsic_store_shared:
   case nir_intrinsic_store_global:
   case nir_intrinsic_store_scratch:
         default:
            }
      static bool
   ir3_nir_lower_ssbo_size_filter(const nir_instr *instr, const void *data)
   {
      return instr->type == nir_instr_type_intrinsic &&
            }
      static nir_def *
   ir3_nir_lower_ssbo_size_instr(nir_builder *b, nir_instr *instr, void *data)
   {
      uint8_t ssbo_size_to_bytes_shift = *(uint8_t *) data;
   nir_intrinsic_instr *intr = nir_instr_as_intrinsic(instr);
      }
      static bool
   ir3_nir_lower_ssbo_size(nir_shader *s, uint8_t ssbo_size_to_bytes_shift)
   {
      return nir_shader_lower_instructions(s, ir3_nir_lower_ssbo_size_filter,
            }
      void
   ir3_nir_lower_io_to_temporaries(nir_shader *s)
   {
      /* Outputs consumed by the VPC, VS inputs, and FS outputs are all handled
   * by the hardware pre-loading registers at the beginning and then reading
   * them at the end, so we can't access them indirectly except through
   * normal register-indirect accesses, and therefore ir3 doesn't support
   * indirect accesses on those. Other i/o is lowered in ir3_nir_lower_tess,
   * and indirects work just fine for those. GS outputs may be consumed by
   * VPC, but have their own lowering in ir3_nir_lower_gs() which does
   * something similar to nir_lower_io_to_temporaries so we shouldn't need
   * to lower them.
   *
   * Note: this might be a little inefficient for VS or TES outputs which are
   * when the next stage isn't an FS, but it probably don't make sense to
   * depend on the next stage before variant creation.
   *
   * TODO: for gallium, mesa/st also does some redundant lowering, including
   * running this pass for GS inputs/outputs which we don't want but not
   * including TES outputs or FS inputs which we do need. We should probably
   * stop doing that once we're sure all drivers are doing their own
   * indirect i/o lowering.
   */
   bool lower_input = s->info.stage == MESA_SHADER_VERTEX ||
         bool lower_output = s->info.stage != MESA_SHADER_TESS_CTRL &&
         if (lower_input || lower_output) {
      NIR_PASS_V(s, nir_lower_io_to_temporaries, nir_shader_get_entrypoint(s),
            /* nir_lower_io_to_temporaries() creates global variables and copy
   * instructions which need to be cleaned up.
   */
   NIR_PASS_V(s, nir_split_var_copies);
   NIR_PASS_V(s, nir_lower_var_copies);
               /* Regardless of the above, we need to lower indirect references to
   * compact variables such as clip/cull distances because due to how
   * TCS<->TES IO works we cannot handle indirect accesses that "straddle"
   * vec4 components. nir_lower_indirect_derefs has a special case for
   * compact variables, so it will actually lower them even though we pass
   * in 0 modes.
   *
   * Using temporaries would be slightly better but
   * nir_lower_io_to_temporaries currently doesn't support TCS i/o.
   */
      }
      /**
   * Inserts an add of 0.5 to floating point array index values in texture coordinates.
   */
   static bool
   ir3_nir_lower_array_sampler_cb(struct nir_builder *b, nir_instr *instr, void *_data)
   {
      if (instr->type != nir_instr_type_tex)
            nir_tex_instr *tex = nir_instr_as_tex(instr);
   if (!tex->is_array || tex->op == nir_texop_lod)
            int coord_idx = nir_tex_instr_src_index(tex, nir_tex_src_coord);
   if (coord_idx == -1 ||
      nir_tex_instr_src_type(tex, coord_idx) != nir_type_float)
                  unsigned ncomp = tex->coord_components;
            assume(ncomp >= 1);
   nir_def *ai = nir_channel(b, src, ncomp - 1);
   ai = nir_fadd_imm(b, ai, 0.5);
   nir_src_rewrite(&tex->src[coord_idx].src,
            }
      static bool
   ir3_nir_lower_array_sampler(nir_shader *shader)
   {
      return nir_shader_instructions_pass(
      shader, ir3_nir_lower_array_sampler_cb,
   }
      void
   ir3_finalize_nir(struct ir3_compiler *compiler, nir_shader *s)
   {
               struct nir_lower_tex_options tex_options = {
      .lower_rect = 0,
   .lower_tg4_offsets = true,
   .lower_invalid_implicit_lod = true,
               if (compiler->gen >= 4) {
      /* a4xx seems to have *no* sam.p */
      } else {
      /* a3xx just needs to avoid sam.p for 3d tex */
               if (ir3_shader_debug & IR3_DBG_DISASM) {
      mesa_logi("----------------------");
   nir_log_shaderi(s);
               if (s->info.stage == MESA_SHADER_GEOMETRY)
            NIR_PASS_V(s, nir_lower_frexp);
                     OPT_V(s, nir_lower_tex, &tex_options);
            if (compiler->array_index_add_half)
                              /* do idiv lowering after first opt loop to get a chance to propagate
   * constants for divide by immed power-of-two:
   */
   nir_lower_idiv_options idiv_options = {
         };
   bool idiv_progress = OPT(s, nir_opt_idiv_const, 8);
            if (idiv_progress)
                     if (ir3_shader_debug & IR3_DBG_DISASM) {
      mesa_logi("----------------------");
   nir_log_shaderi(s);
               /* st_program.c's parameter list optimization requires that future nir
   * variants don't reallocate the uniform storage, so we have to remove
   * uniforms that occupy storage.  But we don't want to remove samplers,
   * because they're needed for YUV variant lowering.
   */
   nir_foreach_uniform_variable_safe (var, s) {
      if (var->data.mode == nir_var_uniform &&
      (glsl_type_get_image_count(var->type) ||
                  }
               }
      static bool
   lower_subgroup_id_filter(const nir_instr *instr, const void *unused)
   {
               if (instr->type != nir_instr_type_intrinsic)
            nir_intrinsic_instr *intr = nir_instr_as_intrinsic(instr);
   return intr->intrinsic == nir_intrinsic_load_subgroup_invocation ||
            }
      static nir_def *
   lower_subgroup_id(nir_builder *b, nir_instr *instr, void *unused)
   {
               nir_intrinsic_instr *intr = nir_instr_as_intrinsic(instr);
   if (intr->intrinsic == nir_intrinsic_load_subgroup_invocation) {
      return nir_iand(
      b, nir_load_local_invocation_index(b),
   } else if (intr->intrinsic == nir_intrinsic_load_subgroup_id) {
      return nir_ishr(b, nir_load_local_invocation_index(b),
      } else {
      assert(intr->intrinsic == nir_intrinsic_load_num_subgroups);
   /* If the workgroup size is constant,
   * nir_lower_compute_system_values() will replace local_size with a
   * constant so this can mostly be constant folded away.
   */
   nir_def *local_size = nir_load_workgroup_size(b);
   nir_def *size =
      nir_imul24(b, nir_channel(b, local_size, 0),
            nir_def *one = nir_imm_int(b, 1);
   return nir_iadd(b, one,
               }
      static bool
   ir3_nir_lower_subgroup_id_cs(nir_shader *shader)
   {
      return nir_shader_lower_instructions(shader, lower_subgroup_id_filter,
      }
      /**
   * Late passes that need to be done after pscreen->finalize_nir()
   */
   void
   ir3_nir_post_finalize(struct ir3_shader *shader)
   {
      struct nir_shader *s = shader->nir;
                     NIR_PASS_V(s, nir_lower_io, nir_var_shader_in | nir_var_shader_out,
            if (s->info.stage == MESA_SHADER_FRAGMENT) {
      /* NOTE: lower load_barycentric_at_sample first, since it
   * produces load_barycentric_at_offset:
   */
   NIR_PASS_V(s, ir3_nir_lower_load_barycentric_at_sample);
   NIR_PASS_V(s, ir3_nir_lower_load_barycentric_at_offset);
   NIR_PASS_V(s, ir3_nir_move_varying_inputs);
   NIR_PASS_V(s, nir_lower_fb_read);
               if (compiler->gen >= 6 && s->info.stage == MESA_SHADER_FRAGMENT &&
      !(ir3_shader_debug & IR3_DBG_NOFP16)) {
   /* Lower FS mediump inputs to 16-bit. If you declared it mediump, you
   * probably want 16-bit instructions (and have set
   * mediump/RelaxedPrecision on most of the rest of the shader's
   * instructions).  If we don't lower it in NIR, then comparisons of the
   * results of mediump ALU ops with the mediump input will happen in highp,
   * causing extra conversions (and, incidentally, causing
   * dEQP-GLES2.functional.shaders.algorithm.rgb_to_hsl_fragment on ANGLE to
   * fail)
   *
   * However, we can't do flat inputs because flat.b doesn't have the
   * destination type for how to downconvert the
   * 32-bit-in-the-varyings-interpolator value. (also, even if it did, watch
   * out for how gl_nir_lower_packed_varyings packs all flat-interpolated
   * things together as ivec4s, so when we lower a formerly-float input
   * you'd end up with an incorrect f2f16(i2i32(load_input())) instead of
   * load_input).
   */
   uint64_t mediump_varyings = 0;
   nir_foreach_shader_in_variable(var, s) {
      if ((var->data.precision == GLSL_PRECISION_MEDIUM ||
      var->data.precision == GLSL_PRECISION_LOW) &&
   var->data.interpolation != INTERP_MODE_FLAT) {
                  if (mediump_varyings) {
      NIR_PASS_V(s, nir_lower_mediump_io,
            nir_var_shader_in,
            /* This should come after input lowering, to opportunistically lower non-mediump outputs. */
               {
      /* If the API-facing subgroup size is forced to a particular value, lower
   * it here. Beyond this point nir_intrinsic_load_subgroup_size will return
   * the "real" subgroup size.
   */
   unsigned subgroup_size = 0, max_subgroup_size = 0;
   switch (shader->options.api_wavesize) {
   case IR3_SINGLE_ONLY:
      subgroup_size = max_subgroup_size = compiler->threadsize_base;
      case IR3_DOUBLE_ONLY:
      subgroup_size = max_subgroup_size = compiler->threadsize_base * 2;
      case IR3_SINGLE_OR_DOUBLE:
      /* For vertex stages, we know the wavesize will never be doubled.
   * Lower subgroup_size here, to avoid having to deal with it when
   * translating from NIR. Otherwise use the "real" wavesize obtained as
   * a driver param.
   */
   if (s->info.stage != MESA_SHADER_COMPUTE &&
      s->info.stage != MESA_SHADER_FRAGMENT) {
      } else {
      subgroup_size = 0;
      }
               nir_lower_subgroups_options options = {
         .subgroup_size = subgroup_size,
   .ballot_bit_size = 32,
   .ballot_components = max_subgroup_size / 32,
   .lower_to_scalar = true,
   .lower_vote_eq = true,
   .lower_subgroup_masks = true,
   .lower_read_invocation_to_cond = true,
   .lower_shuffle = true,
   .lower_relative_shuffle = true,
            if (!((s->info.stage == MESA_SHADER_COMPUTE) ||
         (s->info.stage == MESA_SHADER_KERNEL) ||
      options.subgroup_size = 1;
                           if ((s->info.stage == MESA_SHADER_COMPUTE) ||
      (s->info.stage == MESA_SHADER_KERNEL)) {
   bool progress = false;
            /* ir3_nir_lower_subgroup_id_cs creates extra compute intrinsics which
   * we need to lower again.
   */
   if (progress)
               /* we cannot ensure that ir3_finalize_nir() is only called once, so
   * we also need to do any run-once workarounds here:
   */
            const nir_lower_image_options lower_image_opts = {
      .lower_cube_size = true,
      };
            const nir_lower_idiv_options lower_idiv_options = {
         };
               /* The resinfo opcode returns the size in dwords on a4xx */
   if (compiler->gen == 4)
            /* The resinfo opcode we have for getting the SSBO size on a6xx returns a
   * byte length divided by IBO_0_FMT, while the NIR intrinsic coming in is a
   * number of bytes. Switch things so the NIR intrinsic in our backend means
   * dwords.
   */
   if (compiler->gen >= 6)
               }
      static bool
   lower_ucp_vs(struct ir3_shader_variant *so)
   {
      if (!so->key.ucp_enables)
                     if (so->key.has_gs) {
         } else if (so->key.tessellation) {
         } else {
                     }
      void
   ir3_nir_lower_variant(struct ir3_shader_variant *so, nir_shader *s)
   {
               if (ir3_shader_debug & IR3_DBG_DISASM) {
      mesa_logi("----------------------");
   nir_log_shaderi(s);
                                 if (so->key.has_gs || so->key.tessellation) {
      switch (so->type) {
   case MESA_SHADER_VERTEX:
      NIR_PASS_V(s, ir3_nir_lower_to_explicit_output, so,
         progress = true;
      case MESA_SHADER_TESS_CTRL:
      NIR_PASS_V(s, nir_lower_io_to_scalar,
         NIR_PASS_V(s, ir3_nir_lower_tess_ctrl, so, so->key.tessellation);
   NIR_PASS_V(s, ir3_nir_lower_to_explicit_input, so);
   progress = true;
      case MESA_SHADER_TESS_EVAL:
      NIR_PASS_V(s, ir3_nir_lower_tess_eval, so, so->key.tessellation);
   if (so->key.has_gs)
      NIR_PASS_V(s, ir3_nir_lower_to_explicit_output, so,
      progress = true;
      case MESA_SHADER_GEOMETRY:
      NIR_PASS_V(s, ir3_nir_lower_to_explicit_input, so);
   progress = true;
      default:
                     /* Note that it is intentional to use the VS lowering pass for GS, since we
   * lower GS into something that looks more like a VS in ir3_nir_lower_gs():
   */
   if (lower_ucp_vs(so)) {
         } else if (s->info.stage == MESA_SHADER_FRAGMENT) {
      if (so->key.ucp_enables && !so->compiler->has_clip_cull)
               /* Move large constant variables to the constants attached to the NIR
   * shader, which we will upload in the immediates range.  This generates
   * amuls, so we need to clean those up after.
   *
   * Passing no size_align, we would get packed values, which if we end up
   * having to load with LDC would result in extra reads to unpack from
   * straddling loads.  Align everything to vec4 to avoid that, though we
   * could theoretically do better.
   */
   OPT_V(s, nir_opt_large_constants, glsl_get_vec4_size_align_bytes,
                  /* Lower large temporaries to scratch, which in Qualcomm terms is private
   * memory, to avoid excess register pressure. This should happen after
   * nir_opt_large_constants, because loading from a UBO is much, much less
   * expensive.
   */
   if (so->compiler->has_pvtmem) {
      progress |= OPT(s, nir_lower_vars_to_scratch, nir_var_function_temp,
               /* Lower scratch writemasks */
            progress |= OPT(s, ir3_nir_lower_wide_load_store);
   progress |= OPT(s, ir3_nir_lower_64b_global);
   progress |= OPT(s, ir3_nir_lower_64b_intrinsics);
   progress |= OPT(s, ir3_nir_lower_64b_undef);
            /* Cleanup code leftover from lowering passes before opt_preamble */
   if (progress) {
                  /* Do the preamble before analysing UBO ranges, because it's usually
   * higher-value and because it can result in eliminating some indirect UBO
   * accesses where otherwise we'd have to push the whole range. However we
   * have to lower the preamble after UBO lowering so that UBO lowering can
   * insert instructions in the preamble to push UBOs.
   */
   if (so->compiler->has_preamble &&
      !(ir3_shader_debug & IR3_DBG_NOPREAMBLE))
         if (!so->binning_pass)
                     if (so->shader_options.push_consts_type == IR3_PUSH_CONSTS_SHARED_PREAMBLE)
                              /* UBO offset lowering has to come after we've decided what will
   * be left as load_ubo
   */
   if (so->compiler->gen >= 6)
                     if (progress)
            /* Fixup indirect load_uniform's which end up with a const base offset
   * which is too large to encode.  Do this late(ish) so we actually
   * can differentiate indirect vs non-indirect.
   */
   if (OPT(s, ir3_nir_fixup_load_uniform))
            /* Do late algebraic optimization to turn add(a, neg(b)) back into
   * subs, then the mandatory cleanup after algebraic.  Note that it may
   * produce fnegs, and if so then we need to keep running to squash
   * fneg(fneg(a)).
   */
   bool more_late_algebraic = true;
   while (more_late_algebraic) {
      more_late_algebraic = OPT(s, nir_opt_algebraic_late);
   if (!more_late_algebraic && so->compiler->gen >= 5) {
      /* Lowers texture operations that have only f2f16 or u2u16 called on
   * them to have a 16-bit destination.  Also, lower 16-bit texture
   * coordinates that had been upconverted to 32-bits just for the
   * sampler to just be 16-bit texture sources.
   */
   struct nir_fold_tex_srcs_options fold_srcs_options = {
      .sampler_dims = ~0,
   .src_types = (1 << nir_tex_src_coord) |
               (1 << nir_tex_src_lod) |
   (1 << nir_tex_src_bias) |
   (1 << nir_tex_src_offset) |
   (1 << nir_tex_src_comparator) |
   (1 << nir_tex_src_min_lod) |
      };
   struct nir_fold_16bit_tex_image_options fold_16bit_options = {
      .rounding_mode = nir_rounding_mode_rtz,
   .fold_tex_dest_types = nir_type_float,
   /* blob dumps have no half regs on pixel 2's ldib or stib, so only enable for a6xx+. */
   .fold_image_dest_types = so->compiler->gen >= 6 ?
         .fold_image_store_data = so->compiler->gen >= 6,
   .fold_srcs_options_count = 1,
      };
      }
   OPT_V(s, nir_opt_constant_folding);
   OPT_V(s, nir_copy_prop);
   OPT_V(s, nir_opt_dce);
                        if (ir3_shader_debug & IR3_DBG_DISASM) {
      mesa_logi("----------------------");
   nir_log_shaderi(s);
                        /* Binning pass variants re-use  the const_state of the corresponding
   * draw pass shader, so that same const emit can be re-used for both
   * passes:
   */
   if (!so->binning_pass)
      }
      static void
   ir3_nir_scan_driver_consts(struct ir3_compiler *compiler, nir_shader *shader, struct ir3_const_state *layout)
   {
      nir_foreach_function (function, shader) {
      if (!function->impl)
            nir_foreach_block (block, function->impl) {
      nir_foreach_instr (instr, block) {
                                    switch (intr->intrinsic) {
   case nir_intrinsic_image_atomic:
   case nir_intrinsic_image_atomic_swap:
   case nir_intrinsic_image_load:
   case nir_intrinsic_image_store:
   case nir_intrinsic_image_size:
      /* a4xx gets these supplied by the hw directly (maybe CP?) */
   if (compiler->gen == 5 &&
      !(intr->intrinsic == nir_intrinsic_image_load &&
         idx = nir_src_as_uint(intr->src[0]);
   if (layout->image_dims.mask & (1 << idx))
         layout->image_dims.mask |= (1 << idx);
   layout->image_dims.off[idx] = layout->image_dims.count;
      }
      case nir_intrinsic_load_base_vertex:
   case nir_intrinsic_load_first_vertex:
      layout->num_driver_params =
            case nir_intrinsic_load_is_indexed_draw:
      layout->num_driver_params =
            case nir_intrinsic_load_base_instance:
      layout->num_driver_params =
            case nir_intrinsic_load_user_clip_plane:
      idx = nir_intrinsic_ucp_id(intr);
   layout->num_driver_params = MAX2(layout->num_driver_params,
            case nir_intrinsic_load_num_workgroups:
      layout->num_driver_params =
            case nir_intrinsic_load_workgroup_id:
      if (!compiler->has_shared_regfile) {
      layout->num_driver_params =
      }
      case nir_intrinsic_load_workgroup_size:
      layout->num_driver_params = MAX2(layout->num_driver_params,
            case nir_intrinsic_load_base_workgroup_id:
      layout->num_driver_params =
            case nir_intrinsic_load_subgroup_size: {
      assert(shader->info.stage == MESA_SHADER_COMPUTE ||
         enum ir3_driver_param size = shader->info.stage == MESA_SHADER_COMPUTE ?
         layout->num_driver_params =
            }
   case nir_intrinsic_load_subgroup_id_shift_ir3:
      layout->num_driver_params =
            case nir_intrinsic_load_draw_id:
      layout->num_driver_params =
            case nir_intrinsic_load_tess_level_outer_default:
      layout->num_driver_params = MAX2(layout->num_driver_params,
            case nir_intrinsic_load_tess_level_inner_default:
      layout->num_driver_params = MAX2(layout->num_driver_params,
            case nir_intrinsic_load_frag_size_ir3:
      layout->num_driver_params = MAX2(layout->num_driver_params,
                  case nir_intrinsic_load_frag_offset_ir3:
      layout->num_driver_params = MAX2(layout->num_driver_params,
                  case nir_intrinsic_load_frag_invocation_count:
      layout->num_driver_params = MAX2(layout->num_driver_params,
            default:
                           /* TODO: Provide a spot somewhere to safely upload unwanted values, and a way
   * to determine if they're wanted or not. For now we always make the whole
   * driver param range available, since the driver will always instruct the
   * hardware to upload these.
   */
   if (!compiler->has_shared_regfile &&
            layout->num_driver_params =
         }
      /* Sets up the variant-dependent constant state for the ir3_shader.  Note
   * that it is also used from ir3_nir_analyze_ubo_ranges() to figure out the
   * maximum number of driver params that would eventually be used, to leave
   * space for this function to allocate the driver params.
   */
   void
   ir3_setup_const_state(nir_shader *nir, struct ir3_shader_variant *v,
         {
                                 if ((compiler->gen < 5) && (v->stream_output.num_outputs > 0)) {
      const_state->num_driver_params =
                        assert((const_state->ubo_state.size % 16) == 0);
   unsigned constoff = v->shader_options.num_reserved_user_consts +
      const_state->ubo_state.size / 16 +
               if (const_state->num_ubos > 0) {
      const_state->offsets.ubo = constoff;
               if (const_state->image_dims.count > 0) {
      unsigned cnt = const_state->image_dims.count;
   const_state->offsets.image_dims = constoff;
               if (v->type == MESA_SHADER_KERNEL) {
      const_state->offsets.kernel_params = constoff;
               if (const_state->num_driver_params > 0) {
      /* num_driver_params in dwords.  we only need to align to vec4s for the
   * common case of immediate constant uploads, but for indirect dispatch
   * the constants may also be indirect and so we have to align the area in
   * const space to that requirement.
   */
   const_state->num_driver_params = align(const_state->num_driver_params, 4);
   unsigned upload_unit = 1;
   if (v->type == MESA_SHADER_COMPUTE ||
      (const_state->num_driver_params >= IR3_DP_VTXID_BASE)) {
               /* offset cannot be 0 for vs params loaded by CP_DRAW_INDIRECT_MULTI */
   if (v->type == MESA_SHADER_VERTEX && compiler->gen >= 6)
         constoff = align(constoff, upload_unit);
                        if ((v->type == MESA_SHADER_VERTEX) && (compiler->gen < 5) &&
      v->stream_output.num_outputs > 0) {
   const_state->offsets.tfbo = constoff;
               switch (v->type) {
   case MESA_SHADER_VERTEX:
      const_state->offsets.primitive_param = constoff;
   constoff += 1;
      case MESA_SHADER_TESS_CTRL:
   case MESA_SHADER_TESS_EVAL:
      const_state->offsets.primitive_param = constoff;
            const_state->offsets.primitive_map = constoff;
   constoff += DIV_ROUND_UP(v->input_size, 4);
      case MESA_SHADER_GEOMETRY:
      const_state->offsets.primitive_param = constoff;
            const_state->offsets.primitive_map = constoff;
   constoff += DIV_ROUND_UP(v->input_size, 4);
      default:
                              }
