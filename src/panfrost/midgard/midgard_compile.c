   /*
   * Copyright (C) 2018-2019 Alyssa Rosenzweig <alyssa@rosenzweig.io>
   * Copyright (C) 2019-2020 Collabora, Ltd.
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
   */
      #include <err.h>
   #include <fcntl.h>
   #include <stdint.h>
   #include <stdio.h>
   #include <stdlib.h>
   #include <sys/mman.h>
   #include <sys/stat.h>
   #include <sys/types.h>
      #include "compiler/glsl/glsl_to_nir.h"
   #include "compiler/nir/nir_builder.h"
   #include "compiler/nir_types.h"
   #include "util/half_float.h"
   #include "util/list.h"
   #include "util/u_debug.h"
   #include "util/u_dynarray.h"
   #include "util/u_math.h"
      #include "compiler.h"
   #include "helpers.h"
   #include "midgard.h"
   #include "midgard_compile.h"
   #include "midgard_nir.h"
   #include "midgard_ops.h"
   #include "midgard_quirks.h"
      #include "disassemble.h"
      static const struct debug_named_value midgard_debug_options[] = {
      {"shaders", MIDGARD_DBG_SHADERS, "Dump shaders in NIR and MIR"},
   {"shaderdb", MIDGARD_DBG_SHADERDB, "Prints shader-db statistics"},
   {"inorder", MIDGARD_DBG_INORDER, "Disables out-of-order scheduling"},
   {"verbose", MIDGARD_DBG_VERBOSE, "Dump shaders verbosely"},
   {"internal", MIDGARD_DBG_INTERNAL, "Dump internal shaders"},
         DEBUG_GET_ONCE_FLAGS_OPTION(midgard_debug, "MIDGARD_MESA_DEBUG",
            int midgard_debug = 0;
      static midgard_block *
   create_empty_block(compiler_context *ctx)
   {
               blk->base.predecessors =
                        }
      static void
   schedule_barrier(compiler_context *ctx)
   {
      midgard_block *temp = ctx->after_block;
   ctx->after_block = create_empty_block(ctx);
   ctx->block_count++;
   list_addtail(&ctx->after_block->base.link, &ctx->blocks);
   list_inithead(&ctx->after_block->base.instructions);
   pan_block_add_successor(&ctx->current_block->base, &ctx->after_block->base);
   ctx->current_block = ctx->after_block;
      }
      /* Helpers to generate midgard_instruction's using macro magic, since every
   * driver seems to do it that way */
      #define EMIT(op, ...) emit_mir_instruction(ctx, v_##op(__VA_ARGS__));
      #define M_LOAD_STORE(name, store, T)                                           \
      static midgard_instruction m_##name(unsigned ssa, unsigned address)         \
   {                                                                           \
      midgard_instruction i = {                                                \
      .type = TAG_LOAD_STORE_4,                                             \
   .mask = 0xF,                                                          \
   .dest = ~0,                                                           \
   .src = {~0, ~0, ~0, ~0},                                              \
   .swizzle = SWIZZLE_IDENTITY_4,                                        \
   .op = midgard_op_##name,                                              \
   .load_store =                                                         \
      {                                                                  \
         };                                                                       \
         if (store) {                                                             \
      i.src[0] = ssa;                                                       \
   i.src_types[0] = T;                                                   \
      } else {                                                                 \
      i.dest = ssa;                                                         \
      }                                                                        \
            #define M_LOAD(name, T)  M_LOAD_STORE(name, false, T)
   #define M_STORE(name, T) M_LOAD_STORE(name, true, T)
      M_LOAD(ld_attr_32, nir_type_uint32);
   M_LOAD(ld_vary_32, nir_type_uint32);
   M_LOAD(ld_ubo_u8, nir_type_uint32); /* mandatory extension to 32-bit */
   M_LOAD(ld_ubo_u16, nir_type_uint32);
   M_LOAD(ld_ubo_32, nir_type_uint32);
   M_LOAD(ld_ubo_64, nir_type_uint32);
   M_LOAD(ld_ubo_128, nir_type_uint32);
   M_LOAD(ld_u8, nir_type_uint8);
   M_LOAD(ld_u16, nir_type_uint16);
   M_LOAD(ld_32, nir_type_uint32);
   M_LOAD(ld_64, nir_type_uint32);
   M_LOAD(ld_128, nir_type_uint32);
   M_STORE(st_u8, nir_type_uint8);
   M_STORE(st_u16, nir_type_uint16);
   M_STORE(st_32, nir_type_uint32);
   M_STORE(st_64, nir_type_uint32);
   M_STORE(st_128, nir_type_uint32);
   M_LOAD(ld_tilebuffer_raw, nir_type_uint32);
   M_LOAD(ld_tilebuffer_16f, nir_type_float16);
   M_LOAD(ld_tilebuffer_32f, nir_type_float32);
   M_STORE(st_vary_32, nir_type_uint32);
   M_LOAD(ld_cubemap_coords, nir_type_uint32);
   M_LOAD(ldst_mov, nir_type_uint32);
   M_LOAD(ld_image_32f, nir_type_float32);
   M_LOAD(ld_image_16f, nir_type_float16);
   M_LOAD(ld_image_32u, nir_type_uint32);
   M_LOAD(ld_image_32i, nir_type_int32);
   M_STORE(st_image_32f, nir_type_float32);
   M_STORE(st_image_16f, nir_type_float16);
   M_STORE(st_image_32u, nir_type_uint32);
   M_STORE(st_image_32i, nir_type_int32);
   M_LOAD(lea_image, nir_type_uint64);
      #define M_IMAGE(op)                                                            \
      static midgard_instruction op##_image(nir_alu_type type, unsigned val,      \
         {                                                                           \
      switch (type) {                                                          \
   case nir_type_float32:                                                   \
         case nir_type_float16:                                                   \
         case nir_type_uint32:                                                    \
         case nir_type_int32:                                                     \
         default:                                                                 \
                  M_IMAGE(ld);
   M_IMAGE(st);
      static midgard_instruction
   v_branch(bool conditional, bool invert)
   {
      midgard_instruction ins = {
      .type = TAG_ALU_4,
   .unit = ALU_ENAB_BRANCH,
   .compact_branch = true,
   .branch =
      {
      .conditional = conditional,
         .dest = ~0,
                  }
      static void
   attach_constants(compiler_context *ctx, midgard_instruction *ins,
         {
      ins->has_constants = true;
      }
      static int
   glsl_type_size(const struct glsl_type *type, bool bindless)
   {
         }
      static bool
   midgard_nir_lower_global_load_instr(nir_builder *b, nir_intrinsic_instr *intr,
         {
      if (intr->intrinsic != nir_intrinsic_load_global &&
      intr->intrinsic != nir_intrinsic_load_shared)
         unsigned compsz = intr->def.bit_size;
   unsigned totalsz = compsz * intr->def.num_components;
   /* 8, 16, 32, 64 and 128 bit loads don't need to be lowered */
   if (util_bitcount(totalsz) < 2 && totalsz <= 128)
                              nir_def *comps[MIR_VEC_COMPONENTS];
            while (totalsz) {
      unsigned loadsz = MIN2(1 << (util_last_bit(totalsz) - 1), 128);
            nir_def *load;
   if (intr->intrinsic == nir_intrinsic_load_global) {
         } else {
      assert(intr->intrinsic == nir_intrinsic_load_shared);
   nir_intrinsic_instr *shared_load =
         shared_load->num_components = loadncomps;
   shared_load->src[0] = nir_src_for_ssa(addr);
   nir_intrinsic_set_align(shared_load, compsz / 8, 0);
   nir_intrinsic_set_base(shared_load, nir_intrinsic_base(intr));
   nir_def_init(&shared_load->instr, &shared_load->def,
         nir_builder_instr_insert(b, &shared_load->instr);
               for (unsigned i = 0; i < loadncomps; i++)
            totalsz -= loadsz;
               assert(ncomps == intr->def.num_components);
               }
      static bool
   midgard_nir_lower_global_load(nir_shader *shader)
   {
      return nir_shader_intrinsics_pass(
      shader, midgard_nir_lower_global_load_instr,
   }
      static bool
   mdg_should_scalarize(const nir_instr *instr, const void *_unused)
   {
               if (nir_src_bit_size(alu->src[0].src) == 64)
            if (alu->def.bit_size == 64)
            switch (alu->op) {
   case nir_op_fdot2:
   case nir_op_umul_high:
   case nir_op_imul_high:
   case nir_op_pack_half_2x16:
            /* The LUT unit is scalar */
   case nir_op_fsqrt:
   case nir_op_frcp:
   case nir_op_frsq:
   case nir_op_fsin_mdg:
   case nir_op_fcos_mdg:
   case nir_op_fexp2:
   case nir_op_flog2:
         default:
            }
      /* Only vectorize int64 up to vec2 */
   static uint8_t
   midgard_vectorize_filter(const nir_instr *instr, const void *data)
   {
      if (instr->type != nir_instr_type_alu)
            const nir_alu_instr *alu = nir_instr_as_alu(instr);
   int src_bit_size = nir_src_bit_size(alu->src[0].src);
            if (src_bit_size == 64 || dst_bit_size == 64)
               }
      void
   midgard_preprocess_nir(nir_shader *nir, unsigned gpu_id)
   {
               /* Lower gl_Position pre-optimisation, but after lowering vars to ssa
   * (so we don't accidentally duplicate the epilogue since mesa/st has
   * messed with our I/O quite a bit already).
   */
            if (nir->info.stage == MESA_SHADER_VERTEX) {
      NIR_PASS_V(nir, nir_lower_viewport_transform);
               NIR_PASS_V(nir, nir_lower_var_copies);
   NIR_PASS_V(nir, nir_lower_vars_to_ssa);
   NIR_PASS_V(nir, nir_split_var_copies);
   NIR_PASS_V(nir, nir_lower_var_copies);
   NIR_PASS_V(nir, nir_lower_global_vars_to_local);
   NIR_PASS_V(nir, nir_lower_var_copies);
            NIR_PASS_V(nir, nir_lower_io, nir_var_shader_in | nir_var_shader_out,
            if (nir->info.stage == MESA_SHADER_VERTEX) {
      /* nir_lower[_explicit]_io is lazy and emits mul+add chains even
   * for offsets it could figure out are constant.  Do some
   * constant folding before pan_nir_lower_store_component below.
   */
   NIR_PASS_V(nir, nir_opt_constant_folding);
               NIR_PASS_V(nir, nir_lower_ssbo);
            NIR_PASS_V(nir, nir_lower_frexp);
            nir_lower_idiv_options idiv_options = {
                           nir_lower_tex_options lower_tex_options = {
      .lower_txs_lod = true,
   .lower_txp = ~0,
   .lower_tg4_broadcom_swizzle = true,
   .lower_txd = true,
               NIR_PASS_V(nir, nir_lower_tex, &lower_tex_options);
            /* TEX_GRAD fails to apply sampler descriptor settings on some
   * implementations, requiring a lowering.
   */
   if (quirks & MIDGARD_BROKEN_LOD)
            /* Midgard image ops coordinates are 16-bit instead of 32-bit */
            if (nir->info.stage == MESA_SHADER_FRAGMENT)
            NIR_PASS_V(nir, pan_lower_helper_invocation);
   NIR_PASS_V(nir, pan_lower_sample_pos);
   NIR_PASS_V(nir, midgard_nir_lower_algebraic_early);
   NIR_PASS_V(nir, nir_lower_alu_to_scalar, mdg_should_scalarize, NULL);
   NIR_PASS_V(nir, nir_lower_flrp, 16 | 32 | 64, false /* always_precise */);
      }
      static void
   optimise_nir(nir_shader *nir, unsigned quirks, bool is_blend)
   {
               do {
                        NIR_PASS(progress, nir, nir_copy_prop);
   NIR_PASS(progress, nir, nir_opt_remove_phis);
   NIR_PASS(progress, nir, nir_opt_dce);
   NIR_PASS(progress, nir, nir_opt_dead_cf);
   NIR_PASS(progress, nir, nir_opt_cse);
   NIR_PASS(progress, nir, nir_opt_peephole_select, 64, false, true);
   NIR_PASS(progress, nir, nir_opt_algebraic);
   NIR_PASS(progress, nir, nir_opt_constant_folding);
   NIR_PASS(progress, nir, nir_opt_undef);
                     NIR_PASS(progress, nir, nir_opt_vectorize, midgard_vectorize_filter,
                        /* Run after opts so it can hit more */
   if (!is_blend)
            do {
               NIR_PASS(progress, nir, nir_opt_dce);
   NIR_PASS(progress, nir, nir_opt_algebraic);
   NIR_PASS(progress, nir, nir_opt_constant_folding);
               NIR_PASS(progress, nir, nir_opt_algebraic_late);
            /* We implement booleans as 32-bit 0/~0 */
            /* Now that booleans are lowered, we can run out late opts */
   NIR_PASS(progress, nir, midgard_nir_lower_algebraic_late);
   NIR_PASS(progress, nir, midgard_nir_cancel_inot);
            /* Clean up after late opts */
   do {
               NIR_PASS(progress, nir, nir_opt_dce);
   NIR_PASS(progress, nir, nir_opt_constant_folding);
               /* Backend scheduler is purely local, so do some global optimizations
   * to reduce register pressure. */
   nir_move_options move_all = nir_move_const_undef | nir_move_load_ubo |
                  NIR_PASS_V(nir, nir_opt_sink, move_all);
            /* Take us out of SSA */
            /* We are a vector architecture; write combine where possible */
   NIR_PASS(progress, nir, nir_move_vec_src_uses_to_dest, false);
            NIR_PASS(progress, nir, nir_opt_dce);
      }
      /* Do not actually emit a load; instead, cache the constant for inlining */
      static void
   emit_load_const(compiler_context *ctx, nir_load_const_instr *instr)
   {
                        assert(instr->def.num_components * instr->def.bit_size <=
         #define RAW_CONST_COPY(bits)                                                   \
      nir_const_value_to_array(consts->u##bits, instr->value,                     \
            switch (instr->def.bit_size) {
   case 64:
      RAW_CONST_COPY(64);
      case 32:
      RAW_CONST_COPY(32);
      case 16:
      RAW_CONST_COPY(16);
      case 8:
      RAW_CONST_COPY(8);
      default:
                  /* Shifted for SSA, +1 for off-by-one */
   _mesa_hash_table_u64_insert(ctx->ssa_constants, (def.index << 1) + 1,
      }
      /* Normally constants are embedded implicitly, but for I/O and such we have to
   * explicitly emit a move with the constant source */
      static void
   emit_explicit_constant(compiler_context *ctx, unsigned node)
   {
      void *constant_value =
            if (constant_value) {
      midgard_instruction ins =
         attach_constants(ctx, &ins, constant_value, node + 1);
         }
      static bool
   nir_is_non_scalar_swizzle(nir_alu_src *src, unsigned nr_components)
   {
               for (unsigned c = 1; c < nr_components; ++c) {
      if (src->swizzle[c] != comp)
                  }
      #define ALU_CASE(nir, _op)                                                     \
      case nir_op_##nir:                                                          \
      op = midgard_alu_op_##_op;                                               \
   assert(src_bitsize == dst_bitsize);                                      \
      #define ALU_CASE_RTZ(nir, _op)                                                 \
      case nir_op_##nir:                                                          \
      op = midgard_alu_op_##_op;                                               \
   roundmode = MIDGARD_RTZ;                                                 \
      #define ALU_CHECK_CMP()                                                        \
      assert(src_bitsize == 16 || src_bitsize == 32 || src_bitsize == 64);        \
         #define ALU_CASE_BCAST(nir, _op, count)                                        \
      case nir_op_##nir:                                                          \
      op = midgard_alu_op_##_op;                                               \
   broadcast_swizzle = count;                                               \
   ALU_CHECK_CMP();                                                         \
      #define ALU_CASE_CMP(nir, _op)                                                 \
      case nir_op_##nir:                                                          \
      op = midgard_alu_op_##_op;                                               \
   ALU_CHECK_CMP();                                                         \
      static void
   mir_copy_src(midgard_instruction *ins, nir_alu_instr *instr, unsigned i,
         {
      nir_alu_src src = instr->src[i];
            ins->src[to] = nir_src_index(NULL, &src.src);
            /* Figure out which component we should fill unused channels with. This
   * doesn't matter too much in the non-broadcast case, but it makes
   * should that scalar sources are packed with replicated swizzles,
   * which works around issues seen with the combination of source
   * expansion and destination shrinking.
   */
   unsigned replicate_c = 0;
   if (bcast_count) {
         } else {
      for (unsigned c = 0; c < NIR_MAX_VEC_COMPONENTS; ++c) {
      if (nir_alu_instr_channel_used(instr, i, c))
                  for (unsigned c = 0; c < NIR_MAX_VEC_COMPONENTS; ++c) {
      ins->swizzle[to][c] =
      src.swizzle[((!bcast_count || c < bcast_count) &&
                  }
      static void
   emit_alu(compiler_context *ctx, nir_alu_instr *instr)
   {
      /* Derivatives end up emitted on the texture pipe, not the ALUs. This
            if (instr->op == nir_op_fddx || instr->op == nir_op_fddy) {
      midgard_emit_derivatives(ctx, instr);
               unsigned nr_components = instr->def.num_components;
   unsigned nr_inputs = nir_op_infos[instr->op].num_inputs;
            /* Number of components valid to check for the instruction (the rest
   * will be forced to the last), or 0 to use as-is. Relevant as
   * ball-type instructions have a channel count in NIR but are all vec4
                     /* Should we swap arguments? */
            ASSERTED unsigned src_bitsize = nir_src_bit_size(instr->src[0].src);
                     switch (instr->op) {
      ALU_CASE(fadd, fadd);
   ALU_CASE(fmul, fmul);
   ALU_CASE(fmin, fmin);
   ALU_CASE(fmax, fmax);
   ALU_CASE(imin, imin);
   ALU_CASE(imax, imax);
   ALU_CASE(umin, umin);
   ALU_CASE(umax, umax);
   ALU_CASE(ffloor, ffloor);
   ALU_CASE(fround_even, froundeven);
   ALU_CASE(ftrunc, ftrunc);
   ALU_CASE(fceil, fceil);
   ALU_CASE(fdot3, fdot3);
   ALU_CASE(fdot4, fdot4);
   ALU_CASE(iadd, iadd);
   ALU_CASE(isub, isub);
   ALU_CASE(iadd_sat, iaddsat);
   ALU_CASE(isub_sat, isubsat);
   ALU_CASE(uadd_sat, uaddsat);
   ALU_CASE(usub_sat, usubsat);
   ALU_CASE(imul, imul);
   ALU_CASE(imul_high, imul);
   ALU_CASE(umul_high, imul);
            /* Zero shoved as second-arg */
            ALU_CASE(uabs_isub, iabsdiff);
                     ALU_CASE_CMP(feq32, feq);
   ALU_CASE_CMP(fneu32, fne);
   ALU_CASE_CMP(flt32, flt);
   ALU_CASE_CMP(ieq32, ieq);
   ALU_CASE_CMP(ine32, ine);
   ALU_CASE_CMP(ilt32, ilt);
            /* We don't have a native b2f32 instruction. Instead, like many
   * GPUs, we exploit booleans as 0/~0 for false/true, and
   * correspondingly AND
   * by 1.0 to do the type conversion. For the moment, prime us
   * to emit:
   *
   * iand [whatever], #0
   *
   * At the end of emit_alu (as MIR), we'll fix-up the constant
            ALU_CASE_CMP(b2f32, iand);
   ALU_CASE_CMP(b2f16, iand);
            ALU_CASE(frcp, frcp);
   ALU_CASE(frsq, frsqrt);
   ALU_CASE(fsqrt, fsqrt);
   ALU_CASE(fexp2, fexp2);
            ALU_CASE_RTZ(f2i64, f2i_rte);
   ALU_CASE_RTZ(f2u64, f2u_rte);
   ALU_CASE_RTZ(i2f64, i2f_rte);
            ALU_CASE_RTZ(f2i32, f2i_rte);
   ALU_CASE_RTZ(f2u32, f2u_rte);
   ALU_CASE_RTZ(i2f32, i2f_rte);
            ALU_CASE_RTZ(f2i8, f2i_rte);
            ALU_CASE_RTZ(f2i16, f2i_rte);
   ALU_CASE_RTZ(f2u16, f2u_rte);
   ALU_CASE_RTZ(i2f16, i2f_rte);
            ALU_CASE(fsin_mdg, fsinpi);
            /* We'll get 0 in the second arg, so:
   * ~a = ~(a | 0) = nor(a, 0) */
   ALU_CASE(inot, inor);
   ALU_CASE(iand, iand);
   ALU_CASE(ior, ior);
   ALU_CASE(ixor, ixor);
   ALU_CASE(ishl, ishl);
   ALU_CASE(ishr, iasr);
            ALU_CASE_BCAST(b32all_fequal2, fball_eq, 2);
   ALU_CASE_BCAST(b32all_fequal3, fball_eq, 3);
            ALU_CASE_BCAST(b32any_fnequal2, fbany_neq, 2);
   ALU_CASE_BCAST(b32any_fnequal3, fbany_neq, 3);
            ALU_CASE_BCAST(b32all_iequal2, iball_eq, 2);
   ALU_CASE_BCAST(b32all_iequal3, iball_eq, 3);
            ALU_CASE_BCAST(b32any_inequal2, ibany_neq, 2);
   ALU_CASE_BCAST(b32any_inequal3, ibany_neq, 3);
            /* Source mods will be shoved in later */
   ALU_CASE(fabs, fmov);
   ALU_CASE(fneg, fmov);
   ALU_CASE(fsat, fmov);
   ALU_CASE(fsat_signed_mali, fmov);
            /* For size conversion, we use a move. Ideally though we would squash
   * these ops together; maybe that has to happen after in NIR as part of
   * propagation...? An earlier algebraic pass ensured we step down by
   * only / exactly one size. If stepping down, we use a dest override to
   * reduce the size; if stepping up, we use a larger-sized move with a
         case nir_op_i2i8:
   case nir_op_i2i16:
   case nir_op_i2i32:
   case nir_op_i2i64:
   case nir_op_u2u8:
   case nir_op_u2u16:
   case nir_op_u2u32:
   case nir_op_u2u64:
   case nir_op_f2f16:
   case nir_op_f2f32:
   case nir_op_f2f64: {
      if (instr->op == nir_op_f2f16 || instr->op == nir_op_f2f32 ||
      instr->op == nir_op_f2f64)
      else
                           /* For greater-or-equal, we lower to less-or-equal and flip the
         case nir_op_fge:
   case nir_op_fge32:
   case nir_op_ige32:
   case nir_op_uge32: {
      op = instr->op == nir_op_fge     ? midgard_alu_op_fle
      : instr->op == nir_op_fge32 ? midgard_alu_op_fle
   : instr->op == nir_op_ige32 ? midgard_alu_op_ile
               flip_src12 = true;
   ALU_CHECK_CMP();
               case nir_op_b32csel:
   case nir_op_b32fcsel_mdg: {
      bool mixed = nir_is_non_scalar_swizzle(&instr->src[0], nr_components);
   bool is_float = instr->op == nir_op_b32fcsel_mdg;
   op = is_float ? (mixed ? midgard_alu_op_fcsel_v : midgard_alu_op_fcsel)
            int index = nir_src_index(ctx, &instr->src[0].src);
                        case nir_op_unpack_32_2x16:
   case nir_op_unpack_32_4x8:
   case nir_op_pack_32_2x16:
   case nir_op_pack_32_4x8: {
      op = midgard_alu_op_imov;
               default:
      mesa_loge("Unhandled ALU op %s\n", nir_op_infos[instr->op].name);
   assert(0);
               /* Promote imov to fmov if it might help inline a constant */
   if (op == midgard_alu_op_imov && nir_src_is_const(instr->src[0].src) &&
      nir_src_bit_size(instr->src[0].src) == 32 &&
   nir_is_same_comp_swizzle(instr->src[0].swizzle,
                              unsigned outmod = 0;
            if (instr->op == nir_op_umul_high || instr->op == nir_op_imul_high) {
         } else if (midgard_is_integer_out_op(op)) {
         } else if (instr->op == nir_op_fsat) {
         } else if (instr->op == nir_op_fsat_signed_mali) {
         } else if (instr->op == nir_op_fclamp_pos_mali) {
                  /* Fetch unit, quirks, etc information */
   unsigned opcode_props = alu_opcode_props[op].props;
            midgard_instruction ins = {
      .type = TAG_ALU_4,
   .dest_type = nir_op_infos[instr->op].output_type | dst_bitsize,
                        for (unsigned i = nr_inputs; i < ARRAY_SIZE(ins.src); ++i)
            if (quirk_flipped_r24) {
      ins.src[0] = ~0;
      } else {
      for (unsigned i = 0; i < nr_inputs; ++i) {
               if (instr->op == nir_op_b32csel || instr->op == nir_op_b32fcsel_mdg) {
      /* The condition is the first argument; move
   * the other arguments up one to be a binary
                  if (i == 0)
         else if (flip_src12)
         else
      } else if (flip_src12) {
                                 if (instr->op == nir_op_fneg || instr->op == nir_op_fabs) {
      /* Lowered to move */
   if (instr->op == nir_op_fneg)
            if (instr->op == nir_op_fabs)
               ins.op = op;
                     if (instr->op == nir_op_b2f32 || instr->op == nir_op_b2i32) {
      /* Presently, our second argument is an inline #0 constant.
   * Switch over to an embedded 1.0 constant (that can't fit
   * inline, since we're 32-bit, not 16-bit like the inline
            ins.has_inline_constant = false;
   ins.src[1] = SSA_FIXED_REGISTER(REGISTER_CONSTANT);
   ins.src_types[1] = nir_type_float32;
            if (instr->op == nir_op_b2f32)
         else
            for (unsigned c = 0; c < 16; ++c)
      } else if (instr->op == nir_op_b2f16) {
      ins.src[1] = SSA_FIXED_REGISTER(REGISTER_CONSTANT);
   ins.src_types[1] = nir_type_float16;
   ins.has_constants = true;
            for (unsigned c = 0; c < 16; ++c)
      } else if (nr_inputs == 1 && !quirk_flipped_r24) {
      /* Lots of instructions need a 0 plonked in */
   ins.has_inline_constant = false;
   ins.src[1] = SSA_FIXED_REGISTER(REGISTER_CONSTANT);
   ins.src_types[1] = ins.src_types[0];
   ins.has_constants = true;
            for (unsigned c = 0; c < 16; ++c)
      } else if (instr->op == nir_op_pack_32_2x16) {
      ins.dest_type = nir_type_uint16;
   ins.mask = mask_of(nr_components * 2);
      } else if (instr->op == nir_op_pack_32_4x8) {
      ins.dest_type = nir_type_uint8;
   ins.mask = mask_of(nr_components * 4);
      } else if (instr->op == nir_op_unpack_32_2x16) {
      ins.dest_type = nir_type_uint32;
   ins.mask = mask_of(nr_components >> 1);
      } else if (instr->op == nir_op_unpack_32_4x8) {
      ins.dest_type = nir_type_uint32;
   ins.mask = mask_of(nr_components >> 2);
                  }
      #undef ALU_CASE
      static void
   mir_set_intr_mask(nir_instr *instr, midgard_instruction *ins, bool is_read)
   {
      nir_intrinsic_instr *intr = nir_instr_as_intrinsic(instr);
   unsigned nir_mask = 0;
            if (is_read) {
               /* Extension is mandatory for 8/16-bit loads */
      } else {
      nir_mask = nir_intrinsic_write_mask(intr);
               /* Once we have the NIR mask, we need to normalize to work in 32-bit space */
   unsigned bytemask = pan_to_bytemask(dsize, nir_mask);
   ins->dest_type = nir_type_uint | dsize;
      }
      /* Uniforms and UBOs use a shared code path, as uniforms are just (slightly
   * optimized) versions of UBO #0 */
      static midgard_instruction *
   emit_ubo_read(compiler_context *ctx, nir_instr *instr, unsigned dest,
               {
               unsigned dest_size = (instr->type == nir_instr_type_intrinsic)
                           /* Pick the smallest intrinsic to avoid out-of-bounds reads */
   if (bitsize <= 8)
         else if (bitsize <= 16)
         else if (bitsize <= 32)
         else if (bitsize <= 64)
         else if (bitsize <= 128)
         else
                     if (instr->type == nir_instr_type_intrinsic)
            if (indirect_offset) {
      ins.src[2] = nir_src_index(ctx, indirect_offset);
   ins.src_types[2] = nir_type_uint32;
            /* X component for the whole swizzle to prevent register
   * pressure from ballooning from the extra components */
   for (unsigned i = 0; i < ARRAY_SIZE(ins.swizzle[2]); ++i)
      } else {
                  if (indirect_offset && !indirect_shift)
                        }
      /* Globals are like UBOs if you squint. And shared memory is like globals if
   * you squint even harder */
      static void
   emit_global(compiler_context *ctx, nir_instr *instr, bool is_read,
         {
               nir_intrinsic_instr *intr = nir_instr_as_intrinsic(instr);
   if (is_read) {
               switch (bitsize) {
   case 8:
      ins = m_ld_u8(srcdest, 0);
      case 16:
      ins = m_ld_u16(srcdest, 0);
      case 32:
      ins = m_ld_32(srcdest, 0);
      case 64:
      ins = m_ld_64(srcdest, 0);
      case 128:
      ins = m_ld_128(srcdest, 0);
      default:
                           /* For anything not aligned on 32bit, make sure we write full
   * 32 bits registers. */
   if (bitsize & 31) {
               for (unsigned c = 0; c < 4 * comps_per_32b; c += comps_per_32b) {
                     unsigned base = ~0;
   for (unsigned i = 0; i < comps_per_32b; i++) {
      if (ins.mask & BITFIELD_BIT(c + i)) {
      base = ins.swizzle[0][c + i];
                           for (unsigned i = 0; i < comps_per_32b; i++) {
      if (!(ins.mask & BITFIELD_BIT(c + i))) {
      ins.swizzle[0][c + i] = base + i;
      }
               } else {
      unsigned bitsize =
            if (bitsize == 8)
         else if (bitsize == 16)
         else if (bitsize <= 32)
         else if (bitsize <= 64)
         else if (bitsize <= 128)
         else
                                 /* Set a valid swizzle for masked out components */
   assert(ins.mask);
            for (unsigned i = 0; i < ARRAY_SIZE(ins.swizzle[0]); ++i) {
      if (!(ins.mask & (1 << i)))
                  }
      static midgard_load_store_op
   translate_atomic_op(nir_atomic_op op)
   {
      /* clang-format off */
   switch (op) {
   case nir_atomic_op_xchg:    return midgard_op_atomic_xchg;
   case nir_atomic_op_cmpxchg: return midgard_op_atomic_cmpxchg;
   case nir_atomic_op_iadd:    return midgard_op_atomic_add;
   case nir_atomic_op_iand:    return midgard_op_atomic_and;
   case nir_atomic_op_imax:    return midgard_op_atomic_imax;
   case nir_atomic_op_imin:    return midgard_op_atomic_imin;
   case nir_atomic_op_ior:     return midgard_op_atomic_or;
   case nir_atomic_op_umax:    return midgard_op_atomic_umax;
   case nir_atomic_op_umin:    return midgard_op_atomic_umin;
   case nir_atomic_op_ixor:    return midgard_op_atomic_xor;
   default: unreachable("Unexpected atomic");
   }
      }
      /* Emit an atomic to shared memory or global memory. */
   static void
   emit_atomic(compiler_context *ctx, nir_intrinsic_instr *instr)
   {
      midgard_load_store_op op =
            nir_alu_type type =
      (op == midgard_op_atomic_imin || op == midgard_op_atomic_imax)
               bool is_shared = (instr->intrinsic == nir_intrinsic_shared_atomic) ||
            unsigned dest = nir_def_index(&instr->def);
   unsigned val = nir_src_index(ctx, &instr->src[1]);
   unsigned bitsize = nir_src_bit_size(instr->src[1]);
            midgard_instruction ins = {
      .type = TAG_LOAD_STORE_4,
   .mask = 0xF,
   .dest = dest,
   .src = {~0, ~0, ~0, val},
   .src_types = {0, 0, 0, type | bitsize},
                        if (op == midgard_op_atomic_cmpxchg) {
      unsigned xchg_val = nir_src_index(ctx, &instr->src[2]);
            ins.src[2] = val;
   ins.src_types[2] = type | bitsize;
            if (is_shared) {
      ins.load_store.arg_reg = REGISTER_LDST_LOCAL_STORAGE_PTR;
   ins.load_store.arg_comp = COMPONENT_Z;
      } else {
                     ins.src[1] = nir_src_index(ctx, src_offset);
         } else
      mir_set_offset(ctx, &ins, src_offset,
                     }
      static void
   emit_varying_read(compiler_context *ctx, unsigned dest, unsigned offset,
               {
      midgard_instruction ins = m_ld_vary_32(dest, PACK_LDST_ATTRIB_OFS(offset));
   ins.mask = mask_of(nr_comp);
            if (type == nir_type_float16) {
      /* Ensure we are aligned so we can pack it later */
               for (unsigned i = 0; i < ARRAY_SIZE(ins.swizzle[0]); ++i)
            midgard_varying_params p = {
      .flat_shading = flat,
   .perspective_correction = 1,
      };
            if (indirect_offset) {
      ins.src[2] = nir_src_index(ctx, indirect_offset);
      } else
            ins.load_store.arg_reg = REGISTER_LDST_ZERO;
            /* For flat shading, we always use .u32 and require 32-bit mode. For
   * smooth shading, we use the appropriate floating-point type.
   *
   * This could be optimized, but it makes it easy to check correctness.
   */
   if (flat) {
      assert(nir_alu_type_get_type_size(type) == 32);
      } else {
               ins.op = (nir_alu_type_get_type_size(type) == 32) ? midgard_op_ld_vary_32
                  }
      static midgard_instruction
   emit_image_op(compiler_context *ctx, nir_intrinsic_instr *instr)
   {
      enum glsl_sampler_dim dim = nir_intrinsic_image_dim(instr);
   unsigned nr_attr = ctx->stage == MESA_SHADER_VERTEX
               unsigned nr_dim = glsl_get_sampler_dim_coordinate_components(dim);
   bool is_array = nir_intrinsic_image_array(instr);
            /* TODO: MSAA */
            unsigned coord_reg = nir_src_index(ctx, &instr->src[1]);
            nir_src *index = &instr->src[0];
            /* For image opcodes, address is used as an index into the attribute
   * descriptor */
   unsigned address = nr_attr;
   if (is_direct)
            midgard_instruction ins;
   if (is_store) { /* emit st_image_* */
      unsigned val = nir_src_index(ctx, &instr->src[3]);
            nir_alu_type type = nir_intrinsic_src_type(instr);
   ins = st_image(type, val, PACK_LDST_ATTRIB_OFS(address));
   nir_alu_type base_type = nir_alu_type_get_base_type(type);
      } else if (instr->intrinsic == nir_intrinsic_image_texel_address) {
      ins =
            } else {                  /* emit ld_image_* */
      nir_alu_type type = nir_intrinsic_dest_type(instr);
   ins = ld_image(type, nir_def_index(&instr->def),
         ins.mask = mask_of(nir_intrinsic_dest_components(instr));
               /* Coord reg */
   ins.src[1] = coord_reg;
   ins.src_types[1] = nir_type_uint16;
   if (nr_dim == 3 || is_array) {
                  /* Image index reg */
   if (!is_direct) {
      ins.src[2] = nir_src_index(ctx, index);
      } else
                        }
      static void
   emit_attr_read(compiler_context *ctx, unsigned dest, unsigned offset,
         {
      midgard_instruction ins = m_ld_attr_32(dest, PACK_LDST_ATTRIB_OFS(offset));
   ins.load_store.arg_reg = REGISTER_LDST_ZERO;
   ins.load_store.index_reg = REGISTER_LDST_ZERO;
            /* Use the type appropriate load */
   switch (t) {
   case nir_type_uint:
   case nir_type_bool:
      ins.op = midgard_op_ld_attr_32u;
      case nir_type_int:
      ins.op = midgard_op_ld_attr_32i;
      case nir_type_float:
      ins.op = midgard_op_ld_attr_32;
      default:
      unreachable("Attempted to load unknown type");
                  }
      static unsigned
   compute_builtin_arg(nir_intrinsic_op op)
   {
      switch (op) {
   case nir_intrinsic_load_workgroup_id:
         case nir_intrinsic_load_local_invocation_id:
         case nir_intrinsic_load_global_invocation_id:
   case nir_intrinsic_load_global_invocation_id_zero_base:
         default:
            }
      static void
   emit_fragment_store(compiler_context *ctx, unsigned src, unsigned src_z,
         {
      assert(rt < ARRAY_SIZE(ctx->writeout_branch));
                                                                  /* Add dependencies */
   ins.src[0] = src;
            if (depth_only)
         else
            for (int i = 0; i < 4; ++i)
            if (~src_z) {
      emit_explicit_constant(ctx, src_z);
   ins.src[2] = src_z;
   ins.src_types[2] = nir_type_uint32;
      }
   if (~src_s) {
      emit_explicit_constant(ctx, src_s);
   ins.src[3] = src_s;
   ins.src_types[3] = nir_type_uint32;
               /* Emit the branch */
   br = emit_mir_instruction(ctx, ins);
   schedule_barrier(ctx);
            /* Push our current location = current block count - 1 = where we'll
               }
      static void
   emit_compute_builtin(compiler_context *ctx, nir_intrinsic_instr *instr)
   {
      unsigned reg = nir_def_index(&instr->def);
   midgard_instruction ins = m_ldst_mov(reg, 0);
   ins.mask = mask_of(3);
   ins.swizzle[0][3] = COMPONENT_X; /* xyzx */
   ins.load_store.arg_reg = compute_builtin_arg(instr->intrinsic);
      }
      static unsigned
   vertex_builtin_arg(nir_intrinsic_op op)
   {
      switch (op) {
   case nir_intrinsic_load_vertex_id_zero_base:
         case nir_intrinsic_load_instance_id:
         default:
            }
      static void
   emit_vertex_builtin(compiler_context *ctx, nir_intrinsic_instr *instr)
   {
      unsigned reg = nir_def_index(&instr->def);
   emit_attr_read(ctx, reg, vertex_builtin_arg(instr->intrinsic), 1,
      }
      static void
   emit_special(compiler_context *ctx, nir_intrinsic_instr *instr, unsigned idx)
   {
               midgard_instruction ld = m_ld_tilebuffer_raw(reg, 0);
   ld.op = midgard_op_ld_special_32u;
   ld.load_store.signed_offset = PACK_LDST_SELECTOR_OFS(idx);
            for (int i = 0; i < 4; ++i)
               }
      static void
   emit_control_barrier(compiler_context *ctx)
   {
      midgard_instruction ins = {
      .type = TAG_TEXTURE_4,
   .dest = ~0,
   .src = {~0, ~0, ~0, ~0},
                  }
      static uint8_t
   output_load_rt_addr(compiler_context *ctx, nir_intrinsic_instr *instr)
   {
               if (loc >= FRAG_RESULT_DATA0)
            if (loc == FRAG_RESULT_DEPTH)
         if (loc == FRAG_RESULT_STENCIL)
               }
      static void
   emit_intrinsic(compiler_context *ctx, nir_intrinsic_instr *instr)
   {
               switch (instr->intrinsic) {
   case nir_intrinsic_decl_reg:
   case nir_intrinsic_store_reg:
      /* Always fully consumed */
         case nir_intrinsic_load_reg: {
      /* NIR guarantees that, for typical isel, this will always be fully
   * consumed. However, we also do our own nir_scalar chasing for
   * address arithmetic, bypassing the source chasing helpers. So we can end
   * up with unconsumed load_register instructions. Translate them here. 99%
   * of the time, these moves will be DCE'd away.
   */
            midgard_instruction ins =
                     ins.mask = BITFIELD_MASK(instr->def.num_components);
   emit_mir_instruction(ctx, ins);
               case nir_intrinsic_discard_if:
   case nir_intrinsic_discard: {
      bool conditional = instr->intrinsic == nir_intrinsic_discard_if;
   struct midgard_instruction discard = v_branch(conditional, false);
            if (conditional) {
      discard.src[0] = nir_src_index(ctx, &instr->src[0]);
               emit_mir_instruction(ctx, discard);
                        case nir_intrinsic_image_load:
   case nir_intrinsic_image_store:
   case nir_intrinsic_image_texel_address:
      emit_image_op(ctx, instr);
         case nir_intrinsic_load_ubo:
   case nir_intrinsic_load_global:
   case nir_intrinsic_load_global_constant:
   case nir_intrinsic_load_shared:
   case nir_intrinsic_load_scratch:
   case nir_intrinsic_load_input:
   case nir_intrinsic_load_interpolated_input: {
      bool is_ubo = instr->intrinsic == nir_intrinsic_load_ubo;
   bool is_global = instr->intrinsic == nir_intrinsic_load_global ||
         bool is_shared = instr->intrinsic == nir_intrinsic_load_shared;
   bool is_scratch = instr->intrinsic == nir_intrinsic_load_scratch;
   bool is_flat = instr->intrinsic == nir_intrinsic_load_input;
   bool is_interp =
            /* Get the base type of the intrinsic */
   /* TODO: Infer type? Does it matter? */
   nir_alu_type t = (is_interp) ? nir_type_float
                           if (!(is_ubo || is_global || is_scratch)) {
                                    bool direct = nir_src_is_const(*src_offset);
            if (direct)
            /* We may need to apply a fractional offset */
   int component =
                  if (is_ubo) {
                              uint32_t uindex = nir_src_as_uint(index);
   emit_ubo_read(ctx, &instr->instr, reg, offset, indirect_offset, 0,
      } else if (is_global || is_shared || is_scratch) {
      unsigned seg =
            } else if (ctx->stage == MESA_SHADER_FRAGMENT && !ctx->inputs->is_blend) {
      emit_varying_read(ctx, reg, offset, nr_comp, component,
      } else if (ctx->inputs->is_blend) {
                              if (*input == ~0)
         else
      } else if (ctx->stage == MESA_SHADER_VERTEX) {
         } else {
                              /* Handled together with load_interpolated_input */
   case nir_intrinsic_load_barycentric_pixel:
   case nir_intrinsic_load_barycentric_centroid:
   case nir_intrinsic_load_barycentric_sample:
                     case nir_intrinsic_load_raw_output_pan: {
               /* T720 and below use different blend opcodes with slightly
                     unsigned target = output_load_rt_addr(ctx, instr);
   ld.load_store.index_comp = target & 0x3;
            if (nir_src_is_const(instr->src[0])) {
      unsigned sample = nir_src_as_uint(instr->src[0]);
   ld.load_store.arg_comp = sample & 0x3;
      } else {
      /* Enable sample index via register. */
   ld.load_store.signed_offset |= 1;
   ld.src[1] = nir_src_index(ctx, &instr->src[0]);
               if (ctx->quirks & MIDGARD_OLD_BLEND) {
      ld.op = midgard_op_ld_special_32u;
   ld.load_store.signed_offset = PACK_LDST_SELECTOR_OFS(16);
               emit_mir_instruction(ctx, ld);
               case nir_intrinsic_load_output: {
                        midgard_instruction ld;
   if (bits == 16)
         else
            unsigned index = output_load_rt_addr(ctx, instr);
   ld.load_store.index_comp = index & 0x3;
            for (unsigned c = 4; c < 16; ++c)
            if (ctx->quirks & MIDGARD_OLD_BLEND) {
      if (bits == 16)
         else
         ld.load_store.signed_offset = PACK_LDST_SELECTOR_OFS(1);
               emit_mir_instruction(ctx, ld);
               case nir_intrinsic_store_output:
   case nir_intrinsic_store_combined_output_pan:
                        if (ctx->stage == MESA_SHADER_FRAGMENT) {
                              unsigned reg_z = ~0, reg_s = ~0, reg_2 = ~0;
   unsigned writeout = PAN_WRITEOUT_C;
   if (combined) {
      writeout = nir_intrinsic_component(instr);
   if (writeout & PAN_WRITEOUT_Z)
         if (writeout & PAN_WRITEOUT_S)
         if (writeout & PAN_WRITEOUT_2)
                                    } else {
                  /* Dual-source blend writeout is done by leaving the
   * value in r2 for the blend shader to use. */
                                                            } else if (ctx->stage == MESA_SHADER_VERTEX) {
               /* We should have been vectorized, though we don't
   * currently check that st_vary is emitted only once
   * per slot (this is relevant, since there's not a mask
   * parameter available on the store [set to 0 by the
   * blob]). We do respect the component by adjusting the
                                                   /* ABI: Format controlled by the attribute descriptor.
   * This simplifies flat shading, although it prevents
   * certain (unimplemented) 16-bit optimizations.
   *
   * In particular, it lets the driver handle internal
   * TGSI shaders that set flat in the VS but smooth in
   * the FS. This matches our handling on Bifrost.
   */
   bool auto32 = true;
                                 midgard_instruction st =
                        /* Attribute instruction uses these 2-bits for the
   * a32 and table bits, pack this specially.
   */
                  /* nir_intrinsic_component(store_intr) encodes the
   * destination component start. Source component offset
   * adjustment is taken care of in
   * install_registers_instr(), when offset_swizzle() is
   * called.
                  assert(nr_comp > 0);
   for (unsigned i = 0; i < ARRAY_SIZE(st.swizzle); ++i) {
      st.swizzle[0][i] = src_component;
   if (i >= dst_component && i < dst_component + nr_comp - 1)
                  } else {
                        /* Special case of store_output for lowered blend shaders */
   case nir_intrinsic_store_raw_output_pan: {
      assert(ctx->stage == MESA_SHADER_FRAGMENT);
            nir_io_semantics sem = nir_intrinsic_io_semantics(instr);
   assert(sem.location >= FRAG_RESULT_DATA0);
            emit_fragment_store(ctx, reg, ~0, ~0, rt + MIDGARD_COLOR_RT0,
                     case nir_intrinsic_store_global:
   case nir_intrinsic_store_shared:
   case nir_intrinsic_store_scratch:
      reg = nir_src_index(ctx, &instr->src[0]);
            unsigned seg;
   if (instr->intrinsic == nir_intrinsic_store_global)
         else if (instr->intrinsic == nir_intrinsic_store_shared)
         else
            emit_global(ctx, &instr->instr, false, reg, &instr->src[1], seg);
         case nir_intrinsic_load_workgroup_id:
   case nir_intrinsic_load_local_invocation_id:
   case nir_intrinsic_load_global_invocation_id:
   case nir_intrinsic_load_global_invocation_id_zero_base:
      emit_compute_builtin(ctx, instr);
         case nir_intrinsic_load_vertex_id_zero_base:
   case nir_intrinsic_load_instance_id:
      emit_vertex_builtin(ctx, instr);
         case nir_intrinsic_load_sample_mask_in:
      emit_special(ctx, instr, 96);
         case nir_intrinsic_load_sample_id:
      emit_special(ctx, instr, 97);
         case nir_intrinsic_barrier:
      if (nir_intrinsic_execution_scope(instr) != SCOPE_NONE) {
      schedule_barrier(ctx);
   emit_control_barrier(ctx);
      } else if (nir_intrinsic_memory_scope(instr) != SCOPE_NONE) {
      /* Midgard doesn't seem to want special handling, though we do need to
   * take care when scheduling to avoid incorrect reordering.
   *
   * Note this is an "else if" since the handling for the execution scope
   * case already covers the case when both scopes are present.
   */
      }
         case nir_intrinsic_shared_atomic:
   case nir_intrinsic_shared_atomic_swap:
   case nir_intrinsic_global_atomic:
   case nir_intrinsic_global_atomic_swap:
      emit_atomic(ctx, instr);
         default:
      fprintf(stderr, "Unhandled intrinsic %s\n",
         assert(0);
         }
      /* Returns dimension with 0 special casing cubemaps */
   static unsigned
   midgard_tex_format(enum glsl_sampler_dim dim)
   {
      switch (dim) {
   case GLSL_SAMPLER_DIM_1D:
   case GLSL_SAMPLER_DIM_BUF:
            case GLSL_SAMPLER_DIM_2D:
   case GLSL_SAMPLER_DIM_MS:
   case GLSL_SAMPLER_DIM_EXTERNAL:
   case GLSL_SAMPLER_DIM_RECT:
            case GLSL_SAMPLER_DIM_3D:
            case GLSL_SAMPLER_DIM_CUBE:
            default:
            }
      /* Tries to attach an explicit LOD or bias as a constant. Returns whether this
   * was successful */
      static bool
   pan_attach_constant_bias(compiler_context *ctx, nir_src lod,
         {
               if (!nir_src_is_const(lod))
                     /* Break into fixed-point */
   signed lod_int = f;
            /* Carry over negative fractions */
   if (lod_frac < 0.0) {
      lod_int--;
               /* Encode */
   word->bias = float_to_ubyte(lod_frac);
               }
      static enum mali_texture_mode
   mdg_texture_mode(nir_tex_instr *instr)
   {
      if (instr->op == nir_texop_tg4 && instr->is_shadow)
         else if (instr->op == nir_texop_tg4)
         else if (instr->is_shadow)
         else
      }
      static void
   set_tex_coord(compiler_context *ctx, nir_tex_instr *instr,
         {
                        int comparator_idx = nir_tex_instr_src_index(instr, nir_tex_src_comparator);
   int ms_idx = nir_tex_instr_src_index(instr, nir_tex_src_ms_index);
   assert(comparator_idx < 0 || ms_idx < 0);
                              ins->src_types[1] = nir_tex_instr_src_type(instr, coord_idx) |
            unsigned nr_comps = instr->coord_components;
            /* Initialize all components to coord.x which is expected to always be
   * present. Swizzle is updated below based on the texture dimension
   * and extra attributes that are packed in the coordinate argument.
   */
   for (unsigned c = 0; c < MIR_VEC_COMPONENTS; c++)
            /* Shadow ref value is part of the coordinates if there's no comparator
   * source, in that case it's always placed in the last component.
   * Midgard wants the ref value in coord.z.
   */
   if (instr->is_shadow && comparator_idx < 0) {
      ins->swizzle[1][COMPONENT_Z] = --nr_comps;
               /* The array index is the last component if there's no shadow ref value
   * or second last if there's one. We already decremented the number of
   * components to account for the shadow ref value above.
   * Midgard wants the array index in coord.w.
   */
   if (instr->is_array) {
      ins->swizzle[1][COMPONENT_W] = --nr_comps;
               if (instr->sampler_dim == GLSL_SAMPLER_DIM_CUBE) {
      /* texelFetch is undefined on samplerCube */
                     /* For cubemaps, we use a special ld/st op to select the face
   * and copy the xy into the texture register
   */
   midgard_instruction ld = m_ld_cubemap_coords(ins->src[1], 0);
   ld.src[1] = coords;
   ld.src_types[1] = ins->src_types[1];
   ld.mask = 0x3; /* xy */
   ld.load_store.bitsize_toggle = true;
   ld.swizzle[1][3] = COMPONENT_X;
            /* We packed cube coordiates (X,Y,Z) into (X,Y), update the
   * written mask accordingly and decrement the number of
   * components
   */
   nr_comps--;
               /* Now flag tex coord components that have not been written yet */
   write_mask |= mask_of(nr_comps) & ~written_mask;
   for (unsigned c = 0; c < nr_comps; c++)
            /* Sample index and shadow ref are expected in coord.z */
   if (ms_or_comparator_idx >= 0) {
               unsigned sample_or_ref =
                     if (ins->src[1] == ~0)
                     for (unsigned c = 0; c < MIR_VEC_COMPONENTS; c++)
            mov.mask = 1 << COMPONENT_Z;
   written_mask |= 1 << COMPONENT_Z;
   ins->swizzle[1][COMPONENT_Z] = COMPONENT_Z;
               /* Texelfetch coordinates uses all four elements (xyz/index) regardless
   * of texture dimensionality, which means it's necessary to zero the
   * unused components to keep everything happy.
   */
   if (ins->op == midgard_tex_op_fetch && (written_mask | write_mask) != 0xF) {
      if (ins->src[1] == ~0)
            /* mov index.zw, #0, or generalized */
   midgard_instruction mov =
         mov.has_constants = true;
   mov.mask = (written_mask | write_mask) ^ 0xF;
   emit_mir_instruction(ctx, mov);
   for (unsigned c = 0; c < MIR_VEC_COMPONENTS; c++) {
      if (mov.mask & (1 << c))
                  if (ins->src[1] == ~0) {
      /* No temporary reg created, use the src coords directly */
      } else if (write_mask) {
      /* Move the remaining coordinates to the temporary reg */
            for (unsigned c = 0; c < MIR_VEC_COMPONENTS; c++) {
      if ((1 << c) & write_mask) {
      mov.swizzle[1][c] = ins->swizzle[1][c];
      } else {
                     mov.mask = write_mask;
         }
      static void
   emit_texop_native(compiler_context *ctx, nir_tex_instr *instr,
         {
      int texture_index = instr->texture_index;
            /* If txf is used, we assume there is a valid sampler bound at index 0. Use
   * it for txf operations, since there may be no other valid samplers. This is
   * a workaround: txf does not require a sampler in NIR (so sampler_index is
   * undefined) but we need one in the hardware. This is ABI with the driver.
   */
   if (!nir_tex_instr_need_sampler(instr))
            midgard_instruction ins = {
      .type = TAG_TEXTURE_4,
   .mask = 0xF,
   .dest = nir_def_index(&instr->def),
   .src = {~0, ~0, ~0, ~0},
   .dest_type = instr->dest_type,
   .swizzle = SWIZZLE_IDENTITY_4,
   .outmod = midgard_outmod_none,
   .op = midgard_texop,
   .texture = {
      .format = midgard_tex_format(instr->sampler_dim),
   .texture_handle = texture_index,
   .sampler_handle = sampler_index,
            if (instr->is_shadow && !instr->is_new_style_shadow &&
      instr->op != nir_texop_tg4)
   for (int i = 0; i < 4; ++i)
         for (unsigned i = 0; i < instr->num_srcs; ++i) {
      int index = nir_src_index(ctx, &instr->src[i].src);
   unsigned sz = nir_src_bit_size(instr->src[i].src);
            switch (instr->src[i].src_type) {
   case nir_tex_src_coord:
                  case nir_tex_src_bias:
   case nir_tex_src_lod: {
               bool is_txf = midgard_texop == midgard_tex_op_fetch;
   if (!is_txf &&
                  ins.texture.lod_register = true;
                                                      case nir_tex_src_offset: {
      ins.texture.offset_register = true;
                                 emit_explicit_constant(ctx, index);
               case nir_tex_src_comparator:
   case nir_tex_src_ms_index:
                  default: {
      fprintf(stderr, "Unknown texture source type: %d\n",
            }
                  }
      static void
   emit_tex(compiler_context *ctx, nir_tex_instr *instr)
   {
      switch (instr->op) {
   case nir_texop_tex:
   case nir_texop_txb:
      emit_texop_native(ctx, instr, midgard_tex_op_normal);
      case nir_texop_txl:
   case nir_texop_tg4:
      emit_texop_native(ctx, instr, midgard_tex_op_gradient);
      case nir_texop_txf:
   case nir_texop_txf_ms:
      emit_texop_native(ctx, instr, midgard_tex_op_fetch);
      default: {
      fprintf(stderr, "Unhandled texture op: %d\n", instr->op);
      }
      }
      static void
   emit_jump(compiler_context *ctx, nir_jump_instr *instr)
   {
      switch (instr->type) {
   case nir_jump_break: {
      /* Emit a branch out of the loop */
   struct midgard_instruction br = v_branch(false, false);
   br.branch.target_type = TARGET_BREAK;
   br.branch.target_break = ctx->current_loop_depth;
   emit_mir_instruction(ctx, br);
               default:
            }
      static void
   emit_instr(compiler_context *ctx, struct nir_instr *instr)
   {
      switch (instr->type) {
   case nir_instr_type_load_const:
      emit_load_const(ctx, nir_instr_as_load_const(instr));
         case nir_instr_type_intrinsic:
      emit_intrinsic(ctx, nir_instr_as_intrinsic(instr));
         case nir_instr_type_alu:
      emit_alu(ctx, nir_instr_as_alu(instr));
         case nir_instr_type_tex:
      emit_tex(ctx, nir_instr_as_tex(instr));
         case nir_instr_type_jump:
      emit_jump(ctx, nir_instr_as_jump(instr));
         case nir_instr_type_undef:
      /* Spurious */
         default:
            }
      /* ALU instructions can inline or embed constants, which decreases register
   * pressure and saves space. */
      #define CONDITIONAL_ATTACH(idx)                                                \
      {                                                                           \
      void *entry =                                                            \
      _mesa_hash_table_u64_search(ctx->ssa_constants, alu->src[idx] + 1);   \
      if (entry) {                                                             \
      attach_constants(ctx, alu, entry, alu->src[idx] + 1);                 \
               static void
   inline_alu_constants(compiler_context *ctx, midgard_block *block)
   {
      mir_foreach_instr_in_block(block, alu) {
      /* Other instructions cannot inline constants */
   if (alu->type != TAG_ALU_4)
         if (alu->compact_branch)
            /* If there is already a constant here, we can do nothing */
   if (alu->has_constants)
                     if (!alu->has_constants) {
         } else if (!alu->inline_constant) {
      /* Corner case: _two_ vec4 constants, for instance with a
   * csel. For this case, we can only use a constant
                  void *entry =
                  if (entry) {
      midgard_instruction ins =
                                 /* Inject us -before- the last instruction which set r31, if
   * possible.
   */
                  if (alu == first) {
         } else {
                     }
      unsigned
   max_bitsize_for_alu(midgard_instruction *ins)
   {
      unsigned max_bitsize = 0;
   for (int i = 0; i < MIR_SRC_COUNT; i++) {
      if (ins->src[i] == ~0)
         unsigned src_bitsize = nir_alu_type_get_type_size(ins->src_types[i]);
      }
   unsigned dst_bitsize = nir_alu_type_get_type_size(ins->dest_type);
            /* We emulate 8-bit as 16-bit for simplicity of packing */
            /* We don't have fp16 LUTs, so we'll want to emit code like:
   *
   *      vlut.fsinr hr0, hr0
   *
   * where both input and output are 16-bit but the operation is carried
   * out in 32-bit
            switch (ins->op) {
   case midgard_alu_op_fsqrt:
   case midgard_alu_op_frcp:
   case midgard_alu_op_frsqrt:
   case midgard_alu_op_fsinpi:
   case midgard_alu_op_fcospi:
   case midgard_alu_op_fexp2:
   case midgard_alu_op_flog2:
      max_bitsize = MAX2(max_bitsize, 32);
         default:
                  /* High implies computing at a higher bitsize, e.g umul_high of 32-bit
   * requires computing at 64-bit */
   if (midgard_is_integer_out_op(ins->op) &&
      ins->outmod == midgard_outmod_keephi) {
   max_bitsize *= 2;
                  }
      midgard_reg_mode
   reg_mode_for_bitsize(unsigned bitsize)
   {
      switch (bitsize) {
         case 8:
   case 16:
         case 32:
         case 64:
         default:
            }
      /* Midgard supports two types of constants, embedded constants (128-bit) and
   * inline constants (16-bit). Sometimes, especially with scalar ops, embedded
   * constants can be demoted to inline constants, for space savings and
   * sometimes a performance boost */
      static void
   embedded_to_inline_constant(compiler_context *ctx, midgard_block *block)
   {
      mir_foreach_instr_in_block(block, ins) {
      if (!ins->has_constants)
         if (ins->has_inline_constant)
                     /* We can inline 32-bit (sometimes) or 16-bit (usually) */
   bool is_16 = max_bitsize == 16;
            if (!(is_16 || is_32))
            /* src1 cannot be an inline constant due to encoding
   * restrictions. So, if possible we try to flip the arguments
                     if (ins->src[0] == SSA_FIXED_REGISTER(REGISTER_CONSTANT) &&
      alu_opcode_props[op].props & OP_COMMUTES) {
               if (ins->src[1] == SSA_FIXED_REGISTER(REGISTER_CONSTANT)) {
      /* Component is from the swizzle. Take a nonzero component */
   assert(ins->mask);
                                 if (is_16) {
                           /* Constant overflow after resize */
   if (scaled_constant != ins->constants.u32[component])
      } else {
                     /* Check for loss of precision. If this is
   * mediump, we don't care, but for a highp
   * shader, we need to pay attention. NIR
   * doesn't yet tell us which mode we're in!
                           if (fp32 != original)
               /* Should've been const folded */
                                                               for (unsigned c = 0; c < MIR_VEC_COMPONENTS; ++c) {
      /* We only care if this component is actually used */
                                 if (test != value) {
      is_vector = true;
                                 /* Get rid of the embedded constant */
   ins->has_constants = false;
   ins->src[1] = ~0;
   ins->has_inline_constant = true;
            }
      /* Dead code elimination for branches at the end of a block - only one branch
   * per block is legal semantically */
      static void
   midgard_cull_dead_branch(compiler_context *ctx, midgard_block *block)
   {
               mir_foreach_instr_in_block_safe(block, ins) {
      if (!midgard_is_branch_unit(ins->unit))
            if (branched)
                  }
      /* We want to force the invert on AND/OR to the second slot to legalize into
   * iandnot/iornot. The relevant patterns are for AND (and OR respectively)
   *
   *   ~a & #b = ~a & ~(#~b)
   *   ~a & b = b & ~a
   */
      static void
   midgard_legalize_invert(compiler_context *ctx, midgard_block *block)
   {
      mir_foreach_instr_in_block(block, ins) {
      if (ins->type != TAG_ALU_4)
            if (ins->op != midgard_alu_op_iand && ins->op != midgard_alu_op_ior)
            if (ins->src_invert[1] || !ins->src_invert[0])
            if (ins->has_inline_constant) {
      /* ~(#~a) = ~(~#a) = a, so valid, and forces both
   * inverts on */
   ins->inline_constant = ~ins->inline_constant;
      } else {
      /* Flip to the right invert order. Note
   * has_inline_constant false by assumption on the
   * branch, so flipping makes sense. */
            }
      static unsigned
   emit_fragment_epilogue(compiler_context *ctx, unsigned rt, unsigned sample_iter)
   {
      /* Loop to ourselves */
   midgard_instruction *br = ctx->writeout_branch[rt][sample_iter];
   struct midgard_instruction ins = v_branch(false, false);
   ins.writeout = br->writeout;
   ins.branch.target_block = ctx->block_count - 1;
   ins.constants.u32[0] = br->constants.u32[0];
   memcpy(&ins.src_types, &br->src_types, sizeof(ins.src_types));
            ctx->current_block->epilogue = true;
   schedule_barrier(ctx);
      }
      static midgard_block *
   emit_block_init(compiler_context *ctx)
   {
      midgard_block *this_block = ctx->after_block;
            if (!this_block)
                     this_block->scheduled = false;
            /* Set up current block */
   list_inithead(&this_block->base.instructions);
               }
      static midgard_block *
   emit_block(compiler_context *ctx, nir_block *block)
   {
               nir_foreach_instr(instr, block) {
      emit_instr(ctx, instr);
                  }
      static midgard_block *emit_cf_list(struct compiler_context *ctx,
            static void
   emit_if(struct compiler_context *ctx, nir_if *nif)
   {
               /* Speculatively emit the branch, but we can't fill it in until later */
   EMIT(branch, true, true);
   midgard_instruction *then_branch = mir_last_in_block(ctx->current_block);
   then_branch->src[0] = nir_src_index(ctx, &nif->condition);
            /* Emit the two subblocks. */
   midgard_block *then_block = emit_cf_list(ctx, &nif->then_list);
            /* Emit a jump from the end of the then block to the end of the else */
   EMIT(branch, false, false);
                     int else_idx = ctx->block_count;
   int count_in = ctx->instruction_count;
   midgard_block *else_block = emit_cf_list(ctx, &nif->else_list);
   midgard_block *end_else_block = ctx->current_block;
                     assert(then_block);
            if (ctx->instruction_count == count_in) {
      /* The else block is empty, so don't emit an exit jump */
   mir_remove_instruction(then_exit);
      } else {
      then_branch->branch.target_block = else_idx;
                                 pan_block_add_successor(&before_block->base, &then_block->base);
            pan_block_add_successor(&end_then_block->base, &ctx->after_block->base);
      }
      static void
   emit_loop(struct compiler_context *ctx, nir_loop *nloop)
   {
               /* Remember where we are */
            /* Allocate a loop number, growing the current inner loop depth */
            /* Get index from before the body so we can loop back later */
            /* Emit the body itself */
            /* Branch back to loop back */
   struct midgard_instruction br_back = v_branch(false, false);
   br_back.branch.target_block = start_idx;
            /* Mark down that branch in the graph. */
   pan_block_add_successor(&start_block->base, &loop_block->base);
            /* Find the index of the block about to follow us (note: we don't add
   * one; blocks are 0-indexed so we get a fencepost problem) */
            /* Fix up the break statements we emitted to point to the right place,
   * now that we can allocate a block number for them */
            mir_foreach_block_from(ctx, start_block, _block) {
      mir_foreach_instr_in_block(((midgard_block *)_block), ins) {
      if (ins->type != TAG_ALU_4)
                        /* We found a branch -- check the type to see if we need to do anything
   */
                  /* It's a break! Check if it's our break */
                                                               /* Now that we've finished emitting the loop, free up the depth again
   * so we play nice with recursion amid nested loops */
            /* Dump loop stats */
      }
      static midgard_block *
   emit_cf_list(struct compiler_context *ctx, struct exec_list *list)
   {
               foreach_list_typed(nir_cf_node, node, node, list) {
      switch (node->type) {
   case nir_cf_node_block: {
                                          case nir_cf_node_if:
                  case nir_cf_node_loop:
                  case nir_cf_node_function:
      assert(0);
                     }
      /* Due to lookahead, we need to report the first tag executed in the command
   * stream and in branch targets. An initial block might be empty, so iterate
   * until we find one that 'works' */
      unsigned
   midgard_get_first_tag_from_block(compiler_context *ctx, unsigned block_idx)
   {
               mir_foreach_block_from(ctx, initial_block, _v) {
      midgard_block *v = (midgard_block *)_v;
   if (v->quadword_count) {
                                    /* Default to a tag 1 which will break from the shader, in case we jump
               }
      /* For each fragment writeout instruction, generate a writeout loop to
   * associate with it */
      static void
   mir_add_writeout_loops(compiler_context *ctx)
   {
      for (unsigned rt = 0; rt < ARRAY_SIZE(ctx->writeout_branch); ++rt) {
      for (unsigned s = 0; s < MIDGARD_MAX_SAMPLE_ITER; ++s) {
      midgard_instruction *br = ctx->writeout_branch[rt][s];
                  unsigned popped = br->branch.target_block;
   pan_block_add_successor(&(mir_get_block(ctx, popped - 1)->base),
                        /* If we have more RTs, we'll need to restore back after our
                                                if (next_br) {
      midgard_instruction uncond = v_branch(false, false);
   uncond.branch.target_block = popped;
   uncond.branch.target_type = TARGET_GOTO;
   emit_mir_instruction(ctx, uncond);
   pan_block_add_successor(&ctx->current_block->base,
            } else {
      /* We're last, so we can terminate here */
               }
      void
   midgard_compile_shader_nir(nir_shader *nir,
                     {
               /* TODO: Bound against what? */
            ctx->inputs = inputs;
   ctx->nir = nir;
   ctx->info = info;
   ctx->stage = nir->info.stage;
   ctx->blend_input = ~0;
   ctx->blend_src1 = ~0;
                              /* Collect varyings after lowering I/O */
            /* Optimisation passes */
            bool skip_internal = nir->info.internal;
            if (midgard_debug & MIDGARD_DBG_SHADERS && !skip_internal)
                     nir_foreach_function_with_impl(func, impl, nir) {
      list_inithead(&ctx->blocks);
   ctx->block_count = 0;
            if (nir->info.outputs_read && !inputs->is_blend) {
                                                   emit_cf_list(ctx, &impl->body);
                        mir_foreach_block(ctx, _block) {
      midgard_block *block = (midgard_block *)_block;
   inline_alu_constants(ctx, block);
      }
                     do {
      progress = false;
   progress |= midgard_opt_dead_code_eliminate(ctx);
            mir_foreach_block(ctx, _block) {
      midgard_block *block = (midgard_block *)_block;
   progress |= midgard_opt_copy_prop(ctx, block);
   progress |= midgard_opt_combine_projection(ctx, block);
                  mir_foreach_block(ctx, _block) {
      midgard_block *block = (midgard_block *)_block;
   midgard_lower_derivatives(ctx, block);
   midgard_legalize_invert(ctx, block);
               if (ctx->stage == MESA_SHADER_FRAGMENT)
            /* Analyze now that the code is known but before scheduling creates
   * pipeline registers which are harder to track */
            if (midgard_debug & MIDGARD_DBG_SHADERS && !skip_internal)
            /* Schedule! */
   midgard_schedule_program(ctx);
            if (midgard_debug & MIDGARD_DBG_SHADERS && !skip_internal)
            /* Analyze after scheduling since this is order-dependent */
            /* Emit flat binary from the instruction arrays. Iterate each block in
   * sequence. Save instruction boundaries such that lookahead tags can
                     int bundle_count = 0;
   mir_foreach_block(ctx, _block) {
      midgard_block *block = (midgard_block *)_block;
      }
   midgard_bundle **source_order_bundles =
         int bundle_idx = 0;
   mir_foreach_block(ctx, _block) {
      midgard_block *block = (midgard_block *)_block;
   util_dynarray_foreach(&block->bundles, midgard_bundle, bundle) {
                              /* Midgard prefetches instruction types, so during emission we
   * need to lookahead. Unless this is the last instruction, in
            mir_foreach_block(ctx, _block) {
      midgard_block *block = (midgard_block *)_block;
   mir_foreach_bundle_in_block(block, bundle) {
                              emit_binary_bundle(ctx, block, bundle, binary, lookahead);
               /* TODO: Free deeper */
                        /* Report the very first tag executed */
                     if (midgard_debug & MIDGARD_DBG_SHADERS && !skip_internal) {
      disassemble_midgard(stdout, binary->data, binary->size, inputs->gpu_id,
                     /* A shader ending on a 16MB boundary causes INSTR_INVALID_PC faults,
   * workaround by adding some padding to the end of the shader. (The
   * kernel makes sure shader BOs can't cross 16MB boundaries.) */
   if (binary->size)
            if ((midgard_debug & MIDGARD_DBG_SHADERDB || inputs->debug) &&
      !nir->info.internal) {
                     mir_foreach_block(ctx, _block) {
      midgard_block *block = (midgard_block *)_block;
                  mir_foreach_bundle_in_block(block, bun)
               /* Calculate thread count. There are certain cutoffs by
                     unsigned nr_threads = (nr_registers <= 4)   ? 4
                                    asprintf(&shaderdb,
            "%s shader: "
   "%u inst, %u bundles, %u quadwords, "
   "%u registers, %u threads, %u loops, "
   "%u:%u spills:fills",
   ctx->inputs->is_blend ? "PAN_SHADER_BLEND"
               if (midgard_debug & MIDGARD_DBG_SHADERDB)
            if (inputs->debug)
                        _mesa_hash_table_u64_destroy(ctx->ssa_constants);
      }
