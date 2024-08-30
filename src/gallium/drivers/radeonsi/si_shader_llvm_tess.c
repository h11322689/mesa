   /*
   * Copyright 2020 Advanced Micro Devices, Inc.
   *
   * SPDX-License-Identifier: MIT
   */
      #include "si_pipe.h"
   #include "si_shader_internal.h"
   #include "sid.h"
      LLVMValueRef si_get_rel_patch_id(struct si_shader_context *ctx)
   {
      switch (ctx->stage) {
   case MESA_SHADER_TESS_CTRL:
            case MESA_SHADER_TESS_EVAL:
      return ctx->abi.tes_rel_patch_id_replaced ?
               default:
      assert(0);
         }
      /* Tessellation shaders pass outputs to the next shader using LDS.
   *
   * LS outputs = TCS inputs
   * TCS outputs = TES inputs
   *
   * The LDS layout is:
   * - TCS inputs for patch 0
   * - TCS inputs for patch 1
   * - TCS inputs for patch 2             = get_tcs_in_current_patch_offset (if RelPatchID==2)
   * - ...
   * - TCS outputs for patch 0            = get_tcs_out_patch0_offset
   * - Per-patch TCS outputs for patch 0  = get_tcs_out_patch0_patch_data_offset
   * - TCS outputs for patch 1
   * - Per-patch TCS outputs for patch 1
   * - TCS outputs for patch 2            = get_tcs_out_current_patch_offset (if RelPatchID==2)
   * - Per-patch TCS outputs for patch 2  = get_tcs_out_current_patch_data_offset (if RelPatchID==2)
   * - ...
   *
   * All three shaders VS(LS), TCS, TES share the same LDS space.
   */
      static LLVMValueRef get_tcs_out_patch0_patch_data_offset(struct si_shader_context *ctx)
   {
         }
      static LLVMValueRef get_tcs_out_current_patch_data_offset(struct si_shader_context *ctx)
   {
      LLVMValueRef patch0_patch_data_offset = get_tcs_out_patch0_patch_data_offset(ctx);
   unsigned patch_dw_stride = si_get_tcs_out_patch_stride(&ctx->shader->selector->info);
   LLVMValueRef patch_stride = LLVMConstInt(ctx->ac.i32, patch_dw_stride, 0);
               }
      /* The offchip buffer layout for TCS->TES is
   *
   * - attribute 0 of patch 0 vertex 0
   * - attribute 0 of patch 0 vertex 1
   * - attribute 0 of patch 0 vertex 2
   *   ...
   * - attribute 0 of patch 1 vertex 0
   * - attribute 0 of patch 1 vertex 1
   *   ...
   * - attribute 1 of patch 0 vertex 0
   * - attribute 1 of patch 0 vertex 1
   *   ...
   * - per patch attribute 0 of patch 0
   * - per patch attribute 0 of patch 1
   *   ...
   *
   * Note that every attribute has 4 components.
   */
   static LLVMValueRef get_tcs_tes_buffer_address(struct si_shader_context *ctx,
               {
      LLVMValueRef base_addr, num_patches;
            num_patches = si_unpack_param(ctx, ctx->args->tcs_offchip_layout, 0, 6);
            constant16 = LLVMConstInt(ctx->ac.i32, 16, 0);
   base_addr = rel_patch_id;
            base_addr = ac_build_imad(&ctx->ac, param_index, param_stride, base_addr);
            LLVMValueRef patch_data_offset = si_unpack_param(ctx, ctx->args->tcs_offchip_layout, 16, 16);
      }
      /**
   * Load from LSHS LDS storage.
   *
   * \param type     output value type
   * \param swizzle  offset (typically 0..3); it can be ~0, which loads a vec4
   * \param dw_addr  address in dwords
   */
   static LLVMValueRef lshs_lds_load(struct si_shader_context *ctx, LLVMTypeRef type, unsigned swizzle,
         {
               if (swizzle == ~0) {
               for (unsigned chan = 0; chan < 4; chan++)
                        dw_addr = LLVMBuildAdd(ctx->ac.builder, dw_addr, LLVMConstInt(ctx->ac.i32, swizzle, 0), "");
   value = ac_lds_load(&ctx->ac, dw_addr);
      }
      enum si_tess_ring
   {
      TESS_FACTOR_RING,
      };
      static LLVMValueRef get_tess_ring_descriptor(struct si_shader_context *ctx, enum si_tess_ring ring)
   {
      LLVMBuilderRef builder = ctx->ac.builder;
            if (ring == TESS_FACTOR_RING) {
      unsigned tf_offset = ctx->screen->hs.tess_offchip_ring_size;
               uint32_t rsrc3 = S_008F0C_DST_SEL_X(V_008F0C_SQ_SEL_X) | S_008F0C_DST_SEL_Y(V_008F0C_SQ_SEL_Y) |
            if (ctx->screen->info.gfx_level >= GFX11)
      rsrc3 |= S_008F0C_FORMAT(V_008F0C_GFX11_FORMAT_32_FLOAT) |
      else if (ctx->screen->info.gfx_level >= GFX10)
      rsrc3 |= S_008F0C_FORMAT(V_008F0C_GFX10_FORMAT_32_FLOAT) |
      else
      rsrc3 |= S_008F0C_NUM_FORMAT(V_008F0C_BUF_NUM_FORMAT_FLOAT) |
         LLVMValueRef desc[4];
   desc[0] = addr;
   desc[1] = LLVMConstInt(ctx->ac.i32, S_008F04_BASE_ADDRESS_HI(ctx->screen->info.address32_hi), 0);
   desc[2] = LLVMConstInt(ctx->ac.i32, 0xffffffff, 0);
               }
      static LLVMValueRef si_nir_load_tcs_varyings(struct ac_shader_abi *abi, LLVMTypeRef type,
                     {
      struct si_shader_context *ctx = si_shader_context_from_abi(abi);
                     uint8_t semantic = info->input[driver_location].semantic;
   /* Load the TCS input from a VGPR. */
   unsigned func_param = ctx->args->ac.tcs_rel_ids.arg_index + 1 +
            LLVMValueRef value[4];
   for (unsigned i = component; i < component + num_components; i++) {
      value[i] = LLVMGetParam(ctx->main_fn.value, func_param + i);
                  }
      static void si_write_tess_factors(struct si_shader_context *ctx, union si_shader_part_key *key,
                     {
      struct si_shader *shader = ctx->shader;
   unsigned tess_inner_index, tess_outer_index;
   LLVMValueRef lds_base, lds_inner, lds_outer, byteoffset, buffer;
   LLVMValueRef out[6], vec0, vec1, tf_base, inner[4], outer[4];
            /* Add a barrier before loading tess factors from LDS. */
   if (!shader->key.ge.part.tcs.epilog.invoc0_tess_factors_are_def) {
               if (!key->tcs_epilog.noop_s_barrier)
               /* Do this only for invocation 0, because the tess levels are per-patch,
   * not per-vertex.
   *
   * This can't jump, because invocation 0 executes this. It should
   * at least mask out the loads and stores for other invocations.
   */
   ac_build_ifcc(&ctx->ac,
            /* Determine the layout of one tess factor element in the buffer. */
   switch (shader->key.ge.part.tcs.epilog.prim_mode) {
   case TESS_PRIMITIVE_ISOLINES:
      stride = 2; /* 2 dwords, 1 vec2 store */
   outer_comps = 2;
   inner_comps = 0;
      case TESS_PRIMITIVE_TRIANGLES:
      stride = 4; /* 4 dwords, 1 vec4 store */
   outer_comps = 3;
   inner_comps = 1;
      case TESS_PRIMITIVE_QUADS:
      stride = 6; /* 6 dwords, 2 stores (vec4 + vec2) */
   outer_comps = 4;
   inner_comps = 2;
      default:
      assert(0);
               for (i = 0; i < 4; i++) {
      inner[i] = LLVMGetUndef(ctx->ac.i32);
               if (shader->key.ge.part.tcs.epilog.invoc0_tess_factors_are_def) {
      /* Tess factors are in VGPRs. */
   for (i = 0; i < outer_comps; i++)
         for (i = 0; i < inner_comps; i++)
      } else {
      /* Load tess_inner and tess_outer from LDS.
   * Any invocation can write them, so we can't get them from a temporary.
   */
   tess_inner_index = ac_shader_io_get_unique_index_patch(VARYING_SLOT_TESS_LEVEL_INNER);
            lds_base = tcs_out_current_patch_data_offset;
   lds_inner = LLVMBuildAdd(ctx->ac.builder, lds_base,
         lds_outer = LLVMBuildAdd(ctx->ac.builder, lds_base,
            for (i = 0; i < outer_comps; i++) {
         }
   for (i = 0; i < inner_comps; i++) {
                     if (shader->key.ge.part.tcs.epilog.prim_mode == TESS_PRIMITIVE_ISOLINES) {
      /* For isolines, the hardware expects tess factors in the
   * reverse order from what NIR specifies.
   */
   LLVMValueRef tmp = out[0];
   out[0] = out[1];
               /* Convert the outputs to vectors for stores. */
   vec0 = ac_build_gather_values(&ctx->ac, out, MIN2(stride, 4));
            if (stride > 4)
            /* Get the buffer. */
            /* Get the offset. */
   tf_base = ac_get_arg(&ctx->ac, ctx->args->ac.tcs_factor_offset);
   byteoffset =
                  /* Store the dynamic HS control word. */
   if (ctx->screen->info.gfx_level <= GFX8) {
      ac_build_ifcc(&ctx->ac,
         ac_build_buffer_store_dword(&ctx->ac, buffer, LLVMConstInt(ctx->ac.i32, 0x80000000, 0),
               ac_build_endif(&ctx->ac, 6504);
               /* Store the tessellation factors. */
   ac_build_buffer_store_dword(&ctx->ac, buffer, vec0, NULL,
                     offset += 16;
   if (vec1)
      ac_build_buffer_store_dword(&ctx->ac, buffer, vec1, NULL,
                     /* Store the tess factors into the offchip buffer if TES reads them. */
   if (shader->key.ge.part.tcs.epilog.tes_reads_tess_factors) {
      LLVMValueRef buf, base, inner_vec, outer_vec, tf_outer_offset;
   LLVMValueRef tf_inner_offset;
            buf = get_tess_ring_descriptor(ctx, TESS_OFFCHIP_RING);
            param_outer = ac_shader_io_get_unique_index_patch(VARYING_SLOT_TESS_LEVEL_OUTER);
   tf_outer_offset = get_tcs_tes_buffer_address(ctx, rel_patch_id,
                     ac_build_buffer_store_dword(&ctx->ac, buf, outer_vec, NULL, tf_outer_offset,
         if (inner_comps) {
      param_inner = ac_shader_io_get_unique_index_patch(VARYING_SLOT_TESS_LEVEL_INNER);
                  inner_vec = ac_build_gather_values(&ctx->ac, inner, inner_comps);
   ac_build_buffer_store_dword(&ctx->ac, buf, inner_vec, NULL,
                     }
      /* This only writes the tessellation factor levels. */
   void si_llvm_tcs_build_end(struct si_shader_context *ctx)
   {
      LLVMBuilderRef builder = ctx->ac.builder;
            rel_patch_id = si_get_rel_patch_id(ctx);
   invocation_id = si_unpack_param(ctx, ctx->args->ac.tcs_rel_ids, 8, 5);
            if (ctx->screen->info.gfx_level >= GFX9) {
      LLVMBasicBlockRef blocks[2] = {LLVMGetInsertBlock(builder), ctx->merged_wrap_if_entry_block};
                     values[0] = rel_patch_id;
   values[1] = LLVMGetUndef(ctx->ac.i32);
            values[0] = tf_lds_offset;
   values[1] = LLVMGetUndef(ctx->ac.i32);
            values[0] = invocation_id;
   values[1] = ctx->ac.i32_1; /* cause the epilog to skip threads */
               /* Return epilog parameters from this function. */
   LLVMValueRef ret = ctx->return_value;
            if (ctx->screen->info.gfx_level >= GFX9) {
      ret =
         ret = si_insert_input_ret(ctx, ret, ctx->args->tes_offchip_addr, 8 + GFX9_SGPR_TCS_OFFCHIP_ADDR);
   /* Tess offchip and tess factor offsets are at the beginning. */
   ret = si_insert_input_ret(ctx, ret, ctx->args->ac.tess_offchip_offset, 2);
   ret = si_insert_input_ret(ctx, ret, ctx->args->ac.tcs_factor_offset, 4);
      } else {
      ret = si_insert_input_ret(ctx, ret, ctx->args->tcs_offchip_layout, GFX6_SGPR_TCS_OFFCHIP_LAYOUT);
   ret = si_insert_input_ret(ctx, ret, ctx->args->tes_offchip_addr, GFX6_SGPR_TCS_OFFCHIP_ADDR);
   /* Tess offchip and tess factor offsets are after user SGPRs. */
   ret = si_insert_input_ret(ctx, ret, ctx->args->ac.tess_offchip_offset, GFX6_TCS_NUM_USER_SGPR);
   ret = si_insert_input_ret(ctx, ret, ctx->args->ac.tcs_factor_offset, GFX6_TCS_NUM_USER_SGPR + 1);
               /* VGPRs */
   rel_patch_id = ac_to_float(&ctx->ac, rel_patch_id);
   invocation_id = ac_to_float(&ctx->ac, invocation_id);
            /* Leave a hole corresponding to the two input VGPRs. This ensures that
   * the invocation_id output does not alias the tcs_rel_ids input,
   * which saves a V_MOV on gfx9.
   */
            ret = LLVMBuildInsertValue(builder, ret, rel_patch_id, vgpr++, "");
            struct si_shader_info *info = &ctx->shader->selector->info;
   if (info->tessfactors_are_def_in_all_invocs) {
               /* get tess factor driver location */
   int outer_loc = -1;
   int inner_loc = -1;
   for (int i = 0; i < info->num_outputs; i++) {
      unsigned semantic = info->output_semantic[i];
   if (semantic == VARYING_SLOT_TESS_LEVEL_OUTER)
         else if (semantic == VARYING_SLOT_TESS_LEVEL_INNER)
               for (unsigned i = 0; i < 6; i++) {
      int loc = i < 4 ? outer_loc : inner_loc;
   LLVMValueRef value = loc < 0 ? LLVMGetUndef(ctx->ac.f32) :
         value = ac_to_float(&ctx->ac, value);
         } else {
         }
      }
      void si_llvm_ls_build_end(struct si_shader_context *ctx)
   {
      struct si_shader *shader = ctx->shader;
            /* Only need return value when merged shader on part mode or mono mode with same thread count. */
   if (ctx->screen->info.gfx_level < GFX9 || (shader->is_monolithic && !same_thread_count))
            if (!ctx->shader->is_monolithic)
                     ret = si_insert_input_ptr(ctx, ret, ctx->args->other_const_and_shader_buffers, 0);
   ret = si_insert_input_ptr(ctx, ret, ctx->args->other_samplers_and_images, 1);
   ret = si_insert_input_ret(ctx, ret, ctx->args->ac.tess_offchip_offset, 2);
   ret = si_insert_input_ret(ctx, ret, ctx->args->ac.merged_wave_info, 3);
   ret = si_insert_input_ret(ctx, ret, ctx->args->ac.tcs_factor_offset, 4);
   if (ctx->screen->info.gfx_level <= GFX10_3)
            ret = si_insert_input_ptr(ctx, ret, ctx->args->internal_bindings, 8 + SI_SGPR_INTERNAL_BINDINGS);
   ret = si_insert_input_ptr(ctx, ret, ctx->args->bindless_samplers_and_images,
                     ret = si_insert_input_ret(ctx, ret, ctx->args->tcs_offchip_layout, 8 + GFX9_SGPR_TCS_OFFCHIP_LAYOUT);
            unsigned vgpr = 8 + GFX9_TCS_NUM_USER_SGPR;
   ret = si_insert_input_ret_float(ctx, ret, ctx->args->ac.tcs_patch_id, vgpr++);
            if (same_thread_count) {
      /* Same thread count is set only when mono mode. */
            struct si_shader_info *info = &shader->selector->info;
            for (unsigned i = 0; i < info->num_outputs; i++) {
                     for (unsigned chan = 0; chan < 4; chan++) {
                                                   }
      /**
   * Compile the TCS epilog function. This writes tessellation factors to memory
   * based on the output primitive type of the tessellator (determined by TES).
   */
   void si_llvm_build_tcs_epilog(struct si_shader_context *ctx, union si_shader_part_key *key)
   {
      struct ac_arg rel_patch_id;
   struct ac_arg invocation_id;
   struct ac_arg tcs_out_current_patch_data_offset;
   struct ac_arg tess_factors[6];
   si_get_tcs_epilog_args(ctx->screen->info.gfx_level, ctx->args, &rel_patch_id, &invocation_id,
            /* Create the function. */
   si_llvm_create_func(ctx, "tcs_epilog", NULL, 0, ctx->screen->info.gfx_level >= GFX7 ? 128 : 0);
            LLVMValueRef invoc0_tess_factors[6];
   for (unsigned i = 0; i < 6; i++)
            si_write_tess_factors(ctx, key, ac_get_arg(&ctx->ac, rel_patch_id),
                           }
      void si_llvm_init_tcs_callbacks(struct si_shader_context *ctx)
   {
         }
