   /*
   * Copyright 2020 Advanced Micro Devices, Inc.
   *
   * SPDX-License-Identifier: MIT
   */
      #include "si_pipe.h"
   #include "si_shader_internal.h"
   #include "sid.h"
   #include "util/u_memory.h"
   #include "ac_nir.h"
      static LLVMValueRef get_vertex_index(struct si_shader_context *ctx,
               {
      LLVMValueRef instance_id = ctx->abi.instance_id;
            bool divisor_is_one = key->instance_divisor_is_one & (1u << input_index);
            LLVMValueRef index = NULL;
   if (divisor_is_one)
         else if (divisor_is_fetched) {
               for (unsigned j = 0; j < 4; j++) {
      udiv_factors[j] = si_buffer_load_const(
      ctx, instance_divisor_constbuf,
                  /* The faster NUW version doesn't work when InstanceID == UINT_MAX.
   * Such InstanceID might not be achievable in a reasonable time though.
   */
   index = ac_build_fast_udiv_nuw(
      &ctx->ac, instance_id, udiv_factors[0],
            if (divisor_is_one || divisor_is_fetched) {
      /* Add StartInstance. */
   LLVMValueRef start_instance = ac_get_arg(&ctx->ac, ctx->args->ac.start_instance);
      } else {
      /* VertexID + BaseVertex */
   LLVMValueRef base_vertex = ac_get_arg(&ctx->ac, ctx->args->ac.base_vertex);
                  }
      /**
   * Build the vertex shader prolog function.
   *
   * The inputs are the same as VS (a lot of SGPRs and 4 VGPR system values).
   * All inputs are returned unmodified. The vertex load indices are
   * stored after them, which will be used by the API VS for fetching inputs.
   *
   * For example, the expected outputs for instance_divisors[] = {0, 1, 2} are:
   *   input_v0,
   *   input_v1,
   *   input_v2,
   *   input_v3,
   *   (VertexID + BaseVertex),
   *   (InstanceID + StartInstance),
   *   (InstanceID / 2 + StartInstance)
   */
   void si_llvm_build_vs_prolog(struct si_shader_context *ctx, union si_shader_part_key *key)
   {
      struct si_shader_args *args = ctx->args;
            const unsigned num_input_sgprs = args->ac.num_sgprs_used;
            /* 4 preloaded VGPRs + vertex load indices as prolog outputs */
   const unsigned num_output_gprs =
         LLVMTypeRef returns[num_output_gprs];
            /* Output SGPRs. */
   for (int i = 0; i < num_input_sgprs; i++)
            /* Output VGPRs */
   for (int i = 0; i < num_input_vgprs; i++)
            /* Vertex load indices. */
   for (int i = 0; i < key->vs_prolog.num_inputs; i++)
            /* Create the function. */
   si_llvm_create_func(ctx, "vs_prolog", returns, num_returns, 0);
            LLVMValueRef input_vgprs[num_input_vgprs];
   for (int i = 0; i < num_input_vgprs; i++)
            if (key->vs_prolog.num_merged_next_stage_vgprs) {
               if (key->vs_prolog.as_ls && ctx->screen->info.has_ls_vgpr_init_bug) {
      /* If there are no HS threads, SPI loads the LS VGPRs
   * starting at VGPR 0. Shift them back to where they
   * belong.
   */
   LLVMValueRef hs_thread_count =
                        for (int i = 4; i > 0; --i) {
      input_vgprs[i + 1] = LLVMBuildSelect(ctx->ac.builder, has_hs_threads,
                     ctx->abi.vertex_id = input_vgprs[args->ac.vertex_id.arg_index - num_input_sgprs];
            /* Copy inputs to outputs. This should be no-op, as the registers match,
   * but it will prevent the compiler from overwriting them unintentionally.
   */
   LLVMValueRef ret = ctx->return_value;
   for (int i = 0; i < num_input_sgprs; i++) {
      LLVMValueRef p = LLVMGetParam(func, i);
      }
   for (int i = 0; i < num_input_vgprs; i++) {
      LLVMValueRef p = ac_to_float(&ctx->ac, input_vgprs[i]);
               /* Compute vertex load indices from instance divisors. */
   LLVMValueRef instance_divisor_constbuf =
      key->vs_prolog.states.instance_divisor_is_fetched ?
         for (int i = 0; i < key->vs_prolog.num_inputs; i++) {
      LLVMValueRef index = get_vertex_index(ctx, &key->vs_prolog.states, i,
            index = ac_to_float(&ctx->ac, index);
                  }
